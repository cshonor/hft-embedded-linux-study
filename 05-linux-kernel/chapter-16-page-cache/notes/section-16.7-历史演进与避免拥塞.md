## ⑦ 历史演进与避免拥塞

| 时代 | 机制 | 致命缺陷 |
|------|------|---------|
| 早期（2.4 及以前） | **`bdflush`**（后台写回）+ **`kupdated`**（定期打时间戳/回写） | 两个线程各管一段，`bdflush` 忙起来 `kupdated` 的功能就没人做 |
| 2.6.6 ~ 2.6.31 | **`pdflush`** — 线程池，按负载动态扩缩（2~8 个） | **全局池**：一个设备拥塞 → 线程全被拖死 → 其他设备也写不出去 |
| **2.6.32 起（现役）** | **flusher 线程 / `bdi_writeback`** — 每个 bdi 一个 | — |

#### pdflush 到底怎么死的：拥塞传染

```
pdflush 模型（线程池，谁有活谁接）：

  盘 A（慢机械盘，队列塞满）
     │
     ├─ pdflush/0 ──► 阻塞在盘 A 的 request_queue 上（等待队列有空位）
     ├─ pdflush/1 ──► 同样阻塞在盘 A
     ├─ pdflush/2 ──► 同样阻塞在盘 A
     └─ ...
     
  盘 B（空闲 NVMe，有大量脏页等着写）
     └─ ❌ 没有线程可用 —— 池子里的线程全被盘 A 占着
```

**根因：** pdflush 线程是**全局共享资源**，而"阻塞在某一个设备的队列上"是**设备局部事件**。
局部事件耗尽全局资源，这就是典型的**拥塞传染（congestion propagation）**。

#### flusher 的解法：把线程按设备私有化

| 设计 | 收益 |
|------|------|
| **每个 `backing_dev_info` 一个 `bdi_writeback`** | 盘 A 的 wb 阻塞，与盘 B 的 wb **完全无关** |
| 每个 wb 有自己的 `delayed_work dwork` | 写回是 per-bdi 排队，不抢全局线程池 |
| 每个 wb 有独立的 4 条链表 | 脏 inode 归属明确，无跨设备锁争用 |
| `b_more_io` 链表 | 保证"写不完的 inode"不被饿死（详见 16.5） |

```
旧：一个 pdflush 全局池 ──► 盘 A 拥塞 ──► 盘 B 写回也被拖死
新：wb-A（自己的 dwork/链表）  wb-B（自己的 dwork/链表） ──► 互不影响
```

---

### 现代还多做了一件事：按带宽公平分配

光"独立"还不够——如果两个设备共享同一条总线（比如挂在同一 HBA 上的多块盘），
各自狂写仍会互相抢带宽。v6.6 的 `bdi_writeback` 里带一组**带宽估计**字段：

```c
/* include/linux/backing-dev-defs.h — v6.6 */
unsigned long bw_time_stamp;          /* 上次更新写带宽的时刻 */
unsigned long written_stamp;          /* 那个时刻已写的页数 */
unsigned long write_bandwidth;        /* 瞬时估算写带宽 */
unsigned long avg_write_bandwidth;    /* 平滑后的写带宽，> 0 */

/*
 * The base dirty throttle rate, re-calculated on every 200ms.
 * All the bdi tasks' dirty rate will be curbed under it.
 */
unsigned long dirty_ratelimit;
unsigned long balanced_dirty_ratelimit;
```

| 机制 | 说明 |
|------|------|
| **每 200ms 重算一次** | 由 `bw_dwork`（带宽估算定时器）驱动 |
| **估算每个 bdi 的写带宽** | 用"这段时间写了多少页"除以耗时 |
| **据此给每个 bdi 分配脏页速率额度** | `dirty_ratelimit` —— 慢设备额度小，快设备额度大 |
| **结果** | 一个慢盘不会被塞给它消化不了的脏页，快盘也不会被慢盘拖着走 |

> 这是从"**避免互相堵塞**"（pdflush 时代的目标）进化到"**按比例公平分配**"。

---

### 再下一层：cgroup writeback（v4.2+）

`CONFIG_CGROUP_WRITEBACK` 打开后，一个 bdi 可以分裂出**多个** wb：

```c
/* include/linux/backing-dev-defs.h — v6.6 struct bdi_writeback 尾部 */
#ifdef CONFIG_CGROUP_WRITEBACK
	struct percpu_ref refcnt;
	struct cgroup_subsys_state *memcg_css;   /* 归属哪个 memcg */
	struct cgroup_subsys_state *blkcg_css;   /* 归属哪个 blkcg */
	struct list_head b_attached;
	...
#endif
```

```
                    bdi（块设备）
                        │
        ┌───────────────┼───────────────┐
        ▼               ▼               ▼
   root wb         wb@memcg-A      wb@memcg-B
 （宿主机）      （容器 A 的脏页）  （容器 B 的脏页）
```

| 解决什么 | 容器/多租户下，写回的**归属与限速**：A 容器疯狂写脏页，不能让 B 容器的 fsync 变慢 |
|---------|------------------------------------------------------------------|
| HFT 用途 | 把**日志/回放进程**关进一个 cgroup、`memory.high` 限住，它们的写回突发就不会污染策略进程所在 cgroup |

---

### 化石：源码里还留着的历史痕迹

```c
/* include/linux/backing-dev-defs.h:48 — v6.6 原文注释 */
enum wb_reason {
	...
	/*
	 * There is no bdi forker thread any more and works are done
	 * by emergency worker, however, this is TPs userland visible
	 * and we'll be exposing exactly the same information,
	 * so it has a mismatch name.
	 */
	WB_REASON_FORKER_THREAD,
	...
};
```

> 曾经的 `bdi_forker_thread`（负责按需派生/销毁 flusher 线程的守护线程）**已经不存在了**，
> 活儿交给了 workqueue 的 emergency worker。但 `WB_REASON_FORKER_THREAD` 这个枚举值被保留下来，
> 理由是 **tracepoint 对外可见**——改名会破坏用户态工具（tracefs/perf 脚本）的兼容。
>
> **这是一条通用铁律：tracepoint、sysfs、/proc 的字段一旦发布就是 ABI，宁可留着错误的名字也不能改。**
> （与 [Ch 5.6 系统调用 ABI 永久性](../../chapter-05-system-calls/notes/section-5.6-添加系统调用与替代方案.md) 是同一条原则的两个实例。）

### 完整写回原因枚举（排障时 tracepoint 会打印）

| `WB_REASON_*` | 触发者 |
|---------------|--------|
| `BACKGROUND` | 脏页比例过后台阈值 |
| `VMSCAN` | 内存回收路径（直接回收时发现脏页） |
| `SYNC` | `sync()` / `syncfs()` |
| `PERIODIC` | `dirty_writeback_centisecs` 周期性唤醒 |
| `LAPTOP_TIMER` | laptop mode 定时器（见 16.6） |
| `FS_FREE_SPACE` | 文件系统空间不足，催促写回 |
| `FOREIGN_FLUSH` | 帮助其他 cgroup wb 刷页 |

| 排障命令 | 用途 |
|---------|------|
| `tracefs /sys/kernel/tracing` 的 `writeback:*` tracepoint | 看每次写回的 reason、设备、写了多少页 |
| `bcc/writeback` 工具 | 直接打印写回事件的直方图 |
| `cat /proc/vmstat \| grep nr_` | `nr_dirty` / `nr_writeback` 实时量 |

→ **Ch 14** request_queue · **NVMe 多队列** 时代思想仍相关 · [Ch 16.5 flusher 线程](./section-16.5-flusher-线程.md)

→ **Ch 14** request_queue · **NVMe 多队列** 时代思想仍相关



<details>
<summary>自测题（点击展开）</summary>

**Q1.** pdflush 和 flusher 线程的区别？为什么换？

<details><summary>答案</summary>

pdflush（2.6.6+）：全局线程池，多设备竞争同一池 → 锁竞争。flusher（2.6.32+）：每块设备一个线程（bdi_writeback），无跨设备竞争。NVMe 的 multi-queue 进一步：每 CPU 一个提交队列。演进方向：减少锁竞争、提高并行度。这就是为什么 NVMe + multi-queue 比 SATA 快——不仅是带宽，更是架构并行度。

</details>

**Q2.** pdflush 的失败不是"锁竞争"那么简单，它真正的结构性缺陷是什么？flusher 又是怎么根治的？

<details><summary>答案</summary>

真正的缺陷是 **局部事件耗尽全局资源**：

pdflush 是一个**全局线程池**（2~8 个线程，按负载动态扩缩），任何设备的写回任务都由池子里空闲的线程接。问题是——"写回"这个动作会**阻塞在设备的 request_queue 上**（等队列腾出槽位）。

于是：一块慢盘/拥塞盘把它的写回任务派给线程，线程阻塞；再来任务，再派一个线程，又阻塞……池子很快被这一块盘占满。此时其他**完全空闲**的设备有大量脏页等着写，却**分不到任何线程**——因为线程池是全局共享的。

这是典型的**拥塞传染**：一个设备的问题扩散成整个系统的写回停滞。锁竞争只是表象。

flusher 的根治办法是**把线程按设备私有化**：
- 每个 `backing_dev_info` 拥有自己的 `struct bdi_writeback`，里面有独立的 `delayed_work dwork`；
- 写回任务是 per-bdi 排队的，盘 A 的 wb 阻塞丝毫不影响盘 B 的 wb；
- 脏 inode 挂在各自的 4 条链表上（`b_dirty`/`b_io`/`b_more_io`/`b_dirty_time`），连锁都是各 wb 自己的 `list_lock`。

结构上就是：**共享变私有，全局变分片**。这和 Ch 14 讲的块层从单队列演进到 blk-mq（per-CPU 软件队列 + per-硬件队列）是**同一个思路在两层的各自实现**。

现代还更进一步：v6.6 的 wb 里有 `write_bandwidth`/`avg_write_bandwidth`/`dirty_ratelimit` 一组字段，每 200ms 估算一次设备的真实写带宽，据此给每个 wb **分配脏页速率额度**——从"别互相堵"进化到"按比例公平分"。

</details>

**Q3.** 源码里 `WB_REASON_FORKER_THREAD` 这个枚举值的注释说"已经没有 bdi forker thread 了"，为什么不直接删掉？

<details><summary>答案</summary>

因为 **tracepoint 对外可见，属于用户态 ABI**。

`include/linux/backing-dev-defs.h:48` 的原文注释：
```c
/*
 * There is no bdi forker thread any more and works are done
 * by emergency worker, however, this is TPs userland visible
 * and we'll be exposing exactly the same information,
 * so it has a mismatch name.
 */
WB_REASON_FORKER_THREAD,
```

历史是这样的：早期有个 `bdi_forker_thread` 守护线程，负责按需**派生/销毁**各设备的 flusher 线程（设备有活就派生，闲下来就销毁）。后来这套被 workqueue 的 emergency worker 取代了，线程本身没了。

但 `WB_REASON_FORKER_THREAD` 这个值是 `writeback` tracepoint 输出的字段之一，用户态工具（perf script、BCC 脚本、trace-cmd 报表）会按枚举值解析。改名或删除会让这些工具**静默地解析错误**——比直接报错更糟。

所以内核的选择是：**保留枚举值，只在注释里说明名字已经名不副实**。

这是内核开发的一条通用铁律：**tracepoint / sysfs / /proc / 系统调用号一旦发布就是 ABI，宁可留着错误的名字，也不能改。** Ch 5.6 讲的系统调用"刻在石头上"是同一条原则——只不过那里是 ABI 兼容性，这里是观测接口的兼容性。对我们读代码的人来说，教训是：**看到名字和注释对不上时，信注释不信名字**。

</details>

</details>
---
