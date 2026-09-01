## ⑩ 每个 CPU 的分配 · Per-CPU

SMP 下 **全局变量 + 锁** 保护计数器 → **缓存行 bouncing** — **per-CPU 数据** 给 **每个 CPU 一份副本**，本核 **通常无锁写**。

> **版本前提**：本节基于 **v6.6 源码实证**
> （`include/linux/percpu.h`、`include/linux/percpu-defs.h`、`include/linux/percpu_counter.h`、
> `arch/x86/include/asm/percpu.h`、`mm/percpu.c`、`lib/percpu_counter.c`）。

#### 动机

| 问题 | per-CPU 解法 |
|------|--------------|
| **`atomic_t` 热点** | 每核 **私有计数** — 周期性汇总 |
| **锁竞争** | 本 CPU **只写自己那份** |
| **false sharing** | 每副本 **独立 cache line**（`____cacheline_aligned_in_smp`） |
| **cache line bouncing** | 见下图 |

```
传统:                                  per-CPU:
  CPU0 ──┐                               CPU0 ──► counter[0]   （自己的 cache line）
  CPU1 ──┼──► [ global counter ]          CPU1 ──► counter[1]
  CPU2 ──┘     ← 同一 cache line 乒乓      CPU2 ──► counter[2]
  每次写：先独占该行（RFO），把其他核的副本作废
  代价：~100 个时钟周期的缓存一致性流量，且随核数恶化
```

#### ⭐ 三代访问原语（v6.6 实证，别再用 `__get_cpu_var`）

`include/linux/percpu-defs.h` 里其实是**三个族**，语义差别很关键：

| 族 | 抢占/中断保护 | 定位 |
|----|--------------|------|
| **`raw_cpu_read/write/add/...`** | **完全不检查** | 注释直说"do not want to do any checks for preemptions… **Unless strictly necessary, always use [__]this_cpu_*() instead**" |
| **`__this_cpu_*`** | **断言抢占已关**（`CONFIG_DEBUG_PREEMPT` 下会报） | 用于"我确定已经 `preempt_disable()` 了"的地方 |
| **`this_cpu_*`** | ✅ **隐含抢占/中断保护** | 注释："Operations with **implied preemption/interrupt protection**. These operations can be used **without worrying about preemption or interrupt**" —— **新代码默认用它** |

可用操作（`percpu-defs.h:487` 逐字列出）：

```
this_cpu_read / write / add / and / or / add_return / sub
this_cpu_xchg / cmpxchg / try_cmpxchg / cmpxchg_double
this_cpu_inc / dec / inc_return / dec_return
```

> ⚠️ **`__get_cpu_var()` 是这一族的"老写法"**（LKD 时代的接口）。
> 现代代码用 **`this_cpu_ptr()` / `this_cpu_*()`**，`get_cpu_var()/put_cpu_var()` 仍然可用
> （实现就是 `preempt_disable()` + `this_cpu_ptr()`）。
> 另外 `DEFINE_PER_CPU(type, name)` 有若干变体，用途各不相同：

| 变体 | 用途 |
|------|------|
| `DEFINE_PER_CPU(type, name)` | 普通静态 per-CPU 变量 |
| `DEFINE_PER_CPU_SHARED_ALIGNED` | 与 SMP 缓存行边界对齐（**防 false sharing**） |
| `DEFINE_PER_CPU_ALIGNED` | 按 `L1_CACHE_BYTES`/架构对齐要求对齐 |
| `DEFINE_PER_CPU_PAGE_ALIGNED` | 页对齐 |
| `DEFINE_PER_CPU_READ_MOSTLY` | 放 `.data..read_mostly` 段（**只读热数据**避免和写热数据共享 cache line） |
| `DEFINE_PER_CPU_FIRST` | 排在该段最前面（给 `current_task` 这类"必须极快"的变量） |

#### x86 上为什么 `this_cpu_*` 是一条指令

```c
/* arch/x86/include/asm/percpu.h:32 */
#define __my_cpu_offset		this_cpu_read(this_cpu_off)

/* :31 —— 用段前缀编译 percpu 变量访问 */
#define __percpu_prefix		"%%"__stringify(__percpu_seg)":"     /* "%%fs:" */
```

```
C 代码：  this_cpu_add(counter, 1);
汇编：    addq $1, %fs:counter          ← 一条指令，本 CPU 内原子
```

> **关键点**：x86 把 **本 CPU 的 percpu 基址** 放在 `this_cpu_off`（一个 per-CPU 变量）里，
> 变量访问靠 **`%fs` 段前缀** 直接寻址，所以 **percpu 访问 = 一条普通指令，无锁、无原子前缀**。
> 代价是它**只对"同一 CPU 上"的其他操作排他**（源码注释：
> "guarantee exclusivity of access for other operations on the **same** processor"）——
> **跨 CPU 的次序与可见性它一概不管**。

#### 接口

| 宏 / API | 作用 |
|----------|------|
| **`DEFINE_PER_CPU(type, name)`** | **静态** per-CPU 变量（编译期放进 `.data..percpu` 段） |
| **`per_cpu(var, cpu)`** | 访问**指定 CPU** 的静态变量实例 |
| **`this_cpu_ptr(ptr)` / `per_cpu_ptr(ptr, cpu)`** | 把 `__percpu` 指针转成**普通可解引用**指针 |
| **`get_cpu_ptr(ptr)` / `put_cpu_ptr(ptr)`** | `preempt_disable()` + `this_cpu_ptr()`，用于**遍历期间不能被迁移**的场景 |
| **`alloc_percpu(type)`** | **动态** 分配 per-CPU 对象（**可能睡眠**） |
| **`alloc_percpu_gfp(type, gfp)`** | 指定 gfp |
| **`free_percpu(ptr)`** | 释放 |

```c
/* 静态 */
DEFINE_PER_CPU(unsigned long, irq_count);

void inc_irq_count(void)
{
	this_cpu_inc(irq_count);        /* 一条指令，本 CPU 内原子 */
}
/* 读别的 CPU：per_cpu(irq_count, cpu)，但要保证那个 CPU 在线且值不会瞬变 */

/* 动态 */
struct stats __percpu *s = alloc_percpu(struct stats);
struct stats *local = get_cpu_ptr(s);   /* preempt_disable + 取本核指针 */
local->packets++;
put_cpu_ptr(s);                         /* preempt_enable */
free_percpu(s);
```

#### 分配器内部：chunk / unit / slot（v6.6 `mm/percpu.c` 实证）

```
pcpu_alloc() 管的三层结构

  ┌─ first chunk（启动时建好，含静态 DEFINE_PER_CPU 全部内容）
  │     ├─ static area：内核 + 模块的 per-CPU 变量
  │     ├─ reserved area：PERCPU_MODULE_RESERVE（8KB，给模块用）
  │     └─ dynamic area：PERCPU_DYNAMIC_RESERVE（64 位 28KB）
  │
  ├─ reserved chunk（可选，存在时专供 reserved 请求 —— slab 起来之前的关键分配）
  │
  └─ dynamic chunks（后续按需新建，vmalloc 区 backing）
        每个 chunk = nr_cpu_ids 个 unit（每 CPU 一份）+ 一张位图管槽位
```

**硬限制**（`mm/percpu.c:1752` 逐字）：

```c
	if (unlikely(!size || size > PCPU_MIN_UNIT_SIZE || align > PAGE_SIZE ||
		     !is_power_of_2(align))) {
		WARN(do_warn, "illegal size (%zu) or align (%zu) for percpu allocation\n",
		     size, align);
		return NULL;
	}
```

| 常量（`include/linux/percpu.h`） | 值 | 含义 |
|--------------------------------|-----|------|
| **`PCPU_MIN_UNIT_SIZE`** | `PFN_ALIGN(32 << 10)` = **32KB** | 最小 unit 大小，**同时也是单次 percpu 分配的**上限 |
| **`PCPU_MIN_ALLOC_SIZE`** | `1 << PCPU_MIN_ALLOC_SHIFT` = **4 B** | 最小分配粒度（内部碎片 ≤ 3 字节） |
| **`PERCPU_MODULE_RESERVE`** | **8 KB**（`CONFIG_MODULES`），否则 0 | 预留给模块的静态 per-CPU 变量 |
| **`PERCPU_DYNAMIC_RESERVE`** | **28 KB**（64 位）/ 20 KB（32 位） | 挂在 first chunk 上的动态区 |
| **`PERCPU_DYNAMIC_EARLY_SIZE`** | **20 KB** | slab 初始化**之前**要保证能供应的动态量 |

> ⚠️ **一处很容易被忽略的联动**（`percpu.h:36-40` 实证）：
> ```c
> #ifdef CONFIG_RANDOM_KMALLOC_CACHES
> #define PERCPU_DYNAMIC_SIZE_SHIFT      12      /* 不是 10！ */
> #else
> #define PERCPU_DYNAMIC_SIZE_SHIFT      10
> #endif
> ```
> 打开 12.7 讲的 `CONFIG_RANDOM_KMALLOC_CACHES`（v6.6 新增的反堆喷特性）后，
> `PERCPU_DYNAMIC_RESERVE` 从 **28KB 变成 112KB**（`28 << 12`）——
> **因为每 CPU 要维护的 kmalloc cache 状态变多了**。
> 这是个"安全特性 → 静态内存占用"的连锁反应，做内存预算时要算进去。

#### ⚠️ `alloc_percpu()` 会睡眠 —— 但有例外

```c
/* mm/percpu.c:1736 —— gfp 白名单与 atomic 判定（逐字） */
	gfp = current_gfp_context(gfp);
	pcpu_gfp = gfp & (GFP_KERNEL | __GFP_NORETRY | __GFP_NOWARN);
	is_atomic = (gfp & GFP_KERNEL) != GFP_KERNEL;

/* :1765 —— 拿的是 mutex，不是 spinlock */
	if (gfp & __GFP_NOFAIL) {
		mutex_lock(&pcpu_alloc_mutex);
	} else if (mutex_lock_killable(&pcpu_alloc_mutex)) { ... }
```

| 事实 | 含义 |
|------|------|
| **`pcpu_alloc_mutex` 是 mutex** | **`alloc_percpu()` 可能睡眠** → **不能在中断、持 spinlock 时调用** |
| **gfp 只有三个标志被接受**（`GFP_KERNEL`/`__GFP_NORETRY`/`__GFP_NOWARN`） | 其他标志**被静默过滤掉**，别指望传 `GFP_ATOMIC` 能改变行为 |
| `is_atomic = (gfp & GFP_KERNEL) != GFP_KERNEL` | 传了"不完整 GFP_KERNEL"的请求被视为 atomic → **此时不能建新 chunk，空间不够就直接失败** |
| **reserved chunk 的存在理由** | 源码注释："Percpu allocator can serve percpu allocations **before slab is initialized** which allows **slab to depend on the percpu allocator**" —— 这是个**启动期的先有鸡还是先有蛋**问题的解法 |

> **实践结论**：`alloc_percpu()` 默认用在 **初始化/热插拔路径**（创建 netdev、挂载文件系统、加 CPU）；
> **数据路径不要调用它**。要避免"每 CPU 一份"的运行时分配，就在**模块加载时**一次分配好。

#### 使用规则

| 规则 | 原因 |
|------|------|
| **访问本 CPU 数据时禁止抢占迁移**（或用 `get_cpu_ptr` / `this_cpu_*`） | 否则 **读到/写到别核副本**（RMW 尤其危险：并发在两核上各自加减） |
| **`this_cpu_*` 只保证"本 CPU 内"排他** | 中断里也访问同一个 percpu 变量时，**非 `this_cpu_*` 的写法会被打断** |
| **汇总全系统值** | 遍历 **`for_each_online_cpu`**（值存在）/ **`for_each_possible_cpu`**（分配时），**非热路径**做 |
| **读别的 CPU 的值** | 是**允许的但语义上近似**：对方可能正在并发修改，需要额外的同步约定或 RCU |
| **CPU 热插拔** | 变量在 `possible` 的每 CPU 上都存在；**下线 CPU 上的数据要显式汇总/迁移** |
| **与 `preempt_disable`** | Ch 10 — per-CPU + **关抢占** 常结对 |

#### 典型内核用户

| 子系统 | per-CPU 内容 |
|--------|--------------|
| **softirq / NAPI** | **`softnet_data`** — 每核网络输入队列 |
| **Slab（SLUB）** | **`kmem_cache_cpu`**（见 [12.7](./section-12.7-Slab-层.md)）：`freelist`/`tid`/`partial` |
| **页分配器** | **`per_cpu_pages`**（见 [12.4](./section-12.4-获得页.md)）：order-0 无锁快路径 |
| **RCU** | **grace period 状态** |
| **统计** | **`vm_event_states`** 等 |

**`percpu_counter`：per-CPU 思想的"成品库"**（v6.6 `lib/percpu_counter.c` 实证）

```c
struct percpu_counter {
	raw_spinlock_t lock;
	s64 count;                 /* 中枢值：只在批量结算时动 */
	s32 __percpu *counters;    /* 每 CPU 一个 s32 增量 */
};
int percpu_counter_batch __read_mostly = 32;
/* compute_batch_value(): percpu_counter_batch = max(32, nr*2);  热插拔时重算 */
```

```
写（热路径）：percpu_counter_add(fbc, 1)
      └─ 只加本 CPU 的 s32；当 |本 CPU 增量| >= batch 时，才持 lock 一次性结算进 count

读（快，不准）：percpu_counter_read(fbc)      → 只读 count（忽略各 CPU 上的增量）
读（慢，准确）：percpu_counter_sum(fbc)        → 遍历所有 CPU 累加（= __percpu_counter_sum）
```

> **这是"per-CPU 权衡"的标准形态**：**写路径 O(1) 无锁**（代价是读的值不精确），
> 精度靠 `batch = max(32, 在线 CPU 数 × 2)` 来界定——
> 误差上界约 **`batch × nr_cpus`**，需要准确值就付遍历代价。
> 用户态设计**每核计数器**时可以直接照抄这个结构（每核累加 + 周期结算 + 阈值 flush）。

**HFT：** 用户态 **每核一条 SPSC ring**、**thread-local 订单簿缓存** = **per-CPU 同构**。避免 **`std::atomic` 全局 hot counter** — 用 **`cpu_local`** 聚合。与内核一样：**读总和慢路径做**，**写路径本核独占**。

> **HFT 补充（四条可直接落地的）**：
> ① **绑核是 per-CPU 的前提**：per-CPU 依赖"同一段代码总在同一核上跑"。
> 线程在核间漂移时，per-CPU 池会**跨核产生伪共享与冷 cache**——
> 内核自己用 `preempt_disable()`/`get_cpu_ptr()` 保证这一点，用户态对应 **`sched_setaffinity` 绑核**；
> ② **RMW 是最大的坑**：`counter++` 在 per-CPU 上**不是**"不用管"，
> 被抢占后恢复在另一核上，就会**在两个副本上各加一次**。用 `this_cpu_*` 语义（或显式禁抢占）；
> ③ **读的代价要提前算**：per-CPU 换来的是"写免费、读昂贵"。
> 如果统计值需要**每次决策前**读准确总和（比如风控限额），per-CPU 反而更慢——
> 这种情况用 **单写者 + 周期快照** 或直接上 `atomic`；
> ④ **别在热路径 `alloc_percpu()`**：它会**睡眠**（mutex）、且单笔上限 **32KB**。
> 池子、ring、簿记结构一律在**启动/建连阶段**分配完。

→ [Ch 8 softirq per-CPU](../../chapter-08-bottom-halves/) · [Ch 10 preempt_disable](../../chapter-06-kernel-data-structures) · [06 Gorman Slab per-CPU cache](../../../06-linux-mm/chapter-08-slab-allocator/notes/section-5-每-CPU-对象缓存.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** per-CPU 变量如何避免锁？有什么局限？

<details><summary>答案</summary>

per-CPU 变量给每个 CPU 一份独立副本，本核读写自己的副本无需锁。统计时累加所有 CPU 副本。局限：1) 抢占关闭期间才能安全访问本核副本（否则被迁移到其他核）；2) 累加需要遍历所有 CPU；3) 不能用于需要全局一致性的场景。网络收包计数器用 per-CPU，完美匹配单核收包模型。

> **按 v6.6 补充**：三点修正。
> ① "无需锁"的准确说法是 **`this_cpu_*` 系列提供"本 CPU 内"排他**
> （源码注释：exclusivity "on the **same** processor"），
> 靠 x86 上的 `%fs` 段前缀编译成**单条指令**实现，**跨 CPU 一致性一概不管**；
> ② "抢占关闭期间才能安全访问"——如果只是**读/写**标量，被抢占到别核只是"取错副本"，
> 但 **RMW（如 `counter++`）会导致两个副本各加一次**，是真正的逻辑错误。
> `this_cpu_*` 族自带"隐含抢占/中断保护"（`percpu-defs.h:486` 注释），新代码应默认用它；
> ③ 还有一条书上没提的局限：**`alloc_percpu()` 会睡眠**（`pcpu_alloc_mutex` 是 mutex，
> `mutex_lock_killable`），且**单次分配上限 32KB**（`PCPU_MIN_UNIT_SIZE`）。

</details>

**Q2.** 缓存行 bouncing 是什么？per-CPU 如何解决？

<details><summary>答案</summary>

多核频繁写同一全局变量（如计数器）→ 每个 CPU 的 L1 cache line 都要 invalidate → L2/L3 来回传递 cache line（bouncing）。per-CPU 给每个 CPU 独立计数器，本核写自己的 cache line 不影响其他核。只有读取总数时才汇总。这就是 `/proc/stat` 的实现原理。

> **按 v6.6 补充：光有"每 CPU 一份"还不够，还要防 false sharing。**
> 如果 N 个 CPU 的副本**挤在同一条 cache line 里**（比如一个 8 字节的 `long`，
> 8 核的副本正好塞满 64 字节一行），那么"每核写自己的"**依然会互相作废这条行**——
> 只是把"抢一个变量"变成了"抢一条 cache line"，**bouncing 一点没少**。
> 所以内核提供了 **`DEFINE_PER_CPU_SHARED_ALIGNED`** / **`DEFINE_PER_CPU_ALIGNED`**
> 强制每副本对齐到缓存行边界，以及 [`____cacheline_aligned_in_smp`](../../chapter-19-portability/notes/section-19.4-数据对齐和结构体填充.md)
> 用于动态结构体（12.4 的 `per_cpu_pages`、12.7 的 `kmem_cache_cpu` 都标了这个）。
> **判据**：副本大小 × CPU 数 是否超过一条 cache line——没超过就必须对齐。

</details>

**Q3.** `alloc_percpu()` 能在中断上下文调用吗？它能传 `GFP_ATOMIC` 吗？

<details><summary>答案</summary>

**不能，两个都不行。**

1. **`pcpu_alloc()` 拿的是 mutex**（`mm/percpu.c:1765`：`mutex_lock_killable(&pcpu_alloc_mutex)`，
   `__GFP_NOFAIL` 时退化为 `mutex_lock`），**会睡眠**，因此不能在中断、softirq、持自旋锁时调用；
2. **gfp 是白名单过滤的**（`:1736`）：`pcpu_gfp = gfp & (GFP_KERNEL | __GFP_NORETRY | __GFP_NOWARN)`——
   传进去的其他标志（含 `GFP_ATOMIC` 里的 `__GFP_HIGH`/`__GFP_KSWAPD_RECLAIM`）**会被静默丢弃**；
3. 反而有个"反向"效果：`is_atomic = (gfp & GFP_KERNEL) != GFP_KERNEL`——
   只要你传的不是完整的 `GFP_KERNEL`，就被判定为 **atomic 请求**，
   此时**不能新建 chunk、不能等待**，空间不够就**直接返回 NULL**。

实践结论：`alloc_percpu()` 只在**初始化 / 热插拔 / 加载模块**这类可睡眠路径上用；
数据路径上一律**提前分配好**，用 `this_cpu_ptr()` / `get_cpu_ptr()` 访问。
另注单次分配上限 **`PCPU_MIN_UNIT_SIZE` = 32KB**。

</details>

**Q4.** `raw_cpu_*`、`__this_cpu_*`、`this_cpu_*` 三族有什么区别？该用哪个？

<details><summary>答案</summary>

| 族 | 抢占/中断 | 什么时候用 |
|----|----------|-----------|
| `raw_cpu_*` | **完全不检查** | 几乎不用。源码注释："do not want to do any checks for preemptions. **Unless strictly necessary, always use [__]this_cpu_*() instead**" |
| `__this_cpu_*` | **断言抢占已关**（`__this_cpu_preempt_check("op")`，只在 `CONFIG_DEBUG_PREEMPT` 下生效） | 你**已经** `preempt_disable()` 了，想让 debug 配置帮你验证这个前提 |
| **`this_cpu_*`** | ✅ **隐含抢占/中断保护** | **默认选择**。注释："can be used **without worrying about preemption or interrupt**" |

三族共享同一套操作名（`read/write/add/and/or/add_return/xchg/cmpxchg/cmpxchg_double/inc/dec`）。

配套：
- 取指针用 **`this_cpu_ptr(ptr)`**；要"取到之后一直访问且不许被迁移"用 **`get_cpu_ptr()`/`put_cpu_ptr()`**（= `preempt_disable()` + `this_cpu_ptr()`）；
- 访问**别的** CPU 的副本用 **`per_cpu_ptr(ptr, cpu)`** / 静态变量的 `per_cpu(var, cpu)`。

</details>

**Q5.** `percpu_counter` 是怎么在"写快"和"读准"之间取舍的？误差上界是多少？

<details><summary>答案</summary>

结构（v6.6 `include/linux/percpu_counter.h:22`）：

```c
struct percpu_counter {
	raw_spinlock_t lock;
	s64 count;              /* 中枢值 */
	s32 __percpu *counters; /* 每 CPU 一个 s32 增量 */
};
```

**写路径**（`percpu_counter_add()` → `percpu_counter_add_batch(fbc, amount, percpu_counter_batch)`）：
只加**本 CPU 的 s32**，**不碰锁**；只有当本 CPU 增量的绝对值 **≥ batch** 时，才持 `lock`
把增量一次性结算进 `count` 并清零。

**batch 值**（`lib/percpu_counter.c:221,228`）：`percpu_counter_batch = max(32, num_online_cpus() * 2)`，
且在 **CPU 热插拔时重算**（`compute_batch_value()` 挂在 cpu hotplug 回调上）。

**读路径两档**：
- `percpu_counter_read()` — 只读 `count`，**快但不精确**（忽略了各 CPU 上尚未结算的增量）；
- `percpu_counter_sum()` — 遍历所有 CPU 累加，**准确但慢**。

**误差上界**：每个 CPU 最多持有一个 < batch 的未结算增量，
所以总数误差 **< `batch × nr_cpus`**，即 **≈ `max(32, 2×nr) × nr`**。
例如 64 核：batch = 128，误差上界 ≈ 8192。

> 用途上的推论：`percpu_counter` 适合**"阈值判断 + 偶尔精确结算"**的场景
> （如 `__percpu_counter_compare()` 判断是否超过水位：先用快读比，接近阈值才用 sum 精确算）；
> 需要**每次都读到精确值**的场合（如风控硬限额），per-CPU 反而更慢——
> 这时用单写者 + 原子变量更合适。

</details>

**Q6.** 打开 `CONFIG_RANDOM_KMALLOC_CACHES` 为什么会让 per-CPU 的静态内存预留涨 4 倍？

<details><summary>答案</summary>

因为 `include/linux/percpu.h:36-40` 把 `PERCPU_DYNAMIC_SIZE_SHIFT` 和这个配置绑在了一起：

```c
#ifdef CONFIG_RANDOM_KMALLOC_CACHES
#define PERCPU_DYNAMIC_SIZE_SHIFT      12
#else
#define PERCPU_DYNAMIC_SIZE_SHIFT      10
#endif
...
#define PERCPU_DYNAMIC_EARLY_SIZE	(20 << PERCPU_DYNAMIC_SIZE_SHIFT)
#if BITS_PER_LONG > 32
#define PERCPU_DYNAMIC_RESERVE		(28 << PERCPU_DYNAMIC_SIZE_SHIFT)   /* 64 位 */
#else
#define PERCPU_DYNAMIC_RESERVE		(20 << PERCPU_DYNAMIC_SIZE_SHIFT)   /* 32 位 */
#endif
```

于是 64 位内核上 `PERCPU_DYNAMIC_RESERVE` 从 **28KB（28<<10）变成 112KB（28<<12）**。

**为什么**：这个配置（见 [12.7 §7](./section-12.7-Slab-层.md)）为普通 kmalloc 建 **15 份副本 cache**，
SLUB 的 **per-CPU 状态**（`kmem_cache_cpu`）是按 cache 数量线性增长的，
所以要给动态 percpu 分配**预留更多空间**，否则启动早期（slab 还没起来、只有 first chunk 可用）
就会分配失败。

**这是一条通用规律的体现**：per-CPU 内存的静态开销 =
**"per-CPU 对象的种类数" × CPU 数**。
CPU 数越多、per-CPU 结构越多，**启动期预留和后续 dynamic chunk 的开销越大**——
在核数很多（如 256 核）的机器上，这笔账必须算进内存预算。

</details>

</details>
---
