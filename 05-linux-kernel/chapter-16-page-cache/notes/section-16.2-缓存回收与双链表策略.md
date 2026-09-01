## ② 缓存回收与双链表策略

内存紧张时需 **驱逐缓存页** — 优先换出 **干净** 页。

#### 版本断崖：双链表从一个「文件级」结构变成 per-node 的**五**条链表

LKD 描述的是 `address_space` 里那两条链表。现代内核把它搬到了**内存节点层**：

```c
/* include/linux/mmzone.h — v6.6 */
enum lru_list {
	LRU_INACTIVE_ANON = LRU_BASE,                 /* 匿名页·冷 */
	LRU_ACTIVE_ANON   = LRU_BASE + LRU_ACTIVE,    /* 匿名页·热 */
	LRU_INACTIVE_FILE = LRU_BASE + LRU_FILE,      /* 文件页·冷 ← 页缓存在这 */
	LRU_ACTIVE_FILE   = LRU_BASE + LRU_FILE + LRU_ACTIVE,  /* 文件页·热 */
	LRU_UNEVICTABLE,                              /* mlock 住的、不可回收 */
	NR_LRU_LISTS
};

struct lruvec {
	struct list_head	lists[NR_LRU_LISTS];
	spinlock_t		lru_lock;      /* 每 lruvec 一把锁 */
	unsigned long		anon_cost;     /* 记录换出代价，动态平衡扫描比例 */
	unsigned long		file_cost;
	...
};
```

| 变化 | 旧（LKD 时代） | 现代 v6.6 |
|------|---------------|----------|
| 链表归属 | `address_space` 内 | **每个 NUMA 节点 / 每个 memcg 各自一套 `lruvec`** |
| 链表数量 | 2（active / inactive） | **5**（anon/file 各冷热 + unevictable） |
| 锁 | 全局或 per-zone | **per-lruvec `lru_lock`**，NUMA 多节点天然分片 |
| 冷热判定 | 简单二次机会 | 二次机会 + refault distance + 可选 MGLRU |

**为什么把 anon 和 file 分开？** 因为二者的**回收代价完全不同**：

| 类型 | 回收时要做什么 | 代价 |
|------|--------------|------|
| **file 干净页** | 直接丢弃（数据已在盘上） | ~0 |
| **file 脏页** | 先写回再丢弃 | 一次磁盘 IO |
| **anon 页** | 必须写入 swap 才能丢弃 | 一次磁盘 IO + swap 占用 |

`lruvec` 里的 `anon_cost` / `file_cost` 就是用来**动态权衡**"这次该多扫 anon 还是多扫 file"的经验值。

---

#### 双链表到底解决了什么（LKD 的"一次扫描污染"问题）

```
假设单链表 LRU，你跑了一次 find / 扫了 20 万个文件：

  旧 LRU：  [热: 策略代码页] ← 被 20 万个一次性的页从尾部挤出去
  双链表：  一次性页进 inactive 尾部，访问第二次才升 active
           → 扫描一遍不会污染 active 里的真热数据
```

**核心规则只有两条：**

| 规则 | 说明 |
|------|------|
| 页在 inactive 被**再次访问** | 提升到 active（"二次机会"） |
| 回收只从 **inactive 尾部**取 | 干净 file 页优先（零代价），脏页要等写回 |

> **这里的"访问"怎么检测？** 靠**页表项的 Accessed 位**：回收扫描时清位，过一段时间再看谁被重新置位（年轻位）。
> 这也是为什么老内核的 swap 行为在 NUMA 上很微妙——扫描和置位可能跨节点。

---

#### MGLRU：可选的第三代（v6.1+）

| 项目 | 传统 LRU | Multi-Gen LRU |
|------|---------|---------------|
| 配置 | 默认 | `CONFIG_LRU_GEN`（`mm/Kconfig:1220`，**无 `default y`**） |
| 顺序维护 | 靠链表搬移 | 靠**世代编号**（folio 上记 generation，不搬链表） |
| 热点判定 | Accessed 位 + 扫描 | 世代 + **多次访问加权**，抗"只摸一次"的干扰 |
| 开关 | — | `/sys/kernel/mm/lru_gen/enabled`（需 `CONFIG_LRU_GEN_ENABLED`，`mm/Kconfig:1229`） |

> **现实提醒：** 主流 x86 发行版内核多数**没有打开** MGLRU（Kconfig 里 LRU_GEN 不带 default y），
> 它主要活跃在 Android 和部分云厂商的定制内核上。排障前先 `cat /sys/kernel/mm/lru_gen/enabled` 确认你在哪一套上。

---

**HFT 视角：**

| 手段 | 作用 |
|------|------|
| **`mlockall(MCL_CURRENT\|MCL_FUTURE)`** | 把策略进程整个地址空间钉死，页直接进 `LRU_UNEVICTABLE`，永不参与回收 |
| **`mlock` / `mlock2` 关键区** | 只钉行情解码表、订单簿这类最热的几 MB |
| **`madvise(MADV_DONTNEED)`** | 主动丢弃一次性大块数据（回放完的历史行情），避免把热页挤出 active |
| **`echo 3 > drop_caches`** | 只在**维护窗口**用——它会清空整个页缓存，紧接着必然是缓存冷启动的批量 IO |
| **cgroup `memory.low`** | 给交易进程留保护水位，让回收录入优先从同 cgroup 的日志/回放进程身上拿 |

→ **Ch 12** 物理页回收 · [Ch 17 页/dcache 回收 slab 视角](../../chapter-17-devices-modules/) · [06.6 Ch7 内存调优](../../../06.6-systems-performance/chapter-07-memory/notes/section-7.6-调优指南.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Linux 页面回收的 LRU 双链表策略是什么？

<details><summary>答案</summary>

active_list（活跃页）和 inactive_list（不活跃页）。新页进 inactive 尾部。被再次访问时从 inactive 提升到 active。回收从 inactive 尾部开始。这比单链表 LRU 更好：防止「一次扫描污染」（如 find / 扫描大量文件后不立即驱逐活跃数据）。HFT 用 mlock 锁页不参与回收。

</details>

**Q2.** 现代内核的 LRU 链表挂在哪里？为什么比 LKD 描述的多出三条？

<details><summary>答案</summary>

挂在 **`struct lruvec`** 上（`include/linux/mmzone.h`），而 `lruvec` 是**每个 NUMA 节点、每个 memcg 各一份**，不是每个文件一份。

链表从 2 条变成 5 条（`enum lru_list`）：
- `LRU_INACTIVE_ANON` / `LRU_ACTIVE_ANON` —— 匿名页（进程堆栈、malloc 的堆）
- `LRU_INACTIVE_FILE` / `LRU_ACTIVE_FILE` —— 文件页（**页缓存住在这里**）
- `LRU_UNEVICTABLE` —— `mlock` 钉住的页，回收扫描直接跳过

多出来的三条各有用处：
1. **anon / file 分开**：回收代价不同（file 干净页可零代价丢弃，anon 必须写 swap），`lruvec` 用 `anon_cost`/`file_cost` 动态决定"这轮多扫哪边"；
2. **UNEVICTABLE 独立**：钉住的页不必每次扫描都被检查一遍，直接整链表跳过。

这个拆分同时解决了 NUMA 扩展性——每个节点有自己的 `lru_lock`，多节点并行回收时不再抢同一把全局锁。

</details>

**Q3.** HFT 策略进程已经 `mlockall` 了，为什么有时候还是观测到缺页延迟尖刺？

<details><summary>答案</summary>

`mlockall` 保证的是**页不会被回收出内存**，但不等于"不会发生缺页"。几种常见漏网：

1. **mlockall 之后才映射的内存**没被覆盖。`mlockall(MCL_FUTURE)` 才能把未来映射也钉住，只给 `MCL_CURRENT` 的话，之后 `mmap` 的历史行情文件、`dlopen` 的库、新 `malloc` 的堆都还躺在回收候选中。
2. **文件页被 `truncate`/`ftruncate` 或 `invalidate`** 时会被强制踢出页缓存，mlock 管不住别的进程动文件。
3. **NUMA 平衡（numa balancing）**会跨节点迁移页，迁移瞬间目标节点要重新建页表，产生一次可观测的停顿。策略进程一般应该 `numactl --membind` + 关闭 `kernel.numa_balancing`。
4. **THP 在缺页时的压缩/分配**：透明大页缺页路径要走一次 2MB 连续内存分配，内存碎片化时会触发 compaction（直接回收+搬迁），这是毫秒级的。HFT 常用 `madvise(MADV_HUGEPAGE)` 预铺好，或直接关 THP 改用 4K 页求确定性。

验证手段：`perf record -e page-faults,major-faults -p <pid>` 看是否还有 major fault（需要 IO 的缺页）；`/proc/<pid>/numa_maps` 看页是否真的都在本地节点。

</details>

</details>
---
