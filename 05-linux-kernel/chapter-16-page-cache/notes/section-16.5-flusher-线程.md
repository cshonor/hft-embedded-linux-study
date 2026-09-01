## ⑤ flusher 线程 · The Flusher Threads

**内核线程组** — 负责 **脏页写回**。

> **版本订正：** LKD 说"每个磁盘主轴（spindle）一个 flusher 线程"。现代表述是
> **每个 `backing_dev_info`（bdi）一个 `bdi_writeback`**——bdi 大致对应"一个块设备"，
> 与主轴不再是一一对应（一个 NVMe 设备可能有多个队列，却只有一个 bdi；而 cgroup writeback 又能从一个 bdi 分裂出多个 wb）。

#### 三种触发条件（附 v6.6 精确阈值）

| # | 触发 | 配置 | v6.6 默认值 |
|---|------|------|------------|
| 1 | **可用内存低于阈值** | `vm.dirty_background_ratio`（后台，不阻塞）<br>`vm.dirty_ratio`（**前台，阻塞写者**） | **10%** / **20%** |
| 2 | **脏页停留超时** | `vm.dirty_expire_centisecs` | **3000**（30 秒） |
| 3 | **用户显式同步** | `sync()` / `fsync()` / `fdatasync()` / `syncfs()` | — |

```
脏页积累
    ├─ 超过 10% ────► 后台唤醒 wb 的 dwork（写者无感）
    ├─ 超过 20% ────► 前台限速：写者被 balance_dirty_pages 睡住
    ├─ 脏页 >30s ───► 周期性强制写回（dirty_expire）
    └─ fsync()   ───► 该文件脏页刷盘 + 设备 cache flush（阻塞）
```

---

### bdi_writeback：写回的组织者

```c
/* include/linux/backing-dev-defs.h — v6.6（节选） */
struct bdi_writeback {
	struct backing_dev_info *bdi;
	unsigned long state;
	unsigned long last_old_flush;

	struct list_head b_dirty;      /* ① 脏 inode（还没开始写） */
	struct list_head b_io;         /* ② 本轮正在写回的 inode */
	struct list_head b_more_io;    /* ③ 本轮没写完的 inode（下轮优先） */
	struct list_head b_dirty_time; /* ④ 只有时间戳脏的 inode（atime/mtime） */
	spinlock_t list_lock;          /* 保护上面四条链表 */

	atomic_t writeback_inodes;     /* 正在写回的 inode 数 */

	unsigned long write_bandwidth;      /* 本设备估算写带宽 */
	unsigned long avg_write_bandwidth;  /* 平滑后的写带宽 */
	unsigned long dirty_ratelimit;      /* 本设备被分配的脏页速率额度 */
	unsigned long balanced_dirty_ratelimit;

	spinlock_t work_lock;
	struct list_head work_list;
	struct delayed_work dwork;     /* ← flusher 就是跑这个 workqueue item */
	struct delayed_work bw_dwork;  /* 带宽估算定时器（每 200ms） */
	...
};
```

**四条 inode 链表为什么是四条？** 这是防**活锁**的关键：

| 链表 | 含义 | 为什么要单独一条 |
|------|------|-----------------|
| `b_dirty` | 脏了但本轮还没轮到 | 与"正在写"的分开，避免新脏的 inode 插队 |
| `b_io` | 本轮挑出来要写的 | 一轮写回的"工作集" |
| `b_more_io` | **本轮没写完**的（被限速打断 / 页太多） | 下一轮**优先**处理它们 → 保证每个脏 inode 最终都能写完，不会被新来的活生生饿死 |
| `b_dirty_time` | 只有 atime/ctime 变了，数据没变 | 这类"脏"只需更新时间戳，优先级更低，可以攒更久 |

> 对应到 `struct writeback_control`（`include/linux/writeback.h`）里的 `tagged_writepages` 标志位——
> 注释原文就是 "tag-and-write to avoid livelock"，与 `PAGECACHE_TAG_TOWRITE` 配合使用。

---

### fsync 到底做了几件事（为什么它比"写一次磁盘"贵）

```
fsync(fd)
  1. 把该文件的所有脏页写回            （数据 IO）
  2. 把该文件的元数据 inode 写回        （元数据 IO，fsync 有、fdatasync 无）
  3. 等所有 IO 完成                      （等中断/完成队列）
  4. 发 FLUSH/FUA 命令刷设备的易失缓存  （REQ_PREFLUSH / REQ_FUA，见 Ch 14.4）
  5. 返回，并更新 address_space->wb_err （错误只报一次，见下）
```

> **`wb_err` 的坑：** 写回错误是通过 `address_space->wb_err` 这个**带代号的 seqcount** 报告的。
> 一次错误只会被"消费"一次——`fsync` 返回的 `-EIO` 只会给**第一个**来问的调用者，后来的调用者看到 `wb_err` 没变，就认为"没事"。
> 多进程共享一个日志文件时，这意味着**错误可能被别的进程吃掉**。日志系统最好自己维护持久化位点。

---

| 观测 | 看什么 |
|------|--------|
| **`cachestat`**（BCC） | 页缓存命中率 |
| **`ext4slower`**（BCC） | 慢于阈值的 ext4 操作（含 fsync） |
| **`biosnoop`**（BCC） | 每次块 IO 的延迟与进程归属 |
| **`/proc/meminfo`** | `Dirty:` / `Writeback:` 两项实时脏页量 |
| **`/proc/vmstat`** | `nr_dirty` / `nr_writeback` / `pgscan_*` |

**HFT：** 突发 **`fsync` 日志** 仍可能造成 **P99 尖刺** — 与策略核隔离、异步批量、专用盘。

| 降低 fsync 痛感的四种办法 | 代价 |
|--------------------------|------|
| **批量**：多条日志攒一次 fsync | 丢数据窗口变大 |
| **专用盘**：日志盘与策略/回放盘物理分离 | 硬件成本 |
| **挪线程**：fsync 放到 IO 线程，策略线程只投递 | 架构复杂度（需无锁队列） |
| **`O_DIRECT` + 自管缓冲**：完全绕开页缓存 | 必须自己对齐（见 Ch 14.2），丢内核预读/合并 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** flusher 线程什么时候触发写回？HFT 如何控制？

<details><summary>答案</summary>

触发条件：1) 脏页比例超阈值（vm.dirty_ratio 默认 20%）；2) 脏页存活超时（vm.dirty_expire_centisecs 默认 30s）；3) 用户调用 sync/fsync。HFT 控制：调小 vm.dirty_ratio 减少突发写回；交易日志用 O_SYNC 或 fsync 保证即时落盘。flusher 是后台线程，不影响前台交易线程（除非内存回收时阻塞）。

</details>

**Q2.** `bdi_writeback` 为什么要维护 4 条 inode 链表，而不是 1 条？

<details><summary>答案</summary>

4 条链表（`include/linux/backing-dev-defs.h`）各自解决一个具体问题：

| 链表 | 装什么 | 为什么必须独立 |
|------|--------|--------------|
| `b_dirty` | 脏了但本轮还没轮到的 inode | 让"新脏的"和"正在写的"分开，防止新任务插队到老任务前面 |
| `b_io` | 本轮挑出来要写的 inode | 一轮写回的工作集，写完后要么清空要么挪走 |
| `b_more_io` | 本轮**没写完**的 inode | **防活锁的关键**：下一轮优先处理它们，保证"写不完的 inode"最终一定写得完，不会被源源不断的新脏页饿死 |
| `b_dirty_time` | 只有 atime/mtime 变了、数据没脏 | 时间戳的"脏"不占数据风险，可以攒更久，降级处理 |

如果只有 1 条链表，一个持续被追加写的大日志文件会永远待在链表头部附近——每次挑 inode 都挑到它，其他文件的脏页**永远轮不到写回**。这就是活锁。

配合 `PAGECACHE_TAG_TOWRITE`（xarray 第 3 个 tag 位）标记"本轮已挑中"，下一轮扫描会跳过已标记条目，从剩下的继续——这就是 `struct writeback_control` 里 `tagged_writepages` 标志位注释写的 "tag-and-write to avoid livelock"。

</details>

**Q3.** 多线程共写一个日志文件，某次 `fsync` 返回 `-EIO`，其他线程调用 `fsync` 却返回成功，为什么？

<details><summary>答案</summary>

因为写回错误是通过 `address_space->wb_err` 这个 **errseq_t（带代号的序列号）** 上报的，语义是**一次性消费**。

机制：
1. 写回失败时，内核递增 `mapping->wb_err`（这是一个 seqcount，低 2 位是"错误标志"，高位是"代号"）；
2. `fsync` 调用者在**打开文件/首次同步时**记下当时的 `wb_err` 值（保存在 `file->f_wb_err`）；
3. 每次 `fsync` 比较"当前 wb_err" 和"我上次记的"，不同则报告错误并更新自己记的值，**相同则返回成功**。

结果：**第一次**问的调用者拿到 `-EIO`，之后问的调用者因为值已经"被消费"过，看到没变化就认为一切正常——错误被静默吞掉。

对 HFT 日志系统的影响与对策：
- 不要依赖 `fsync` 的返回值作为唯一持久化判据；
- 自己维护一个**单调递增的持久化位点**（写成功后更新共享内存/单独的小文件），由**单一线程**负责 fsync 并检查结果；
- 或者干脆用 `O_SYNC` 打开，让每次 `write` 的返回值直接暴露错误（`write` 返回短写/错误，语义更直观）；
- 监控上要盯 `/proc/meminfo` 的 `Writeback:` 异常残留和设备 `dmesg` 里的 IO error，而不是只看应用日志。

</details>

</details>
---
