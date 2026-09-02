## ⑥ 争用和可扩展性 · Contention and Scalability

> 承接 [9.5 死锁](./section-9.5-死锁.md) 与 [Ch 10 同步方法](../../chapter-10-sync-methods/)。
> 本节回答：**争用的真实成本是什么、内核用什么机制对抗它、以及怎么测量。**

#### 锁争用（lock contention）

| 定义 | 影响 |
|------|------|
| 锁 **已被占用**，其他线程 **排队/自旋** 等待 | **瓶颈** — CPU 空转或睡眠唤醒开销 |

**HFT：** `perf lock`、延迟尖刺 — **高争用 mutex** 在热路径上是 **P99 杀手**。

---

### 一、⭐ 争用不是"一个"问题，是三个层次

| 层次 | 现象 | 成本 |
|------|------|------|
| **无争用** | 一次原子 RMW 操作拿到锁 | ⭐ 只有原子指令本身的开销（~20 周期量级） |
| **有争用但不阻塞** | 自旋几次就拿到 | 自旋开销 + 可能的 cacheline 传输 |
| ⭐ **真正有害的争用** | 多个 CPU 长时间抢同一把锁 | ⭐ **cacheline bouncing** + 流水线停顿 + 公平性尾延迟 |

> ⭐ 关键区分：**"锁被频繁获取"（acquisitions 高）不等于"有争用"**（contentions 高）。
> 一把每秒被获取 100 万次、但从不冲突的锁，扩展性完全没问题。
> 只有 `contentions` 指标上升才是真问题——这正是 §7 要讲的测量方法。

#### Amdahl 定律的锁版本

设串行部分占比为 `s`（锁保护的临界区）：

```
  加速比 = 1 / (s + (1-s)/N)

  N → ∞ 时，加速比上限 = 1/s
```

| 串行占比 `s` | N=8 加速比 | N→∞ 上限 |
|-------------|-----------|---------|
| 5% | 5.9× | **20×** |
| 10% | 4.7× | **10×** |
| ⭐ 20% | 3.3× | **5×** |
| 50% | 1.8× | **2×** |

> ⭐ **残酷的事实**：即使只有 **5%** 的代码在全局锁里，加 CPU 的上限就是 **20 倍**——而且这还是乐观模型（没算争用本身恶化的部分）。

#### ⭐ 现实比 Amdahl 更糟：争用会随 N 恶化

Amdahl 假设 `s` 是常数。但锁争用下 **N 越大、等待时间越长**，`s` 会**随 N 增长**：

```
吞吐
  │        ╱╲
  │       ╱  ╲___  ← 争用区：N 越大，等待开销越大，吞吐反而下降
  │      ╱
  │     ╱
  │    ╱
  │   ╱
  │  ╱
  └──────────────► CPU 数 N
     ↑
  拐点：cacheline 开始 bouncing
```

---

### 二、⭐ 争用的真实成本：cacheline bouncing

自旋锁是一个**共享变量**。每个 CPU 自旋时都在 `LOAD` 它 → 缓存一致性协议（MESI）要保证所有 CPU 看到的值一致。

```
CPU0 ──┐
CPU1 ──┤
CPU2 ──┼──► 同一个 cacheline（锁变量）
CPU3 ──┤     MESI: Modified/Exclusive/Shared/Invalid
...    ─┘
```

| 事件 | 后果 |
|------|------|
| 锁持有者释放（写） | 该 cacheline 在所有其他 CPU 的副本被**置为 Invalid** |
| N-1 个等待者同时读 | ⭐ 全部 cache miss → 同时发起总线请求 → **N-1 次传输** |
| 抢锁（原子 RMW） | 该 cacheline 被"抢"到发起者，其他再从 Invalid 重建 |

> ⭐ **这就是"增加 CPU 反而降低吞吐"的物理原因**：N 越大，一次锁释放引发的 cacheline 传输次数越多（O(N) 甚至 O(N²)），而有效工作仍是 O(1)。

#### ⭐ false sharing（伪共享）—— 更隐蔽的杀手

两个**逻辑上无关**的变量碰巧落在**同一个 cacheline**（64 字节）里，即使它们被不同 CPU 访问、各自有各自的锁，也会互相引发失效。

```
 cacheline (64B)
┌──────────────────────────────────────┐
│ counter_A (CPU0 独占写) │ counter_B (CPU1 独占写) │
└──────────────────────────────────────┘
        ↑ 无关，但共享一条 cacheline → 互相失效
```

**内核的解法**：`____cacheline_aligned_in_smp`

```c
struct foo {
	spinlock_t	lock;
	int		counter;
} ____cacheline_aligned_in_smp;
```

| 宏 | 作用 |
|----|------|
| `____cacheline_aligned_in_smp` | SMP 下按 cacheline 对齐，UP 下不浪费空间 |
| `__cacheline_aligned` | 无条件对齐 |
| `___cacheline_aligned_in_smp` | 对齐 + 归入 `.data..cacheline_aligned` 段 |

> ⭐ 内核里到处能看到这个后缀（`futex_hash_bucket`、`bit_wait_table`、`zone->lock`…），原因就是 false sharing 在大规模机器上是**实测可见的**性能差异。

---

### 三、⭐ 硬件的回应：qspinlock（v4.2）—— 排队自旋锁

ticket spinlock 已经解决了公平性（先来先到），但**所有人仍然自旋在同一个变量上** → bouncing 依旧。

qspinlock（基于 MCS 锁）的核心改进：**让第 3 个及以后的等待者，各自自旋在自己的 per-CPU 节点上。**

#### 32 位字段布局（v6.6 `include/asm-generic/qspinlock_types.h`）

```c
typedef struct qspinlock {
	union {
		atomic_t val;
#ifdef __LITTLE_ENDIAN
		struct {
			u8	locked;
			u8	pending;
		};
		struct {
			u16	locked_pending;
			u16	tail;
		};
#endif
	};
} arch_spinlock_t;
```

```
NR_CPUS < 16K 时：

 31                    18 17 16 15   9  8  7        0
┌──────────────────────┬─────┬────────┬─┬───────────┐
│   tail cpu (+1)      │idx  │ 未用   │P│  locked   │
└──────────────────────┴─────┴────────┴─┴───────────┘
                        │              │      └─ 0-7  锁是否被持有
                        │              └──────── 8    pending（第 2 个等待者）
                        └─────────────────────── 16-17 tail index（0-3）

NR_CPUS >= 16K 时：tail idx 缩到 9-10，tail cpu 占 11-31。
```

#### ⭐⭐ 三层结构：为什么需要 `pending`

| 层 | 字段 | 谁 | 自旋在哪 |
|----|------|-----|---------|
| 1 | `locked` (0-7) | 当前持有者 | — |
| 2 | ⭐ `pending` (bit 8) | **第 2 个**等待者 | ⭐ **锁变量本身** |
| 3 | `tail` (16-31) | 第 3 个及以后 | ⭐ **自己的 per-CPU MCS 节点** |

> ⭐⭐ **`pending` 是 qspinlock 的独创优化**：
> MCS 队列的"手递手"（handoff）有额外开销（要设置下一个节点的 `locked`、要读 tail）。
> 当**只有 2 个竞争者**时（这是绝大多数锁的实际情况！），用 `pending` 位让第二个人直接在锁变量上自旋，**完全跳过 MCS 队列路径**。
>
> 只有真的出现第 3 个竞争时，才进入完整的 MCS 队列。

#### ⭐ 每 CPU 有 4 个 MCS 节点槽位

```c
#define _Q_TAIL_IDX_BITS	2
```

`_Q_TAIL_IDX_BITS = 2` → 索引 0~3 → **每个 CPU 有 4 个 MCS 节点**。为什么是 4？因为同一 CPU 可能在不同上下文嵌套等待不同的锁：

| 槽位 | 上下文 |
|------|--------|
| 0 | 任务（task）上下文 |
| 1 | softirq |
| 2 | hardirq |
| 3 | ⭐ NMI |

> 如果只有 1 个槽位，一个 CPU 在任务上下文等锁 A 时被中断、又在中断里等锁 B，就会踩自己的节点——**死锁**。

#### ⭐ 官方的劝告：别急着用 qspinlock

v6.6 `include/asm-generic/qspinlock.h:5-9`：

```c
/*
 * A 'generic' spinlock implementation that is based on MCS locks. For an
 * architecture that's looking for a 'generic' spinlock, please first consider
 * ticket-lock.h and only come looking here when you've considered all the
 * constraints below and can show your hardware does actually perform better
 * with qspinlock.
 */
```

⭐ **官方明说：架构选型时应先考虑 ticket-lock，只有在实测证明 qspinlock 更好时才用它。**

**三条硬件依赖：**

| 依赖 | 说明 |
|------|------|
| ① **RCsc 内存模型** | `atomic_*_release()/acquire()` 必须是 RCsc（或 power 上至少 RCtso）——普通代码只要求 RCpc |
| ② **前进性保证** | qspinlock 用到的原子操作集合远大于普通 spinlock，每个都要有 forward progress；LL/SC 架构上的 `cmpxchg` 循环未必满足 |
| ③ ⭐ **混合尺寸原子操作** | 需要 `xchg16` 等 mixed-size atomics；LL/SC 架构要用 32 位 and+or 实现才能保前进性 |

> 源码还引用了一篇 POPL'17 论文讲混合尺寸原子操作：`http://www.cl.cam.ac.uk/~pes20/popl17/mixed-size.pdf`
>
> ⭐ **这解释了为什么 qspinlock 不是所有架构的默认**——x86 用它是因为硬件天然满足；弱内存序架构要付出额外代价。

#### 运行时检测争用

```c
static __always_inline int queued_spin_is_contended(struct qspinlock *lock)
{
	return atomic_read(&lock->val) & ~_Q_LOCKED_MASK;
}
```

⭐ **任何非 `locked` 位被设置**（即 `pending` 或 `tail` 非空）→ 有争用。这是**零成本的争用检测**，被 lockref 等优化路径使用。

**版本断崖**：qspinlock 引入于 **v4.2**（`include/asm-generic/qspinlock.h`：v4.1 = 14 字节 404 残片 / v4.2 = 4207 字节）。

---

### 四、⭐ 减少争用的四种武器

| 武器 | 做法 | 适用 | 代价 |
|------|------|------|------|
| ⭐ **per-CPU 化** | 每 CPU 一份副本，消灭共享 | 读多写少、可聚合的计数/缓存 | 需要聚合步骤；关抢占 |
| **分片（sharding）** | 一个结构拆 N 份，各一把锁 | 哈希表、对象池 | 跨片操作复杂 |
| ⭐ **RCU** | 读侧完全无锁 | 读极多写极少 | 写侧要复制 + 延迟回收 |
| **无锁（lock-free）** | CAS 循环 / ring buffer | 特定模式（SPSC、MPMC 队列） | 正确性难证；ABA 问题 |

| 并发形状 | 首选武器 |
|---------|---------|
| 单 CPU 独占 | per-CPU |
| SPSC 跨上下文 | ⭐ 无锁环（kfifo） |
| MPMC + 批量收割 | `llist` |
| 读极多写极少 | ⭐ RCU |
| 多写者有序键 | 分片 + 细粒度锁 |
| ⭐ 全局引用计数 | ⭐ `percpu_ref` |

---

### 五、⭐ `percpu_ref`：把全局计数器 per-CPU 化

一个被所有 CPU 频繁 `get/put` 的引用计数，是最经典的争用源。内核的解法是 `percpu_ref`。

v6.6 `include/linux/percpu-refcount.h:1-48`（作者 Kent Overstreet, Google, 2012）：

```c
/*
 * This implements a refcount with similar semantics to atomic_t - atomic_inc(),
 * atomic_dec_and_test() - but percpu.
 *
 * There's one important difference between percpu refs and normal atomic_t
 * refcounts; you have to keep track of your initial refcount, and then when you
 * start shutting down you call percpu_ref_kill() _before_ dropping the initial
 * refcount.
 *
 * The refcount will have a range of 0 to ((1U << 31) - 1), i.e. one bit less
 * than an atomic_t - this is because of the way shutdown works, see
 * percpu_ref_kill()/PERCPU_COUNT_BIAS.
 *
 * Before you call percpu_ref_kill(), percpu_ref_put() does not check for the
 * refcount hitting 0 - it can't, if it was in percpu mode. percpu_ref_kill()
 * puts the ref back in single atomic_t mode, collecting the per cpu refs and
 * issuing the appropriate barriers, and then marks the ref as shutting down so
 * that percpu_ref_put() will check for the ref hitting 0.  After it returns,
 * it's safe to drop the initial ref.
 * ...
 * Note that the free path, free_ioctx(), needs to go through explicit call_rcu()
 * to synchronize with RCU protected lookup_ioctx().  percpu_ref operations don't
 * imply RCU grace periods of any kind and if a user wants to combine percpu_ref
 * with RCU protection, it must be done explicitly.
 */
```

#### ⭐ 两阶段关闭协议（与普通 refcount 最大的不同）

```
  正常期（percpu 模式）           关闭期（atomic 模式）
  ┌────────────────────┐         ┌────────────────────┐
  │ CPU0: count[0]     │         │ 单一 atomic_t      │
  │ CPU1: count[1]     │  kill   │ （收集所有 percpu  │
  │ CPU2: count[2]     │ ──────► │  计数 + 屏障）     │
  │ ...                │         │                    │
  │ put 不检查 0       │         │ put 开始检查 0     │
  └────────────────────┘         └────────────────────┘
                                          │
                                          ▼
                                  可以 drop 初始引用
```

| 规则 | 说明 |
|------|------|
| ⭐ **必须先 `percpu_ref_kill()`** | 然后才能 drop 初始引用 |
| ⭐ **kill 之前 `put` 不检查 0** | 在 percpu 模式下**无法**检查（每个 CPU 有自己的计数） |
| ⭐ **计数范围少一位** | `0 ~ ((1U << 31) - 1)`，因为 shutdown 机制占用一位（`PERCPU_COUNT_BIAS`） |
| ⭐ **`percpu_ref` 不隐含 RCU** | 原文："percpu_ref operations don't imply RCU grace periods of any kind"，需要 RCU 时必须**显式** `call_rcu()` |
| `kill()` 自带 once 语义 | 返回 true 一次，之后返回 false |

> ⭐⭐ **"kill 之前 put 不检查 0"这条最反直觉**：它意味着在 percpu 模式下，引用计数**可能已经被减到 0 但对象不能释放**。这正是"用协议换性能"的代价——把"何时能释放"的判断从热路径挪到了关闭路径。

**典型使用者**：aio 的 `struct kioctx`（`fs/aio.c`）、`bdi_writeback`、`io_uring` 的部分结构。

---

### 六、⭐ 案例：`mmap_sem` 的三十年战争

这是内核里最著名的可扩展性战役，时间线完整展示了"如何一步步打掉全局锁"。

| 时间 | 事件 |
|------|------|
| 早期 | `mm->mmap_sem` —— **一把全局读写信号量**保护整个进程地址空间 |
| ⭐ **v5.8** | 改名 `mmap_lock`（语义澄清：它不只保护 mmap） |
| ⭐ **v6.1** | VMA 存储从 rbtree+链表双簿记 → **maple tree**（RCU 安全读成为可能） |
| ⭐ **v6.4** | ⭐ **per-VMA lock**：缺页处理不再需要拿整个 `mmap_lock` |

v6.6 `include/linux/mm_types.h:549-551, 607`：

```c
struct vma_lock {
	struct rw_semaphore lock;
};

struct vm_area_struct {
	...
#ifdef CONFIG_PER_VMA_LOCK
	struct vma_lock *vm_lock;      /* ⭐ 指针，且条件编译 */
	...
#endif
};
```

#### ⭐ 三个设计细节

| 细节 | 说明 |
|------|------|
| ⭐ **`vm_lock` 是指针** | 大多数 VMA 永远不会被并发访问 → **按需分配**，省内存 |
| **条件编译** | `#ifdef CONFIG_PER_VMA_LOCK` —— 不需要时可完全关掉 |
| **依赖 maple tree** | ⭐ 必须先在 v6.1 换成 maple tree（RCU 安全读），per-VMA lock 才可能实现 |

> ⭐⭐ **顺序不能颠倒**：per-VMA lock 的前提是"能在 RCU 读侧安全地找到 VMA"。rbtree 做不到（rebalance 改结构），所以**必须先有 v6.1 的 maple tree，才有 v6.4 的 per-VMA lock**。
> 这就是 [6.6 讲的](../../chapter-06-kernel-data-structures/notes/section-6.6-选择合适的数据结构.md)"数据结构选型决定并发上限"的实证。

**版本断崖实测**：

| 特性 | 版本 | 判据 |
|------|------|------|
| `mmap_sem` → `mmap_lock` | **v5.8** | `mm_types.h` 中 v5.7 只有 `mmap_sem`(2) / v5.8 只有 `mmap_lock`(2) |
| maple tree 管 VMA | **v6.1** | `include/linux/maple_tree.h` v6.0 = 14B 残片 / v6.1 = 23073B |
| ⭐ per-VMA lock | **v6.4** | `mm.h` 中 `lock_vma_under_rcu`/`vma_start_read`：v6.3 = 0 / v6.4 = 3 |

---

### 七、⭐ 怎么测量：`perf lock` 与 `/proc/lock_stat`

#### `/proc/lock_stat` 的 12 个字段（v6.6 `kernel/locking/lockdep_proc.c:570-592`）

```
lock_stat version 0.4

class name | con-bounces | contentions | waittime-min | waittime-max |
waittime-total | waittime-avg | acq-bounces | acquisitions |
holdtime-min | holdtime-max | holdtime-total | holdtime-avg
```

| 字段 | 含义 | 关注点 |
|------|------|--------|
| ⭐ **`contentions`** | 需要等待的获取次数 | ⭐ **这是"争用"的直接指标** |
| `con-bounces` | 因争用导致的 cacheline bounce 次数 | 反映 bouncing 严重度 |
| `waittime-min/max/total/avg` | 等待时间的分布 | ⭐ **`waittime-max` 是尾延迟的直接证据** |
| `acquisitions` | 总获取次数 | 高 ≠ 有争用 |
| `acq-bounces` | 获取时的 bounce 次数 | |
| `holdtime-min/max/total/avg` | 持有时间分布 | ⭐ 缩短它 = 减临界区 |

> ⭐ **最该看的两个**：`contentions`（有没有争用）和 **`waittime-max`**（最坏情况多惨）。
> 对 HFT 来说，**`waittime-max` 比 `waittime-avg` 重要得多**——P99 杀手藏在 max 里。

#### ⭐ 统计本身不能成为争用源

v6.6 `kernel/locking/lock_events.h:33-45`：

```c
/*
 * Increment the statistical counters. use raw_cpu_inc() because of lower
 * overhead and we don't care if we loose the occasional update.
 */
static inline void __lockevent_inc(enum lock_events event, bool cond)
{
	if (cond)
		raw_cpu_inc(lockevents[event]);
}
```

| 设计 | 理由 |
|------|------|
| ⭐ **`raw_cpu_inc()`** | per-CPU 裸递增，无原子、无屏障 |
| ⭐ **"we don't care if we loose the occasional update"** | ⭐ 允许偶尔丢更新 —— **用统计精度换"测量零开销"** |
| **未配置时是空宏** | `#else` 分支里 `#define lockevent_inc(ev)` 什么都不做 |

> ⭐⭐ **这条注释是"可扩展性"主题下最重要的设计哲学之一**：
> **观测手段本身不能改变被观测对象的行为，也不能成为新的瓶颈。**
> 一个用全局原子计数器的锁统计，会在 NUMA 机器上自己变成争用热点——所以内核选择"允许丢计数的 per-CPU 裸计数"。

#### 常用工具

| 工具 | 用途 | 开销 |
|------|------|------|
| `perf lock record` + `perf lock report` | 记录并报告锁的等待/持有时间 | 中（tracepoint） |
| `/proc/lock_stat` | lockdep 统计（12 个字段） | 依赖 `CONFIG_LOCK_STAT` |
| ⭐ `bpftrace -e 'tracepoint:lock:lock_acquire {...}'` | 自定义聚合 | ⭐ 低（内核侧聚合） |
| `perf c2c` | ⭐ **专门查 false sharing / cacheline bouncing** | 中高（采样 HITM） |

> ⭐ **`perf c2c` 是查 false sharing 的专用工具**（HITM = Hit In The Modified，即"读到了别人改过的行"）。排查"CPU 越多越慢"的问题时，它比 `perf lock` 更直接。

---

### 八、锁粒度（granularity）

| 粒度 | 争用高时 | 争用低时 |
|------|----------|----------|
| **粗** | 扩展差 | 实现简单 |
| **细** | 扩展好 | 开销可能偏大 |

| 维度 | 粗粒度锁 | 细粒度锁 |
|------|---------|---------|
| 锁数量 | 1 | N |
| 无争用开销 | ⭐ 小（一次原子操作） | 大（可能要拿多把锁） |
| 扩展性 | ❌ O(N) bouncing | ⭐ 好 |
| 死锁风险 | 低 | ⭐ **高**（多锁顺序问题，靠 lockdep 检查） |
| 内存 | 小 | 大（每把锁 + 可能的 padding） |
| 典型 | 早期 `mmap_sem` | ⭐ per-VMA lock |

#### ⭐ 中间路线：分层 / 分片

| 模式 | 做法 | 例子 |
|------|------|------|
| **快速路径 + 慢速路径** | 无争用时走无锁快路径，争用才上锁 | ⭐ qspinlock 的 `pending`、lockref |
| **读侧无锁（RCU）** | 读不加锁，写持锁 | dentry 查找 |
| **分片** | `hash(key) % N` 选锁 | ⭐ `futex_hash_bucket`（256 桶，各一把锁） |
| ⭐ **per-CPU + 惰性聚合** | 每 CPU 本地操作，定期汇总 | ⭐ `percpu_ref` |

---

### 九、工程建议（作者）

```
从简单锁开始 ──►  profiling 见争用 ──► 再细化粒度 / per-CPU / RCU
```

| 阶段 | 做法 |
|------|------|
| 初版 | **一把锁护结构** — 正确优先 |
| 优化 | 仅在有 **实测争用** 时拆锁、减临界区 |

> ⭐ **v6.6 qspinlock 的头部注释是这条建议的最好背书**：官方明确要求架构维护者"先考虑 ticket-lock，只有**实测证明** qspinlock 更好时才用"。
>
> 内核开发者的共识是：**没有 profile 数据的优化 = 猜测**。细粒度锁带来的死锁风险和复杂度是真实成本，只有在 contention 指标证实瓶颈时才值得付。

---

### 十、HFT 视角

| 内核机制 | 用户态对应 | HFT 关注点 |
|---------|-----------|-----------|
| cacheline bouncing | `std::atomic` 共享计数器 | ⭐ 热路径上任何被多线程写的共享变量都是 P99 杀手 |
| false sharing | 两个无关原子变量相邻 | ⭐ 用 `alignas(64)` 或填充到 64 字节 |
| qspinlock 的 MCS 队列 | `pthread_spinlock` 的公平性 | 公平锁降低尾延迟，但吞吐可能下降 |
| ⭐ `percpu_ref` | 全局引用计数 | ⭐ 改成 per-thread 计数 + 定期聚合 |
| ⭐ per-VMA lock | 全局锁 → 细粒度 | ⭐ "数据结构选型决定并发上限" |
| `waittime-max` | 锁等待的最大值 | ⭐ **盯 max，不要盯 avg** |

**实操四条：**

1. ⭐ **先测量再优化**。用 `perf lock report` 看 `contentions` 和 `waittime-max`，用 `perf c2c` 查 false sharing。别猜。
2. ⭐ **盯尾不盯均**。平均等待 1μs 但 max 5ms 的锁，对 HFT 来说是不可接受的——那 5ms 就是一次 P99.99 尖刺。
3. **消灭共享优先于优化锁**。per-thread 副本 + 聚合，比任何"更快的锁"都有效。
4. **热路径上的原子变量单独占 cacheline**。`alignas(64)` 或前后各填 64 字节。

---

→ **Ch 10** spinlock、mutex、seqlock、RCU 选型 · [Ch 10.2 自旋锁](../../chapter-10-sync-methods/notes/section-10.2-自旋锁.md) · [Ch 6.6 数据结构选型](../../chapter-06-kernel-data-structures/notes/section-6.6-选择合适的数据结构.md)

### 常见陷阱

1. 以为锁的正确性就够了——锁的争用程度直接影响性能和可扩展性
2. 混淆锁持有时间和锁等待时间——持有时间是「锁住了多久」，等待时间是「等了多久才拿到」
3. 以为增加 CPU 数量总能提升性能——锁争用下，增加 CPU 反而降低吞吐（锁竞争恶化）
4. ⭐ 以为"acquisitions 高"等于"有争用"——要看 **`contentions`**，一把被频繁获取但从不冲突的锁没问题
5. ⭐ 以为平均等待时间够看——**HFT 要盯 `waittime-max`**，尖刺藏在最大值里
6. ⭐ 以为 false sharing 只是理论问题——`perf c2c` 在真机上经常能查到，且影响可达数量级
7. 以为 qspinlock 是"更先进的 spinlock，所有架构都该用"——⭐ **官方注释要求先考虑 ticket-lock**，qspinlock 对硬件有三条额外要求（RCsc、前进性、混合尺寸原子）
8. 以为细粒度锁总是更好——它带来死锁风险和内存开销，**只有实测有争用时才值得**（内核官方建议）
9. ⭐ 以为 `percpu_ref` 只是"per-CPU 的 atomic_t"——它有**两阶段关闭协议**（必须先 kill 再 drop 初始引用），且 kill 前 `put` 不检查 0

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 锁争用（contention）怎么测量？

<details><summary>答案</summary>

内核：① `perf lock record` + `perf lock report`：记录锁等待时间和持有时间。② `/proc/lock_stat`：lockdep 统计。③ `bpftrace -e 'tracepoint:lock:lock_acquire { ... }'`：追踪锁获取。用户态：① `perf lock`。② `Valgrind --tool=drd`。③ `pthread_mutex` 的 `trylock` 探测。指标：con-<N>（等待次数）、wait-total（总等待时间）、hold-total（总持有时间）。

<details><summary>按 v6.6 修订/补充</summary>

**字段名需要修正**——v6.6 `/proc/lock_stat` 实际有 **12 个字段**（`kernel/locking/lockdep_proc.c:570-592`）：

```
lock_stat version 0.4

class name | con-bounces | contentions |
waittime-min | waittime-max | waittime-total | waittime-avg |
acq-bounces | acquisitions |
holdtime-min | holdtime-max | holdtime-total | holdtime-avg
```

| 字段 | 含义 | 关注点 |
|------|------|--------|
| ⭐ `contentions` | 需要等待的获取次数 | ⭐ **争用的直接指标** |
| `con-bounces` | 争用导致的 cacheline bounce 次数 | bouncing 严重度 |
| `waittime-min/max/total/avg` | 等待时间分布 | ⭐ **`waittime-max` 是尾延迟证据** |
| `acquisitions` | 总获取次数 | ⭐ 高 ≠ 有争用 |
| `acq-bounces` | 获取时 bounce 次数 | |
| `holdtime-min/max/total/avg` | 持有时间分布 | 缩短它 = 减临界区 |

（原答案写的 `con-<N>` / `wait-total` / `hold-total` 不是 v6.6 的字段名，实际是 `waittime-total` / `holdtime-total`。）

**补充两个工具：**

| 工具 | 用途 |
|------|------|
| ⭐ **`perf c2c`** | 专门查 false sharing / cacheline bouncing（HITM = Hit In The Modified）。排查"CPU 越多越慢"时比 `perf lock` 更直接 |
| `perf bench` | 微基准 |

**⭐ 还有一个必须知道的设计细节**——统计本身不能成为争用源（`kernel/locking/lock_events.h:33-45`）：

```c
/*
 * Increment the statistical counters. use raw_cpu_inc() because of lower
 * overhead and we don't care if we loose the occasional update.
 */
static inline void __lockevent_inc(enum lock_events event, bool cond)
{
	if (cond)
		raw_cpu_inc(lockevents[event]);
}
```

内核**故意**用 per-CPU 裸计数（无原子、无屏障）并**容忍偶尔丢更新**，因为用全局原子计数器做锁统计，自己会在 NUMA 机器上变成争用热点。`CONFIG_LOCK_EVENT_COUNTS` 未开时这些是空宏。

</details>
</details>

**Q2.** 为什么增加 CPU 在锁争用下反而降低性能？

<details><summary>答案</summary>

假设一把全局锁保护共享数据。N 个 CPU 同时请求锁：① 只有 1 个拿到，其余 N-1 个 spin 等待。② N 越大，spin 浪费的 CPU 越多。③ 锁释放时 N-1 个 CPU 抢锁 → cache line bouncing。④ 吞吐随 N 增加先升后降（Amdahl's Law 的锁版本）。解决：① 减小临界区。② per-CPU 数据。③ 无锁数据结构（RCU）。

<details><summary>按 v6.6 补充（物理机制 + qspinlock 的解法）</summary>

答案的机制描述是对的，补充**物理层面的原因**和**内核的解法**：

**cacheline bouncing 的具体过程：**

| 事件 | 后果 |
|------|------|
| 锁持有者释放（写锁变量） | 该 cacheline 在所有其他 CPU 的副本被置为 **Invalid** |
| N-1 个等待者同时读 | ⭐ 全部 cache miss → 同时发起总线请求 → **O(N) 次传输** |
| 抢锁（原子 RMW） | cacheline 被"抢"到发起者，其他人再从 Invalid 重建 |

有效工作仍是 O(1)，但**协调开销是 O(N) 甚至 O(N²)** → N 越大越亏。

**Amdahl 还低估了问题**：它假设串行占比 `s` 是常数。但争用下 **N 越大、等待越久，`s` 会随 N 增长**。

| 串行占比 `s` | N=8 加速比 | N→∞ 上限 |
|-------------|-----------|---------|
| 5% | 5.9× | 20× |
| 10% | 4.7× | 10× |
| ⭐ 20% | 3.3× | 5× |

**⭐ 内核的解法：qspinlock（v4.2）**

ticket spinlock 解决了公平性，但所有人仍自旋在同一个变量上。qspinlock 让**第 3 个及以后的等待者各自自旋在自己的 per-CPU MCS 节点上**：

| 层 | 字段 | 自旋在哪 |
|----|------|---------|
| 1 | `locked` (0-7) | —（持有者） |
| 2 | ⭐ `pending` (bit 8) | 锁变量本身（第 2 个等待者） |
| 3 | `tail` (16-31) | ⭐ 自己的 per-CPU MCS 节点 |

于是"一次释放 → N-1 次 cacheline 传输"变成"**一次释放 → 1 次定向传输给队列下一个**"。

**另外三种解法**：per-CPU 化（消灭共享）、RCU（读侧无锁）、分片（`futex_hash_bucket` 256 桶各一把锁）。

</details>
</details>

**Q3.** HFT 如何设计无锁/低争用数据结构？

<details><summary>答案</summary>

① SPSC 环形队列：单生产者单消费者，`atomic<head>` + `atomic<tail>` + release/acquire 序。② per-thread 缓存：每线程独立操作，定期聚合。③ RCU 模式：读端无锁（`atomic load` 指针），写端复制+替换+延迟回收。④ 分片锁：`sharded_hashmap`，N 个 bucket 各一把锁，减少争用。⑤ `std::shared_mutex`：多读单写，适合读多写少。

<details><summary>按 v6.6 补充（三条内核经验）</summary>

这个答案已经覆盖了主要模式，补三条从内核可以直接抄的经验：

**① ⭐ 警惕 false sharing——比锁本身更隐蔽**

两个逻辑上无关的变量若落在同一 cacheline（64B），即使各自有各自的锁也会互相失效。

```c
struct foo {
	spinlock_t	lock;
	int		counter;
} ____cacheline_aligned_in_smp;      /* ⭐ 内核的标准解法 */
```

用户态对应：`alignas(64)` 或手工填充。**用 `perf c2c` 验证**。

**② ⭐ 引用计数用 per-thread + 惰性聚合（对应 `percpu_ref`）**

内核的 `percpu_ref` 是把全局计数器 per-CPU 化的标准方案（aio 的 `kioctx`、`bdi_writeback`）。用户态平移：每线程一个 `long`，定期汇总。

⚠️ **但要注意它有协议代价**（热路径性能是用关闭路径的复杂度换来的）：

| 规则 | 说明 |
|------|------|
| ⭐ 必须先 `percpu_ref_kill()` | 然后才能 drop 初始引用 |
| ⭐ kill 之前 `put` **不检查 0** | percpu 模式下无法检查 |
| ⭐ 不隐含 RCU grace period | 需要 RCU 时必须显式 `call_rcu()` |
| 计数范围少一位 | `0 ~ ((1U<<31)-1)`，shutdown 机制占一位 |

**③ ⭐ 盯 `waittime-max` 而不是 `waittime-avg`**

`/proc/lock_stat` 同时给出 min/max/total/avg 四项。对 HFT 来说：

- 平均等待 1μs 但 **max 5ms** 的锁 = 一次 P99.99 尖刺
- **平均值好看不代表尾延迟可接受**

**④ 优先级：消灭共享 > 优化锁**

per-thread 副本 + 聚合，比任何"更快的锁"都有效。锁再快，也有 cacheline bouncing 的下限；没有共享，就没有 bouncing。

</details>
</details>

**Q4.** qspinlock 的 `pending` 位是什么？为什么需要它？

<details><summary>答案</summary>

v6.6 `include/asm-generic/qspinlock_types.h` 的 32 位布局（`NR_CPUS < 16K`）：

```
 31                    18 17 16 15   9  8  7        0
┌──────────────────────┬─────┬────────┬─┬───────────┐
│   tail cpu (+1)      │idx  │ 未用   │P│  locked   │
└──────────────────────┴─────┴────────┴─┴───────────┘
                        │              │      └─ 0-7  locked
                        │              └──────── 8    pending
                        └─────────────────────── 16-17 tail index (0-3)
```

| 层 | 字段 | 谁 | 自旋在哪 |
|----|------|-----|---------|
| 1 | `locked` (0-7) | 当前持有者 | — |
| 2 | ⭐ `pending` (bit 8) | **第 2 个**等待者 | ⭐ 锁变量本身 |
| 3 | `tail` (16-31) | 第 3 个及以后 | ⭐ 自己的 per-CPU MCS 节点 |

⭐⭐ **为什么需要 `pending`**：

MCS 队列的"手递手"（handoff）有额外开销——要设置下一个节点的 `locked` 字段、要读 tail。

而**绝大多数锁在实际运行中只有 2 个竞争者**。此时用 `pending` 位让第二个人直接在锁变量上自旋，**完全跳过 MCS 队列路径**。

只有真的出现第 3 个竞争者时，才进入完整的 MCS 队列。

> 这是典型的"**为常见情况做快路径优化**"：2 竞争者是最常见形态，为它单独设计一条路径。

**另一个细节：`_Q_TAIL_IDX_BITS = 2` → 每 CPU 4 个 MCS 节点槽位**

| 槽位 | 上下文 |
|------|--------|
| 0 | 任务（task） |
| 1 | softirq |
| 2 | hardirq |
| 3 | ⭐ NMI |

因为同一 CPU 可能在不同上下文嵌套等待不同的锁。若只有 1 个槽位，任务上下文等锁 A 时被中断、又在中断里等锁 B，就会踩自己的节点 → **死锁**。

**版本**：qspinlock 引入于 **v4.2**。

</details>

**Q5.** 官方为什么说架构选型时"先考虑 ticket-lock"？

<details><summary>答案</summary>

v6.6 `include/asm-generic/qspinlock.h:5-9` 原文：

```c
/*
 * A 'generic' spinlock implementation that is based on MCS locks. For an
 * architecture that's looking for a 'generic' spinlock, please first consider
 * ticket-lock.h and only come looking here when you've considered all the
 * constraints below and can show your hardware does actually perform better
 * with qspinlock.
 */
```

⭐ 即：**先考虑 ticket-lock，只有在实测证明 qspinlock 更好时才用它。**

**三条硬件依赖：**

| 依赖 | 说明 |
|------|------|
| ① **RCsc 内存模型** | `atomic_*_release()/acquire()` 必须是 RCsc（power 上至少 RCtso）—— 普通代码只要求 RCpc |
| ② **前进性保证（forward progress）** | qspinlock 用到的原子操作集合远大于普通 spinlock，每个都要保证前进性；LL/SC 架构上的 `cmpxchg()` 循环未必满足 |
| ③ ⭐ **混合尺寸原子操作（mixed-size atomics）** | 需要 `xchg16` 等；LL/SC 架构要用 32 位 and+or 实现才能保前进性 |

源码还引用了一篇 POPL'17 论文专门讨论这个问题：

> `http://www.cl.cam.ac.uk/~pes20/popl17/mixed-size.pdf`

⭐ **结论**：x86 用 qspinlock 是因为硬件天然满足这三条；**弱内存序架构要付出额外代价才能用**。所以它不是"所有架构的默认最优解"。

> ⭐ 这段注释也是"工程建议"一节的最佳背书：**没有 profile 数据的优化 = 猜测**。内核官方对"更先进"的锁都持这种态度。

</details>

**Q6.** `percpu_ref` 和普通 `atomic_t` 引用计数有什么关键区别？

<details><summary>答案</summary>

v6.6 `include/linux/percpu-refcount.h:1-48`（作者 Kent Overstreet, Google, 2012）。

⭐⭐ **最关键的区别：两阶段关闭协议。**

```
  正常期（percpu 模式）           关闭期（atomic 模式）
  ┌────────────────────┐         ┌────────────────────┐
  │ CPU0: count[0]     │         │ 单一 atomic_t      │
  │ CPU1: count[1]     │  kill   │ （收集所有 percpu  │
  │ CPU2: count[2]     │ ──────► │  计数 + 屏障）     │
  │ put 不检查 0       │         │ put 开始检查 0     │
  └────────────────────┘         └────────────────────┘
                                          │
                                          ▼
                                  可以 drop 初始引用
```

| 规则 | 说明 |
|------|------|
| ⭐ **必须先 `percpu_ref_kill()`** | 然后才能 drop 初始引用（源码注释："you have to keep track of your initial refcount, and then when you start shutting down you call `percpu_ref_kill()` _before_ dropping the initial refcount"） |
| ⭐ **kill 之前 `put` 不检查 0** | "it can't, if it was in percpu mode" —— 每个 CPU 有自己的计数，**无法**判断全局是否为 0 |
| ⭐ **计数范围少一位** | `0 ~ ((1U << 31) - 1)`，因为 shutdown 机制占用一位（`PERCPU_COUNT_BIAS`） |
| ⭐ **不隐含 RCU grace period** | "percpu_ref operations don't imply RCU grace periods of any kind"——需要 RCU 时必须**显式** `call_rcu()` |
| `kill()` 自带 once 语义 | 返回 true 一次，之后返回 false |

⭐⭐ **"kill 之前 put 不检查 0"最反直觉**：它意味着 percpu 模式下引用可能已减到 0 但对象**不能释放**。

这正是"**用协议换性能**"的代价：把"何时能释放"的判断从热路径（每次 put）挪到了关闭路径（kill 一次）。热路径上只剩一次 per-CPU 裸递增——无原子、无争用。

**典型使用者**：aio 的 `struct kioctx`（`fs/aio.c`）、`bdi_writeback`、io_uring 的部分结构。

</details>

**Q7.** 内核打掉 `mmap_sem` 这把全局锁用了哪几步？

<details><summary>答案</summary>

这是内核里最著名的可扩展性战役，时间线：

| 时间 | 事件 |
|------|------|
| 早期 | `mm->mmap_sem` —— 一把全局读写信号量保护整个进程地址空间 |
| ⭐ **v5.8** | 改名 `mmap_lock`（语义澄清：它不只保护 mmap） |
| ⭐ **v6.1** | VMA 存储从 rbtree+链表双簿记 → **maple tree**（RCU 安全读成为可能） |
| ⭐ **v6.4** | ⭐ **per-VMA lock**：缺页处理不再需要拿整个 `mmap_lock` |

v6.6 `include/linux/mm_types.h:549-551, 607`：

```c
struct vma_lock {
	struct rw_semaphore lock;
};

struct vm_area_struct {
	...
#ifdef CONFIG_PER_VMA_LOCK
	struct vma_lock *vm_lock;      /* ⭐ 指针，且条件编译 */
	...
#endif
};
```

**三个设计细节：**

| 细节 | 说明 |
|------|------|
| ⭐ `vm_lock` 是**指针** | 大多数 VMA 永远不会被并发访问 → **按需分配**，省内存 |
| 条件编译 | `#ifdef CONFIG_PER_VMA_LOCK` —— 不需要时可完全关掉 |
| 依赖 maple tree | ⭐ 必须先在 v6.1 换成 maple tree，per-VMA lock 才可能实现 |

⭐⭐ **顺序不能颠倒**：per-VMA lock 的前提是"能在 RCU 读侧安全地找到 VMA"。rbtree 做不到（rebalance 会改结构，RCU 读者可能读到正在旋转的节点），所以**必须先有 v6.1 的 maple tree，才有 v6.4 的 per-VMA lock**。

这是"**数据结构选型决定并发上限**"的最好实证——见 [6.6 §7](../../chapter-06-kernel-data-structures/notes/section-6.6-选择合适的数据结构.md)。

**版本断崖实测判据：**

| 特性 | 版本 | 判据 |
|------|------|------|
| `mmap_sem` → `mmap_lock` | **v5.8** | `mm_types.h`：v5.7 只有 `mmap_sem`(2) / v5.8 只有 `mmap_lock`(2) |
| maple tree 管 VMA | **v6.1** | `maple_tree.h`：v6.0 = 14B 残片 / v6.1 = 23073B |
| ⭐ per-VMA lock | **v6.4** | `mm.h` 中 `lock_vma_under_rcu`/`vma_start_read`：v6.3 = 0 / v6.4 = 3 |

</details>

**Q8.** false sharing 是什么？内核怎么解决？

<details><summary>答案</summary>

**false sharing（伪共享）**：两个**逻辑上无关**的变量碰巧落在**同一个 cacheline**（x86 上 64 字节）里。即使它们被不同 CPU 访问、各自有各自的锁，缓存一致性协议也会让它们互相失效。

```
 cacheline (64B)
┌──────────────────────────────────────┐
│ counter_A (CPU0 独占写) │ counter_B (CPU1 独占写) │
└──────────────────────────────────────┘
        ↑ 逻辑无关，但共享一条 cacheline → 互相 Invalid
```

CPU0 写 `counter_A` → CPU1 的整条 cacheline 失效 → CPU1 写 `counter_B` 要重新加载 → 反过来再让 CPU0 失效。**两个变量都不需要共享，却付了共享的代价。**

**内核的解法（`include/linux/cache.h`）：**

| 宏 | 作用 |
|----|------|
| `____cacheline_aligned_in_smp` | SMP 下按 cacheline 对齐；**UP 下不浪费空间** |
| `__cacheline_aligned` | 无条件对齐 |
| `___cacheline_aligned_in_smp` | 对齐 + 归入 `.data..cacheline_aligned` 段 |

```c
struct foo {
	spinlock_t	lock;
	int		counter;
} ____cacheline_aligned_in_smp;      /* ⭐ 常见写法 */
```

内核里到处能看到这个后缀——`futex_hash_bucket`、`bit_wait_table`、`zone->lock` 等等。原因就是 false sharing 在大规模机器上是**实测可见的**性能差异。

**用户态对应**：`alignas(64)`，或手工在成员间填充 64 字节。

**怎么验证**：⭐ **`perf c2c`** —— 专门查 cacheline bouncing，报告 **HITM**（Hit In The Modified，即"读到了别人刚改过的行"）。排查"CPU 越多越慢"时它比 `perf lock` 更直接。

</details>

**Q9.** 内核的锁统计为什么用 `raw_cpu_inc` 而不是原子操作？

<details><summary>答案</summary>

v6.6 `kernel/locking/lock_events.h:33-45`：

```c
/*
 * Increment the statistical counters. use raw_cpu_inc() because of lower
 * overhead and we don't care if we loose the occasional update.
 */
static inline void __lockevent_inc(enum lock_events event, bool cond)
{
	if (cond)
		raw_cpu_inc(lockevents[event]);
}
```

| 设计 | 理由 |
|------|------|
| ⭐ **`raw_cpu_inc()`** | per-CPU 裸递增，**无原子、无屏障** |
| ⭐ **"we don't care if we loose the occasional update"** | 明确容忍偶尔丢更新 |
| **未配置时是空宏** | `#else` 分支：`#define lockevent_inc(ev)`（什么都不做） |

⭐⭐ **核心理由**：**观测手段本身不能成为新的瓶颈。**

如果用全局原子计数器做锁统计：
- 每次锁获取都要一次全局原子 RMW
- 在 NUMA 机器上，这个计数器的 cacheline 会在所有节点间 bouncing
- **统计锁争用的工具，自己变成了最大的争用源**（典型的观测者效应）

所以内核选择：**per-CPU 裸计数（零争用）+ 容忍精度损失**。

⭐ **设计哲学**：统计的目的是发现"数量级级别"的问题（ contention 是不是瓶颈、max 等待是不是离谱），而不是精确到个位。**用精度换零开销是正确的取舍。**

</details>

**Q10.** 锁粒度应该怎么选？细粒度锁总是更好吗？

<details><summary>答案</summary>

⭐ **不是。细粒度锁有真实成本，只有实测有争用时才值得。**

| 维度 | 粗粒度锁 | 细粒度锁 |
|------|---------|---------|
| 锁数量 | 1 | N |
| 无争用开销 | ⭐ 小（一次原子操作） | 大（可能要拿多把锁） |
| 扩展性 | ❌ O(N) bouncing | ⭐ 好 |
| ⭐ 死锁风险 | 低 | ⭐ **高**（多锁顺序问题，靠 lockdep 检查） |
| 内存 | 小 | 大（每把锁 + 可能的 cacheline padding） |
| 典型 | 早期 `mmap_sem` | ⭐ per-VMA lock（v6.4） |

**中间路线（通常比"一味细化"更好）：**

| 模式 | 做法 | 例子 |
|------|------|------|
| 快速路径 + 慢速路径 | 无争用走无锁快路径，争用才上锁 | ⭐ qspinlock 的 `pending` 位、lockref |
| 读侧无锁（RCU） | 读不加锁，写持锁 | dentry 查找 |
| 分片 | `hash(key) % N` 选锁 | ⭐ `futex_hash_bucket`（256 桶各一把锁） |
| ⭐ per-CPU + 惰性聚合 | 每 CPU 本地操作，定期汇总 | ⭐ `percpu_ref` |

**内核官方的选型建议**（`qspinlock.h:5-9` 是同一精神的体现）：

```
从简单锁开始 ──► profiling 见争用 ──► 再细化粒度 / per-CPU / RCU
```

即：**先正确、后优化；优化必须有 profile 数据支撑。**

> ⭐ 一个反例值得记住：qspinlock 虽然是"更先进"的锁，官方注释却要求架构维护者**先考虑 ticket-lock**，只有在"can show your hardware does actually perform better"时才用。**内核对"先进性"本身是警惕的。**

</details>

</details>

---

→ [Ch 10 同步方法](../../chapter-10-sync-methods/) · [Ch 6.6 数据结构选型](../../chapter-06-kernel-data-structures/notes/section-6.6-选择合适的数据结构.md) · [Ch 4.4 休眠与唤醒](../../chapter-04-process-scheduling/notes/section-4.4-休眠与唤醒.md)
