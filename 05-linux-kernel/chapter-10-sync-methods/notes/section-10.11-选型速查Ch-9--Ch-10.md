## 选型速查（Ch 9 + Ch 10）

一页做完「该用哪把锁」— 先问 **上下文** 与 **持有时间**，再问读写比。

> 本文所有结论以 **v6.6** 源码为准，实证文件：
> `Documentation/locking/locktypes.rst`（17157 B，v6.6 官方锁类型与嵌套规则）
> `include/linux/local_lock_internal.h`（3499 B，非 RT / RT 两套实现全文）
> `include/asm-generic/qrwlock.h` + `qrwlock_types.h`（v6.6 排队读写锁）
> `arch/x86/include/asm/spinlock.h`（**v3.15** 历史版本，坐实 LKD 时代的写者饥饿）
> `include/linux/refcount.h`（12371 B）/ `include/linux/rwsem.h`（7783 B）
>
> ⚠️ 本篇的前两节（决策树 + 速查表）是原笔记内容，**第 1 节起是 v6.6 视角的深度补充**，
> 其中包含对原速查表的 **3 处订正**（rwlock 的写者饥饿、preempt_disable 的现代写法、
> 「不确定就用 spin_lock_irqsave」这个建议本身）。

---

### §0 一页速查（原笔记，保留）

#### 决策树

```
能在中断 / softirq / 原子上下文？
  │是
  ├─ 只需改一个计数/标志 ──► atomic
  ├─ 短临界区 ──► spinlock（共享给 ISR 则 irqsave / bh）
  ├─ 读极多写极少且写要及时 ──► seqlock
  └─ 读多写少且写可饿 ──► rwlock（慎）   ← ⚠️ 见 §3：v6.6 上「写可饿」不成立
  │否（仅进程上下文）
  ├─ 短且绝不睡眠 ──► 仍可用 spinlock
  ├─ 互斥且可睡眠 ──► mutex（首选）
  ├─ 资源计数 >1 ──► semaphore
  └─ 等「做完了」事件 ──► completion

只要防本 CPU 调度迁移、数据 per-CPU ──► preempt_disable   ← ⚠️ 见 §4：新代码用 local_lock
跨核/设备看见顺序 ──► mb / rmb / wmb（或依赖锁自带语义）
```

#### 表格式速查

| 场景 | 首选 | 禁止 |
|------|------|------|
| 单变量计数/标志 | **atomic_t** | 无保护 `++` |
| 短临界区、中断可重入路径 | **spinlock + irqsave/bh** | 持锁睡眠 |
| 读多写少、写可延迟 | **rwlock** | 读锁里长时间工作 |
| 读极多、写很少、写不能饿 | **seqlock** | 读侧有副作用 |
| 仅当前 CPU 私有 | **`preempt_disable` / this_cpu** | 当跨 CPU 锁用 |
| 进程上下文、互斥、可睡 | **`mutex`** | 在 ISR 用 |
| 计数资源池 | **semaphore** | ISR 里 `down` |
| 等另一上下文完成 | **completion** | 在 ISR 里 wait |
| 跨核/MMIO 顺序 | **barrier 家族** | 假设「赋值即全局可见」 |
| 新代码大锁省事 | — | **BKL** |

#### 持有时间经验

| 量级 | 倾向 |
|------|------|
| 几十周期～几微秒 | spinlock / atomic |
| 可能阻塞、等 I/O、拷用户态 | mutex |
| 「几乎只读配置」 | seqlock / RCU（书后续主题） |

#### HFT / 驱动一句话

| 路径 | 原则 |
|------|------|
| **热路径** | 原子 > 短自旋 > 无锁结构；避免睡眠锁 |
| **慢路径 / ioctl / probe** | mutex、completion |
| **硬中断** | 只做最短工作 + 调度 softirq/tasklet/work；锁用 irqsave |
| **观测** | `perf lock`、锁持有时间直方图、`%soft` |

---

## §1 v6.6 官方的三分类（不是「快/慢」两分）

LKD 的讲法是按「会不会睡眠」分，但 v6.6 的 `Documentation/locking/locktypes.rst:12-14`
开篇就说内核的锁分成 **三类**，第三类是 LKD 时代没有的：

```
The kernel provides a variety of locking primitives which can be divided
into three categories:
```

### 1.1 三类与其成员（`locktypes.rst:23-83` 原文清单）

| 类别 | 成员 | 关键性质 |
|------|------|----------|
| **Sleeping locks**<br>（睡眠锁） | `mutex`、`rt_mutex`、`semaphore`<br>`rw_semaphore`、`ww_mutex`、`percpu_rw_semaphore` | 只能在**可抢占的进程上下文**获取 |
| **CPU local locks**<br>（CPU 本地锁） | `local_lock` | **纯本 CPU 并发控制，不防跨 CPU** |
| **Spinning locks**<br>（自旋锁） | `raw_spinlock_t`、**bit spinlocks** | 严格自旋，任何配置下都不睡 |

而 `spinlock_t` / `rwlock_t` / `local_lock` 是**会变的**（`locktypes.rst:46-49, 71-73`）：

> On PREEMPT_RT kernels, these lock types are converted to sleeping locks:
> `local_lock`、`spinlock_t`、`rwlock_t`
>
> On non-PREEMPT_RT kernels, these lock types are also spinning locks:
> `spinlock_t`、`rwlock_t`

也就是说——

```
                   非 PREEMPT_RT                PREEMPT_RT
                ┌──────────────────┐        ┌──────────────────┐
spinlock_t      │ spinning         │  ───►  │ sleeping (rt_mutex)│
rwlock_t        │ spinning         │  ───►  │ sleeping (rt_mutex)│
local_lock      │ CPU local        │  ───►  │ per-CPU spinlock   │
                ├──────────────────┤        ├──────────────────┤
raw_spinlock_t  │ spinning         │  不变  │ spinning           │
bit spinlock    │ spinning         │  不变  │ spinning           │
mutex / rwsem   │ sleeping         │  不变  │ sleeping           │
                └──────────────────┘        └──────────────────┘
```

⭐ **选型的第一个问题因此变成：「你的代码将来要不要跑在 PREEMPT_RT 上？」**
因为 `spinlock_t` 在 RT 上会变成睡眠锁，所有「持 spinlock 时不能睡」的推理
（以及「持 spinlock 时关了抢占所以 per-CPU 数据一定安全」的推理）**全部失效**。

### 1.2 raw_spinlock_t 的保留地（`locktypes.rst:210-217`）

文档对 `raw_spinlock_t` 的用法给了很窄的许可：

> Use raw_spinlock_t only in **real critical core code, low-level interrupt
> handling** and places where **disabling preemption or interrupts is required**,
> for example, to safely access hardware state. raw_spinlock_t can sometimes
> also be used when the **critical section is tiny**, thus avoiding RT-mutex overhead.

翻译成选型判据，只有三条：
1. 底层中断处理 / 调度器 / 真正的 core code
2. 本身就**需要**关抢占或关中断（比如访问硬件状态）
3. 临界区**极小**，用 rt_mutex 的开销反而更亏

除此之外一律用 `spinlock_t`。这也是 Ch10.7 那条规律的延续——
「自编排睡眠序列的地方用 raw 锁」，`raw_spinlock_t` 的合法性来自**它不会被偷偷改成睡眠锁**。

### 1.3 bit spinlock：唯一无法被 RT 替换的锁（`locktypes.rst:492-500`）

文档原文：

> PREEMPT_RT **cannot substitute** bit spinlocks because **a single bit is too
> small to accommodate an RT-mutex**. Therefore, the semantics of bit spinlocks
> are preserved on PREEMPT_RT kernels, so that the **raw_spinlock_t caveats
> also apply** to bit spinlocks.

⭐ 这是整章唯一一个「物理上无法被 RT 化」的锁：**一个 bit 塞不下一个 rt_mutex**。
后果是：bit spinlock 在 RT 上仍然是真自旋 + 关抢占，所以
「持 bit spinlock 时不能睡」在 RT 上依然成立，它是 `raw_spinlock_t` 的同伙，
位于 §2 嵌套金字塔的**最底层**。

文档还提到有些 bit spinlock 是靠**使用点的 `#ifdef`** 被替换成普通 `spinlock_t` 的，
而 `spinlock_t` 的替换不需要改使用点（靠头文件和核心实现里的条件编译），
这就是为什么「用 `spinlock_t` 而不是 bit spinlock」在 RT 兼容性上更省心。

### 1.4 Owner semantics（`locktypes.rst:86-95`）—— semaphore 是唯一的异类

> The aforementioned lock types **except semaphores** have strict owner semantics:
> The context (task) that acquired the lock must release it.

这条看似显然，但它是 semaphore 在现代内核里被劝退的根因（见 §7）。
唯一的例外接口是 `rw_semaphore` 的**非属主释放**（`locktypes.rst:94-95`）：
读锁允许由另一个 task 释放（专用接口，如 `up_read_non_owner()`）。

**HFT 关联**：属主语义是**优先级继承（PI）的前提**。没有属主 → 不知道该 boost 谁 →
无法 PI → 无界的优先级反转（详见 §7.1）。

---

## §2 ⭐⭐ 嵌套金字塔：选型的硬约束（原文完全缺失）

`locktypes.rst:502-520` 给的规则，是比「性能排序」重要得多的选型约束——
**违反它根本跑不起来（lockdep 会直接报错），而不是「慢一点」。**

### 2.1 四条基本规则（原文）

```
The most basic rules are:

  - Lock types of the same lock category (sleeping, CPU local, spinning)
    can nest arbitrarily as long as they respect the general lock ordering
    rules to prevent deadlocks.

  - Sleeping lock types cannot nest inside CPU local and spinning lock types.

  - CPU local and spinning lock types can nest inside sleeping lock types.

  - Spinning lock types can nest inside all lock types
```

### 2.2 由此得出的三层嵌套顺序（`locktypes.rst:513-519`）

```
  1) Sleeping locks                        ← 最外层
  2) spinlock_t, rwlock_t, local_lock      ← 中间层
  3) raw_spinlock_t and bit spinlocks      ← 最内层
```

注意 `locktypes.rst:511-512` 的解释：**顺序是由 PREEMPT_RT 的类别迁移倒推出来的**：

> The fact that PREEMPT_RT changes the lock category of spinlock_t and rwlock_t
> from spinning to sleeping and substitutes local_lock with a per-CPU spinlock_t
> means that they **cannot be acquired while holding a raw spinlock**.

```
             ┌─────────────────────────────┐
             │  1) mutex / rwsem / ww_mutex│  可以睡
             │     semaphore / rt_mutex    │
             │  ┌──────────────────────────┴──┐
             │  │ 2) spinlock_t / rwlock_t    │  RT 上会睡
             │  │    local_lock               │
             │  │  ┌──────────────────────────┴──┐
             │  │  │ 3) raw_spinlock_t           │  永不睡
             │  │  │    bit spinlock             │
             │  │  └─────────────────────────────┘
             │  └────────────────────────────────┘
             └───────────────────────────────────┘

  合法方向：外 → 内（下面可以套进上面）
  非法方向：内 → 外（持有第 3 层时去拿第 2/1 层 = RT 上「原子上下文里睡觉」）
```

⭐ 最后一句是关键：

> **Lockdep will complain if these constraints are violated, both in
> PREEMPT_RT and otherwise.**

也就是说——**即使在非 RT 内核上，lockdep 也会按 RT 的规则报错**。
这是个刻意的设计：让非 RT 上的开发也能提前发现 RT 兼容性问题。

### 2.3 这条规则在真实代码里的三个陷阱

#### 陷阱 A：`local_irq_disable()` + `spin_lock()`（`locktypes.rst:396-410`）

```c
/* 非 RT 上等价于 spin_lock_irq()，但 RT 上是错的 */
local_irq_disable();
spin_lock(&lock);          /* RT 上 spinlock 是 rt_mutex，要求完全可抢占上下文 */
```

文档原文：

> On PREEMPT_RT kernel this code sequence **breaks because RT-mutex requires a
> fully preemptible context**. Instead, use `spin_lock_irq()` or
> `spin_lock_irqsave()` and their unlock counterparts.

**正确写法**：`spin_lock_irq(&lock)` —— 一个 API 同时表达「关中断 + 加锁」，
RT 上由实现自己决定怎么拆，而不是调用者手写两步。

#### 陷阱 B：`local_lock_irq()` + `raw_spin_lock()`（`locktypes.rst:333-345`）

```c
/* 非 RT 上等价于 raw_spin_lock_irq()，RT 上错误 */
local_lock_irq(&local_lock);
raw_spin_lock(&lock);
```

> On a PREEMPT_RT kernel this code sequence breaks because `local_lock_irq()`
> is mapped to a per-CPU `spinlock_t` which **neither disables interrupts nor
> preemption**.

**正确写法**：`local_lock_irq(&local_lock); spin_lock(&lock);` ——
两个都在第 2 层，RT 上都是 rt_mutex，可嵌套。

#### 陷阱 C：每个 local_lock 有独立作用域（`locktypes.rst:349-390`）

```c
/* ❌ 错的：两个不同名的 local_lock 不能互相序列化 func3() 的调用者 */
func1() { local_irq_save(flags);    -> local_lock_irqsave(&local_lock_1, flags);
          func3();
          local_irq_restore(flags); -> local_unlock_irqrestore(&local_lock_1, flags); }

func2() { local_irq_save(flags);    -> local_lock_irqsave(&local_lock_2, flags);
          func3();
          local_irq_restore(flags); -> local_unlock_irqrestore(&local_lock_2, flags); }

func3() { lockdep_assert_irqs_disabled();   /* RT 上必然触发 */
          access_protected_data(); }
```

非 RT 上这没问题——因为 `local_irq_save()` 是无条件的全局动作，谁调都一样。
但 **RT 上 `local_lock_1` 和 `local_lock_2` 是两把不同的 per-CPU spinlock**，
func1 和 func2 之间根本没有互斥。

**正确做法**：两个函数共**同一个** `local_lock`，断言改成 `lockdep_assert_held(&local_lock)`。

⭐ 这条揭示了 `local_lock` 与 `preempt_disable()` 的本质差别：
`preempt_disable()` 是**作用域无关的全局开关**（谁调都关同一个东西），
`local_lock` 是**具名的作用域**（谁和谁互斥由名字决定）。
把前者机械替换成后者时，如果不重新审视作用域边界，就会引入 RT 上才暴露的 bug。

---

## §3 ⭐ 版本断崖：rwlock 的「写者饥饿」在 v3.16 已终结

原速查表写的是：

> 读多写少、**写可饿** ──► rwlock（慎）

这是 **LKD（2.6 时代）的正确描述，在 v6.6 上不成立**。三重证据如下。

### 3.1 老实现（v3.15，x86 `arch/x86/include/asm/spinlock.h:222-239`）

```c
static inline void arch_read_lock(arch_rwlock_t *rw)
{
	asm volatile(LOCK_PREFIX READ_LOCK_SIZE(dec) " (%0)\n\t"
		     "jns 1f\n"                  /* 计数 >= 0 → 拿到 */
		     "call __read_lock_failed\n\t"
		     "1:\n"
		     ::LOCK_PTR_REG (rw) : "memory");
}

static inline void arch_write_lock(arch_rwlock_t *rw)
{
	asm volatile(LOCK_PREFIX WRITE_LOCK_SUB(%1) "(%0)\n\t"
		     "jz 1f\n"                   /* 计数 == 0 → 拿到 */
		     "call __write_lock_failed\n\t"
		     "1:\n"
		     ::LOCK_PTR_REG (&rw->write), "i" (RW_LOCK_BIAS)
		     : "memory");
}
```

`RW_LOCK_BIAS = 0x01000000`。读锁是 **`lock decl`** 把计数减 1，
写锁是 **`lock subl $0x01000000`** 把计数减一整个 BIAS。

⭐ **饥饿的根源就在这两条指令的不对称**：

| | 指令 | 成功条件 | 是否排队 |
|---|---|---|---|
| 读者 | `lock decl` （-1） | 结果 **≥ 0** | ❌ 不排队，直接抢 |
| 写者 | `lock subl $BIAS`（-0x1000000） | 结果 **== 0** | ❌ 不排队，直接抢 |

读者的成功条件是「≥ 0」，写者的成功条件是「== 0」。
只要有读者在持锁（计数从 0x01000000 掉到 0x00ffffff），后来者只要 `decl` 到 ≥ 0 就成功。
于是**源源不断的读者会让计数永远回不到 0x01000000**，写者 `subl $BIAS` 永远得不到 0。

```
t0:  cnts = 0x0100_0000                  （无人持锁）
t1:  读者 A: decl → 0x00ff_ffff  ≥0 ✅ 拿到
t2:  读者 B: decl → 0x00ff_fffe  ≥0 ✅ 拿到
t3:  写者 W: subl $0x0100_0000 → 0xfeff_fffe  ≠0 ❌ 失败，进 __write_lock_failed
t4:  读者 C: decl → 0x00ff_fffd  ≥0 ✅ 又拿到了（插队在 W 前面！）
t5:  读者 D: decl → 0x00ff_fffc  ≥0 ✅ 又拿到
...  只要读者不停，W 永远等不到 cnts == 0x0100_0000
```

**这就是 LKD 讲的「读者优先 / 写者可能饿死」**，`__write_lock_failed` 里也只是自旋重试同样的 `subl`，没有任何排队机制。

### 3.2 新实现（v3.16 起，排队公平）

二分定位证据：

| tag | `kernel/locking/qrwlock.c` | 结论 |
|-----|---------------------------|------|
| v3.14 | 77 B（404 残片） | 不存在 |
| **v3.15** | **77 B（404 残片）** | **不存在** |
| **v3.16** | **3733 B** | **引入** |

作者：`(C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.`，`Authors: Waiman Long`。

v3.16 的 `kernel/locking/qrwlock.c` 里能看到排队逻辑：

```c
	/* :107-108 —— 写者主动广播「我在等」 */
	 * Set the waiting flag to notify readers that a writer is pending,
	 * or wait for a previous writer to go away.

	/* :114 */
				    cnts | _QW_WAITING) == cnts))

	/* :66 —— 读者拿不到锁时进等待队列，而不是原地重抢 */
	 * Put the reader into the wait queue

	/* :71 */
	 * At the head of the wait queue now, wait until the writer state
```

⭐ **`_QW_WAITING` 这个标志位就是「反饥饿」的全部机制**：
写者等锁时先把 `_QW_WAITING` 置上；新来的读者看到这个标志，
就知道有写者在等，于是**不再插队**，而是乖乖进等待队列。

### 3.3 v6.6 的 qrwlock 位布局（`include/asm-generic/qrwlock.h:26-31`）

```c
#define	_QW_WAITING	0x100		/* A writer is waiting	   */
#define	_QW_LOCKED	0x0ff		/* A writer holds the lock */
#define	_QW_WMASK	0x1ff		/* Writer mask		   */
#define	_QR_SHIFT	9		/* Reader count shift	   */
#define _QR_BIAS	(1U << _QR_SHIFT)
```

```
  31 ──────────────  9  8    7 ──── 0
  ┌──────────────────┬───┬──────────┐
  │   读者计数        │ W │  wlocked │
  │  (每个读者 +0x200)│ A │  (0xff)  │
  │                  │ I │          │
  │                  │ T │          │
  └──────────────────┴───┴──────────┘
   bit9+              bit8  bit0-7
   _QR_BIAS=0x200    0x100   _QW_LOCKED=0x0ff
```

为什么 `_QW_LOCKED` 是 `0x0ff`（8 位全 1）而不是 `0x01`？看类型定义就明白了：

```c
/* include/asm-generic/qrwlock_types.h:13-27 */
typedef struct qrwlock {
	union {
		atomic_t cnts;
		struct {
#ifdef __LITTLE_ENDIAN
			u8 wlocked;	/* Locked for write? */
			u8 __lstate[3];
#else
			u8 __lstate[3];
			u8 wlocked;	/* Locked for write? */
#endif
		};
	};
	arch_spinlock_t		wait_lock;
} arch_rwlock_t;
```

⭐⭐ **这是个很漂亮的技巧**：`cnts` 和 `wlocked` 是 **union**。
写者持锁时把低 8 位全写成 1（即 `wlocked` 字节 = `0xff`），
放锁时——

```c
/* qrwlock.h:116-121 */
static inline void queued_write_unlock(struct qrwlock *lock)
{
	smp_store_release(&lock->wlocked, 0);
}
```

——**只写一个字节，而且不是原子操作**。
对比 `queued_read_unlock()` 必须 `atomic_sub_return_release(_QR_BIAS, &lock->cnts)`（真原子指令），
写者放锁便宜得多。代价是这个 union 依赖字节序（源码里 `wlocked` 的位置按
`__LITTLE_ENDIAN` 分了两种排布），大端机器上字段顺序相反。

### 3.4 v6.6 rwlock 的无争用成本

```c
/* qrwlock.h:77-88 —— 读者：一条原子指令 */
static inline void queued_read_lock(struct qrwlock *lock)
{
	int cnts;
	cnts = atomic_add_return_acquire(_QR_BIAS, &lock->cnts);
	if (likely(!(cnts & _QW_WMASK)))
		return;
	queued_read_lock_slowpath(lock);      /* 有写者/有写者在等 → 排队 */
}

/* qrwlock.h:92-102 —— 写者：一条原子指令 */
static inline void queued_write_lock(struct qrwlock *lock)
{
	int cnts = 0;
	/* Optimize for the unfair lock case where the fair flag is 0. */
	if (likely(atomic_try_cmpxchg_acquire(&lock->cnts, &cnts, _QW_LOCKED)))
		return;
	queued_write_lock_slowpath(lock);
}
```

| 路径 | 无争用时的开销 | 放锁 |
|------|----------------|------|
| `read_lock()` | 1 × `atomic_add_return_acquire`（`lock xadd`） | 1 × `atomic_sub_return_release` |
| `write_lock()` | 1 × `atomic_try_cmpxchg_acquire`（`lock cmpxchg`） | ⭐ 1 字节 `store_release`，**非原子** |

⚠️ 注意 `queued_write_lock` 的 fastpath 是 **CAS 而不是 fetch-add**——
它要求 `cnts == 0`（既无写者也无读者），一旦不满足立刻走 slowpath。
这就是「公平」的代价：写者不能像老实现那样靠 `subl` 碰运气。

### 3.5 官方对 v6.6 rwlock 的定性（`locktypes.rst:299-302`）

```
Non-PREEMPT_RT kernels implement rwlock_t as a spinning lock and the
suffix rules of spinlock_t apply accordingly. **The implementation is fair,
thus preventing writer starvation.**
```

### 3.6 那 rwlock 现在还「慎」吗？

**仍然要慎，但理由变了**：

| | LKD 时代（≤ v3.15） | v6.6 |
|---|---|---|
| 写者饥饿 | ⚠️ **会饿死** | ✅ 排队公平，不会饿 |
| 读侧递归 | ❌ 死锁 | ❌ 死锁（未变） |
| 读锁内睡眠 | ❌ 非法（自旋+关抢占） | ❌ 非法（未变） |
| 读多时扩展性 | 好（但饿写者） | ⚠️ 所有读者抢**同一个 `cnts` 缓存行**，读者越多 bounce 越凶 |
| RT 上的写者饥饿 | — | ⚠️ **重新出现**（见下） |

⭐ **RT 上写者饥饿又回来了**（`locktypes.rst:305-315`）：

> Because an rwlock_t writer cannot grant its priority to multiple readers,
> a preempted low-priority reader will continue holding its lock,
> **thus starving even high-priority writers**. In contrast, because readers
> can grant their priority to a writer, a preempted low-priority writer will
> have its priority boosted until it releases the lock, thus preventing that
> writer from starving readers.

非对称的根源是 **PI 只能点对点**：
- 一个写者 → 可以把自己的优先级给**那一个**阻塞的写者 ✅
- 一个写者 → 无法把优先级同时给**N 个**并发的读者 ❌

所以 RT 上 rwlock 的饥饿方向**反过来了**：低优先级读者会饿死高优先级写者。
`rw_semaphore` 在 RT 上有完全一样的问题（`locktypes.rst:147-155`）。

**HFT 关联**：这直接决定了「行情网关里配置表该用什么锁」。
RT 内核 + 有多个并发读者时，rwlock 的写侧延迟是**无界**的（取决于最低优先级读者的调度），
这时候宁可用 `seqlock`（写者不会被读者阻塞，只会被写者阻塞）或 RCU。

---

## §4 ⭐ preempt_disable 的现代替代品：local_lock

原速查表写的是「仅当前 CPU 私有 → `preempt_disable` / this_cpu」。
v6.6 的新代码应该用 **`local_lock`**。

### 4.1 非 RT：它是个零开销的「空结构体」

```c
/* include/linux/local_lock_internal.h:14-21 */
#ifndef CONFIG_PREEMPT_RT
typedef struct {
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
	struct task_struct	*owner;
#endif
} local_lock_t;
```

⭐⭐ **`CONFIG_DEBUG_LOCK_ALLOC=n` 时 `local_lock_t` 里一个字段都没有——0 字节。**
再加上：

```c
/* :66-71 */
#define __local_lock(lock)					\
	do {							\
		preempt_disable();				\
		local_lock_acquire(this_cpu_ptr(lock));		\
	} while (0)
```

而 `local_lock_acquire()` 在非 debug 构建下就是个空函数（`:50`）：

```c
static inline void local_lock_acquire(local_lock_t *l) { }
```

所以 **`local_lock()` 在非 debug、非 RT 的内核上，展开后就是纯粹的 `preempt_disable()`**。
它不是「为了 RT 而付出的成本」，它是**免费的**——这是它值得用的第一个理由。

### 4.2 那为什么还要它？两个理由（`locktypes.rst:171-180`）

> The named scope of local_lock has two advantages over the regular primitives:
>
>  - The lock name allows **static analysis** and is also a clear documentation
>    of the protection scope while the regular primitives are **scopeless and opaque**.
>
>  - If lockdep is enabled the local_lock gains a **lockmap** which allows to
>    validate the correctness of the protection. This can detect cases where
>    e.g. a function using `preempt_disable()` as protection mechanism is
>    invoked from interrupt or soft-interrupt context. Aside of that
>    `lockdep_assert_held(&llock)` works as with any other locking primitive.

翻译成人话：

1. **`preempt_disable()` 是无名的作用域**——读代码的人（和静态分析工具）
   看不出「这两个 `preempt_disable`/`preempt_enable` 之间的数据是被谁保护的」。
   `local_lock(&foo_lock)` 有名有姓，作用域一目了然。

2. **开了 lockdep 之后它能被校验**。最典型的就是上面说的那类 bug：
   某个函数用 `preempt_disable()` 保护 per-CPU 数据，结果**被从 softirq 上下文调了**——
   `preempt_disable()` 挡不住 softirq，数据照样并发。用 `local_lock` 的话
   lockdep 会直接报「你这把锁在这里没被持有」。
   而 `preempt_disable()` 版本 lockdep 完全看不见。

还有 lockdep 的**锁类型标记**（`:30-34`）：

```c
# define LOCAL_LOCK_DEBUG_INIT(lockname)		\
	.dep_map = {					\
		.name = #lockname,			\
		.wait_type_inner = LD_WAIT_CONFIG,	\
		.lock_type = LD_LOCK_PERCPU,		\
	},						\
	.owner = NULL,
```

`LD_LOCK_PERCPU` 告诉 lockdep「这是把 per-CPU 锁」，从而套用 per-CPU 的正确性规则
（比如允许不同 CPU 上同名锁并发持有）。

### 4.3 RT：它变成 per-CPU spinlock

```c
/* :96-99 */
typedef spinlock_t local_lock_t;          /* ⭐ 类型都换了 */

/* :108-112 */
#define __local_lock(__lock)					\
	do {							\
		migrate_disable();				\
		spin_lock(this_cpu_ptr((__lock)));		\
	} while (0)
```

注意 `migrate_disable()` —— 这是 Ch10.9 §10 讲过的那个原语：
RT 上 `spinlock_t` 不再关抢占，所以必须**显式禁止迁移**才能保证
`this_cpu_ptr()` 拿到的指针在临界区内一直有效。

⭐ 而最露骨的是 `local_lock_irqsave`（`:118-124`）：

```c
#define __local_lock_irqsave(lock, flags)			\
	do {							\
		typecheck(unsigned long, flags);		\
		flags = 0;              /* ⭐ 直接把 flags 扔了 */
		__local_lock(lock);				\
	} while (0)
```

**`flags = 0`** —— RT 上这个版本**根本不读也不写中断状态**，
只是用 `typecheck()` 保住类型安全，然后把参数清零。
这意味着 RT 上 `local_lock_irqsave()` **不关中断**，
它只是禁止了迁移 + 拿了 per-CPU 锁。任何依赖「这里中断是关的」的代码在 RT 上都会炸。

### 4.4 迁移检查清单

| 老写法 | 新写法 | 非 RT 行为 | RT 行为 |
|---|---|---|---|
| `preempt_disable()` | `local_lock(&l)` | 完全相同 | per-CPU spinlock（可抢占） |
| `local_irq_disable()` | `local_lock_irq(&l)` | 完全相同 | ⚠️ **不关中断** |
| `local_irq_save(f)` | `local_lock_irqsave(&l, f)` | 完全相同 | ⚠️ **`f = 0`，不关中断** |
| `get_cpu_ptr()` | `local_lock(&l)` + `this_cpu_ptr()` | 完全相同 | 需要配合 `migrate_disable()` |

⚠️ 最后一行的典型事故（`locktypes.rst:434-452`）：

```c
/* ❌ 非 RT 正确，RT 上错误 */
struct foo *p = get_cpu_ptr(&var1);      /* get_cpu_ptr 隐含 preempt_disable() */
spin_lock(&p->lock);                     /* RT 上 spinlock 要求完全可抢占上下文 → 炸 */
p->count += this_cpu_read(var2);

/* ✅ 两种内核都对 */
migrate_disable();
p = this_cpu_ptr(&var1);
spin_lock(&p->lock);
p->count += this_cpu_read(var2);
```

---

## §5 计数类：atomic_t 还是 refcount_t？

原速查表只写了 `atomic_t`。但 v6.6 里**引用计数**有专门的答案。

### 5.1 refcount_t 的定位（`include/linux/refcount.h:1-9`）

```c
/*
 * Variant of atomic_t specialized for reference counts.
 *
 * The interface matches the atomic_t interface (to aid in porting) but only
 * provides the few functions one should use for reference counting.
 */
```

三个要点：
1. 是 `atomic_t` 的**特化**，不是新机制（底层还是原子指令）
2. 接口刻意做成和 `atomic_t` 一样，**便于移植**
3. 但**只暴露引用计数该用的那几个函数**——`refcount_add(5, &r)` 这种批量操作被刻意收紧

### 5.2 ⭐ 饱和语义（`:11-14`）—— 抗 UAF 攻击

> refcount_t differs from atomic_t in that the counter **saturates at
> REFCOUNT_SATURATED** and will not move once there. This avoids wrapping the
> counter and causing 'spurious' use-after-free issues.

```
   0                          INT_MAX     REFCOUNT_SATURATED   UINT_MAX
  (0x0000_0000)            (0x7fff_ffff)    (0xc000_0000)    (0xffff_ffff)
   +-------------------------------+----------------+----------------+
                                    <---------- bad value! ---------->
                                         (有符号视角下 = 负数)
```

`atomic_t` 溢出会**回绕**：`INT_MAX + 1 = INT_MIN`（负数），
而很多代码判断「计数降到 0 就释放」，回绕后计数从 0 继续减 → **UAF**。

`refcount_t` 一旦检测到溢出/下溢，就把计数**钉死**在 `0xc000_0000`，
从此不再变动。攻击者没法把它搞回 0，也就没法触发释放。

#### 为什么选 `0xc000_0000`（`:21-28`）

> by placing REFCOUNT_SATURATED roughly **equidistant from 0 and INT_MAX**
> we minimise the scope for error

饱和检测本身不是原子的（先 `atomic_fetch_add_relaxed`，
再判断，再 `atomic_set` 到饱和值），这是个 race：

```c
	int old = atomic_fetch_add_relaxed(r);
	// old is INT_MAX, refcount now INT_MIN (0x8000_0000)
	if (old < 0)
		atomic_set(r, REFCOUNT_SATURATED);
```

两个线程同时溢出时，中间可能有人继续加减。把饱和值放在 **0 和 INT_MAX 的中点**，
意味着要「逃逸」出饱和区，需要连续把计数从 `0xc000_0000` 推到 0，
跨度 `0x4000_0000` = 1073741824 次递减。

#### 安全性论证（`:44-52`）—— 一段很硬核的概率分析

文档接着算了这个 race 到底有多难打：

```
(UINT_MAX+1-REFCOUNT_SATURATED) / PID_MAX_LIMIT =
0x40000000 / 0x400000 = 0x100 = 256
```

`PID_MAX_LIMIT = 0x400000`（约 419 万，受 `FUTEX_TID_MASK` 限制，不能再大）。
所以攻击者需要**同时**操纵 256 次以上的嵌套引用，才能让计数逃出饱和区。
再加上要卡准调度时机连续多次命中同一个 race，文档的结论是：

> there doesn't appear to be a practical avenue of attack

⚠️ 但注意有个前提：**「no batched refcounting operations are used」**。
如果代码用 `refcount_add(1000, &r)` 这种大批量加减，安全性论证就不成立了。

### 5.3 ⭐ 内存序被刻意放宽（`:60-69`）

```
Memory ordering rules are slightly relaxed wrt regular atomic_t functions
and provide only what is strictly required for refcounts.

The increments are **fully relaxed**; these will not provide ordering.
The rationale is that whatever is used to obtain the object we're increasing
the reference count on will provide the ordering. For locked data structures,
its the lock acquire, for RCU/lockless data structures its the dependent load.

Do note that **inc_not_zero() provides a control dependency** which will order
future stores against the inc, this ensures we'll never modify the object
if we did not in fact acquire a reference.
```

| 操作 | 内存序 | 理由 |
|------|--------|------|
| `refcount_inc()` | ⭐ **relaxed** | 拿到对象这个动作本身已提供序（锁的 acquire / RCU 的依赖加载） |
| `refcount_dec_and_test()` | release（释放对象时必须） | 保证释放前的写对其他 CPU 可见 |
| `refcount_inc_not_zero()` | ⭐ 靠**控制依赖** | 天然保证「没拿到引用就不会改对象」 |

⭐ `refcount_inc()` 是 **relaxed** 的，这比 `atomic_inc()`（默认 full barrier 语义）
**便宜得多**。

⚠️ 但要澄清一个常见的误解：**relaxed 说的是「不做多余的屏障」，不是「不加 lock 前缀」**。
原子性还是要的，`refcount_inc()` 在 x86 上仍然是 `lock incl`（或 `lock xadd`），
只是**不额外插 `mfence`**。省掉的是屏障，不是原子指令。

### 5.4 选型判据

| 场景 | 用 |
|------|-----|
| 引用计数（对象生命周期） | **`refcount_t`** |
| 统计量、计数器（不控制生命周期） | `atomic_t` / `local_t` / per-CPU counter |
| 需要「>0 才+1」的弱引用升级 | `refcount_inc_not_zero()` |
| 位标志 | `unsigned long` + `set_bit/clear_bit`（原子位操作） |
| 统计用的 per-CPU 累加 | `this_cpu_add()` / `local_t` |

---

## §6 rw_semaphore：进程上下文的读写锁（原表缺失）

原速查表在「进程上下文」分支里没有读写锁。v6.6 的答案是 `rw_semaphore`。

### 6.1 API 与 v6.6 的现代写法（`include/linux/rwsem.h:168-215`）

```c
/* lock for reading */
extern void down_read(struct rw_semaphore *sem);
extern int __must_check down_read_interruptible(struct rw_semaphore *sem);
extern int __must_check down_read_killable(struct rw_semaphore *sem);
extern int down_read_trylock(struct rw_semaphore *sem);

/* lock for writing */
extern void down_write(struct rw_semaphore *sem);
extern int __must_check down_write_killable(struct rw_semaphore *sem);
extern int down_write_trylock(struct rw_semaphore *sem);

/* release */
extern void up_read(struct rw_semaphore *sem);
extern void up_write(struct rw_semaphore *sem);

/* ⭐ v6.6 的 cleanup.h 自动释放（:205-208） */
DEFINE_GUARD(rwsem_read, struct rw_semaphore *, down_read(_T), up_read(_T))
DEFINE_FREE(up_read, struct rw_semaphore *, if (_T) up_read(_T))

/* 写降级为读（:215）—— 不释放锁，避免中间窗口 */
extern void downgrade_write(struct rw_semaphore *sem);
```

⭐ 两个值得注意的现代特性：

1. **`guard(rwsem_read)` / `__free(up_read)`** —— 用 `cleanup.h` 的作用域自动释放，
   彻底消灭「错误路径忘记 unlock」。这是 Ch10.9 §9 里 `guard(preempt)` 同一套机制。

2. **`downgrade_write()`** —— 「先查后改」模式的原子降级：

```c
down_write(&sem);
if (!entry_exists(key)) {
	insert(key, val);
	downgrade_write(&sem);   /* ⭐ 降级为读锁，中间不开窗口 */
}
/* ... 继续以读锁身份访问 ... */
up_read(&sem);
```

如果写成 `up_write(); down_read();`，中间会有一个**无锁窗口**，
另一个写者可能插进来把 entry 删掉。

### 6.2 ⭐ 缓存行布局（`rwsem.h:37-46`）—— 一段被低估的优化注释

```c
/*
 * For an uncontended rwsem, count and owner are the only fields a task
 * needs to touch when acquiring the rwsem. So they are put next to each
 * other to increase the chance that they will share the same cacheline.
 *
 * In a contended rwsem, the owner is likely the most frequently accessed
 * field in the structure as the optimistic waiter that holds the osq lock
 * will spin on owner. For an embedded rwsem, other hot fields in the
 * containing structure should be moved further away from the rwsem to
 * reduce the chance that they will share the same cacheline causing
 * cacheline bouncing problem.
 */
```

这段话信息密度极高，拆成三条：

| 场景 | 谁被频繁访问 | 布局目标 |
|------|--------------|----------|
| **无争用** | `count` + `owner` | 放**相邻**，争取同一个 cacheline（一次预取搞定） |
| **有争用** | `owner`（乐观自旋者在 spin 它） | 把**结构体里的其它 hot field 挪远** |
| **嵌入在别的 struct 里** | — | ⚠️ 别把 rwsem 放在 hot 字段旁边 |

⭐ 第三条是可以直接用的工程建议：**把 `struct rw_semaphore` 放在结构体的冷端**，
不要和频繁读写的字段挤在一起。有争用时乐观自旋者（optimistic spinner）
会疯狂读 `owner`，导致整个 cacheline 在所有等待者之间 bounce，
旁边任何字段都会被拖累。

（乐观自旋 = OSQ，即 MCS 锁队列，Ch10.5 讲 mutex 时提到过同一个机制。）

### 6.3 rw_semaphore vs rwlock：选型

| | `rwlock_t` | `rw_semaphore` |
|---|---|---|
| 上下文 | 原子上下文可用 | **只能进程上下文**（会睡） |
| 临界区能否睡眠 | ❌ | ✅ |
| 开销（无争用） | ~1 条原子指令 | 更高（原子 + 可能的 `current` 记账） |
| 读者能否被抢占后睡眠 | ❌ 关抢占 | ✅ |
| 非 RT 上是否公平 | ✅ qrwlock 排队 | ✅（`locktypes.rst:137-138` 明说 fair） |
| RT 上写者饥饿 | ⚠️ 有 | ⚠️ 有（同样的 PI 点对点限制） |
| 非属主释放 | ❌ | ✅ 读锁有专用接口 |

**判据一句话**：临界区里要睡（比如要 `copy_to_user`、要 `kmalloc(GFP_KERNEL)`）→ 只能 `rw_semaphore`；
临界区是纯内存操作且可能在原子上下文 → `rwlock_t`。

---

## §7 semaphore 为什么被官方劝退

原速查表把 semaphore 列为「资源计数 >1」的首选。v6.6 官方的态度更微妙。

### 7.1 无属主 → 无法优先级继承（`locktypes.rst:122-129`）

```
semaphores and PREEMPT_RT
-------------------------

PREEMPT_RT does not change the semaphore implementation because counting
semaphores have **no concept of owners**, thus preventing PREEMPT_RT from
providing priority inheritance for semaphores. After all, **an unknown
owner cannot be boosted**. As a consequence, **blocking on semaphores can
result in priority inversion**.
```

这是 §1.4 那条「除 semaphore 外都有属主语义」的直接后果。
`mutex` 有属主 → 可以做 PI → 高优先级任务等待时能 boost 持有者。
`semaphore` 没有属主 → 没人可 boost → **优先级反转无解**。

### 7.2 官方明确劝退新代码（`locktypes.rst:116-120`）

```
Semaphores are often used for both serialization and waiting, but **new use
cases should instead use separate serialization and wait mechanisms, such as
mutexes and completions**.
```

⭐ 关键在「**both**」：semaphore 历史上被当成「互斥 + 等待」二合一用，
而官方认为这两件事应该**分开**——

| 你要做的事 | 老做法（semaphore） | v6.6 推荐 |
|---|---|---|
| 互斥 | `down(&sem)` / `up(&sem)` | **`mutex`** |
| 等某个事件完成 | 初值 0 的 semaphore | **`completion`** |
| 计数资源池（N > 1） | `down()` / `up()` × N | semaphore（**仅此场景保留**） |

也就是说，**semaphore 只剩「真·计数资源池」这一个正当用途**。
如果你只是要互斥，用 mutex；如果你只是要等事件，用 completion。

**HFT 关联**：这条对延迟敏感系统尤其重要。
`mutex` 有 PI，`semaphore` 没有——在低优先级任务持锁、高优先级任务等待的场景下，
`semaphore` 的等待时间**上界不可推理**，而 `mutex` 至少能把持有者的优先级顶上去。

---

## §8 验证：选完锁之后怎么证明它是对的

原「常见陷阱」第 3 条只写了「忘记 lockdep」。这一节给出完整的验证工具链。

### 8.1 lockdep：能验什么、不能验什么

| 能验 | 不能验 |
|------|--------|
| ✅ 锁顺序（ABBA 死锁） | ❌ 真正的性能问题 |
| ✅ 锁类型嵌套（§2 三层金字塔） | ❌ 逻辑错误（锁了不该锁的数据） |
| ✅ 原子上下文里用睡眠锁 | ❌ 忘记加锁（没加锁就没记录，看不见） |
| ✅ per-CPU 锁的作用域（`LD_LOCK_PERCPU`） | ❌ 运行时的争用程度 |
| ✅ 中断安全性（softirq/hardirq 上下文） | |

`include/linux/lockdep.h` 里的断言家族（`CONFIG_LOCKDEP=n` 时全部展开成 `(void)(l)`，零开销）：

```c
/* :323-338 —— 属主断言 */
#define lockdep_assert_held(l)			/* 我持有这把锁吗 */
#define lockdep_assert_not_held(l)
#define lockdep_assert_held_write(l)	/* 持有写锁吗 */
#define lockdep_assert_held_read(l)		/* 持有读锁吗 */
#define lockdep_assert_held_once(l)		/* 只报一次，避免刷屏 */
#define lockdep_assert_none_held_once()	/* 我什么锁都没持有吗（原子上下文自检） */

/* :613-628 —— 上下文断言 */
#define lockdep_assert_irqs_enabled()
#define lockdep_assert_irqs_disabled()
#define lockdep_assert_in_irq()
#define lockdep_assert_no_hardirq()
```

⭐ **`lockdep_assert_none_held_once()` 在热路径上特别有用**：
放在一个「我声称自己是原子的」函数开头，如果有调用者持着锁进来，第一次就会报。

⚠️ 但注意：这些断言在 `CONFIG_LOCKDEP=n` 时是 `do { } while (0)`（`:427-435`），
**生产内核上完全不生效**。它们是开发期工具，不是运行时保护。

### 8.2 剩下三类工具

| 工具 | 验什么 | 开关 |
|------|--------|------|
| **KCSAN** | 数据竞争。它**靠屏障建立 happens-before**——屏障写对了它就安静，写错了它就报 | `CONFIG_KCSAN` |
| **perf lock** | 锁争用的统计（获取次数、平均等待、最大等待） | `perf lock record/report` |
| **`lock` tracepoints** | 单次获取的完整时间线（谁、在哪、等了多久） | `/sys/kernel/tracing/events/lock/` |
| **LKMM** | 形式化验证你的内存序推理对不对 | `tools/memory-model/` + herd7 |

关于 KCSAN 和锁的关系，值得强调一点（呼应 Ch10.10 §11）：
KCSAN **认识屏障**。一段用 `READ_ONCE`/`WRITE_ONCE` + 正确屏障实现的无锁代码，
KCSAN 不会报；但如果你漏了屏障，KCSAN 就会报竞争——
**即使你认为逻辑上没问题**。这就是为什么「无锁代码必须通过 KCSAN」是硬要求。

### 8.3 HFT 特有的验收标准

「跑起来没 bug」对 HFT 不够。一个可用的验收清单：

1. **最坏情况延迟可推理**：能不能不看别人的代码就算出上界？
   （Ch10.7 §HFT 的判据：**如果评估最坏延迟需要读别人的代码，那设计就已经失败了**）
2. **持锁路径上没有不可控的阻塞点**：`kmalloc(GFP_KERNEL)`、`copy_to_user`、
   任何可能 page fault 或调度的东西
3. **优先级反转有解**：热路径上的睡眠锁必须有 PI（→ mutex，不是 semaphore）
4. **RT 上重测一遍**：`spinlock_t` 语义会变，所有「关了抢占所以安全」的推理都要重做
5. **有 lockdep + KCSAN 跑过的记录**：不是「应该没问题」，是「跑过，没报」

---

## §9 HFT 统一延迟画像（Ch10 全章汇总）

把前面十篇的量测结论汇总成一张表。所有数字是 **x86-64、无争用、典型服务器主频**
下的量级参考，**不是保证值**——跨平台（尤其 ARM64）必须自己重测。

### 9.1 获取/释放成本（无争用）

| 原语 | 获取 | 释放 | 是否关抢占 | 是否关中断 |
|------|------|------|-----------|-----------|
| `preempt_disable()` | `preempt_count_inc()` + `barrier()` | `barrier()` + 测试 | ✅ | ❌ |
| `local_lock()`（非 RT） | 同上 | 同上 | ✅ | ❌ |
| `READ_ONCE()` / `WRITE_ONCE()` | 0 指令（编译器屏障） | — | ❌ | ❌ |
| `smp_wmb()` / `smp_rmb()`（x86） | **0 指令**（`barrier()`） | — | ❌ | ❌ |
| `smp_mb()`（x86） | `lock addl $0,-4(%rsp)` | — | ❌ | ❌ |
| `atomic_inc()` | `lock incl` | — | ❌ | ❌ |
| `refcount_inc()` | `lock incl`（relaxed，无额外屏障） | — | ❌ | ❌ |
| `read_seqbegin()` | 1 次读 + `smp_rmb()`（x86 上是 0 指令） | 重试循环 | ❌ | ❌ |
| `write_seqlock()` | 1 × `spin_lock` + 2 × `smp_wmb` | ⭐ 放锁比加锁便宜 | ✅ | ❌ |
| `spin_lock()` | `lock` 指令 + 关抢占 | — | ✅ | ❌ |
| `read_lock()`（qrwlock） | `lock xadd` | `lock xadd` | ✅ | ❌ |
| `write_lock()`（qrwlock） | `lock cmpxchg` | ⭐ 1 字节非原子 store | ✅ | ❌ |
| `mutex_lock()` | fastpath：1 × cmpxchg | fastpath：1 × cmpxchg | ❌ | ❌ |
| `down_read()` / `down_write()` | 原子 + 记账 | — | ❌ | ❌ |
| `down()`（semaphore） | 原子 + 记账 | — | ❌ | ❌ |
| `wait_for_completion()` | → `schedule()` | — | ❌ | ❌ |

### 9.2 有争用时的量级

| 原语 | 有争用行为 | 延迟特征 |
|------|-----------|----------|
| `atomic_*` | 重试（硬件层面的缓存行仲裁） | 有界，随争用者数量上升 |
| `spin_lock` | 自旋（qspinlock 排队 + MCS） | 有界（临界区短时） |
| `seqlock` 读者 | **重试循环** | ⚠️ **无界**（写者持续写则读者持续重试） |
| `rwlock` 读者 | 排队自旋 | 有界（v3.16+ 公平） |
| `rwlock` 写者 | 排队自旋 | 有界（非 RT）；⚠️ RT 上受低优先级读者影响，无界 |
| `mutex` | → `schedule()` | ⚠️ 微秒级起，受调度器影响 |
| `semaphore` | → `schedule()` | ⚠️ 微秒级起，**且无 PI，上界不可推理** |
| `completion` | → `schedule()` | ⚠️ 取决于被等事件的耗时 |

### 9.3 ⭐ HFT 的三条硬规则

1. **热路径上不出现 `schedule()`**。
   一旦睡眠，延迟就从「几十纳秒」跳到「微秒 + 调度器不确定性」，
   而且你无法用代码控制上界。
   → 热路径 = `atomic` / per-CPU / 无锁结构 / 短自旋。

2. **优先消除共享，而不是优化锁**。
   Ch10.7 §迁移对照表里的原则：per-CPU 化、数据分片（sharding）、SPSC 队列。
   一把设计良好的无锁 SPSC 队列比任何锁都快。

3. **「最坏延迟」要能被推理出来**。
   - `spinlock`：上界 = 所有竞争者的临界区时长之和 → 可推理 ✅
   - `seqlock` 读者：读侧重试次数无界 → ❌ 不可推理（除非写者频率有硬上界）
   - `semaphore`：无 PI → ❌ 不可推理
   - `mutex`（带 PI）：上界 = 持有者的剩余临界区 + 调度延迟 → 基本可推理 ✅

### 9.4 内核原语 ↔ C++ `memory_order` 对照（Ch10.10 表，此处补全）

| 内核 | C++ | x86 实际指令 |
|------|-----|--------------|
| `barrier()` | `atomic_signal_fence` | 0（仅编译器屏障） |
| `smp_mb()` | `atomic_thread_fence(seq_cst)` | `lock addl $0,-4(%rsp)` |
| `smp_rmb()` | `atomic_thread_fence(acquire)` | 0（x86） |
| `smp_wmb()` | `atomic_thread_fence(release)` | 0（x86） |
| `smp_load_acquire()` | `load(acquire)` | 普通 `mov` |
| `smp_store_release()` | `store(release)` | 普通 `mov` |
| `READ_ONCE()` | `load(relaxed)`（+ 禁编译器优化） | 普通 `mov` |

⚠️ 两条容易记错的地方：
- **`barrier()` 不是 `atomic_thread_fence`**，它只等价于 `atomic_signal_fence`
  （只约束编译器，不产生任何指令）
- **上面标「0（x86）」的那几格，在 ARM64 上会变成真指令**
  （`dmb ishld` / `dmb ishst` / `ldar` / `stlr`）

x86 上免费的东西在 ARM64 上都要付钱——这是把 HFT 系统从 x86 迁到 ARM 时
最常见的性能落差来源。

---

## §10 实践模板

### 模板 A：给现有代码做 RT 兼容性审查

```
逐条扫，命中任一条就要改：

1. local_irq_disable(); spin_lock(&l);     → spin_lock_irq(&l);
2. local_lock_irq(&ll); raw_spin_lock(&l); → local_lock_irq(&ll); spin_lock(&l);
3. p = get_cpu_ptr(&v); spin_lock(&p->l);  → migrate_disable(); p = this_cpu_ptr(&v);
                                             spin_lock(&p->l);
4. preempt_disable() 保护 per-CPU 数据     → local_lock()（顺便让 lockdep 能验）
5. 两个函数各自 local_irq_save 保护同一份数据
                                           → 共用同一个 local_lock，
                                             断言改 lockdep_assert_held(&ll)
6. 持 spinlock 时调用可能睡眠的函数        → RT 上 spinlock 会睡，但调用者的
                                             「关了抢占所以安全」的假设已失效
7. 用了 semaphore 做互斥                   → mutex（有 PI）
8. 用了 semaphore 等事件                   → completion
```

验收：开 `CONFIG_PREEMPT_RT` + `CONFIG_LOCKDEP` + `CONFIG_DEBUG_ATOMIC_SLEEP` 跑一遍。
第 6 条会被 `DEBUG_ATOMIC_SLEEP` 直接抓到。

### 模板 B：热路径的锁选择（决策顺序）

```
第 0 步：能不能不共享？
  ├─ per-CPU 化（+ local_lock 保护）
  ├─ 分片（sharding）：把 1 把锁拆成 N 把
  └─ SPSC 队列：把共享变成消息传递
      ↓ 都不行，必须共享
第 1 步：临界区里会不会睡？（kmalloc(GFP_KERNEL) / copy_*_user / msleep）
  ├─ 会 → 只能睡眠锁：mutex（互斥）/ rwsem（读写）/ completion（等事件）
  └─ 不会 → 继续
第 2 步：会不会在中断 / softirq 上下文被调？
  ├─ 会 → spinlock 家族（+ _bh / _irq / _irqsave 后缀）
  │       └─ 是 core code 或必须关中断？→ raw_spinlock_t
  └─ 不会 → 继续
第 3 步：读写比？
  ├─ 几乎只读 + 读侧不能有副作用 + 数据简单 → seqlock
  │       ⚠️ 读侧重试无界；写者必须互斥
  ├─ 读多写少 + 需要读侧能睡 → rw_semaphore
  ├─ 读多写少 + 不能睡 → rwlock（注意 RT 上写者会饿）
  └─ 读写差不多 → spinlock / mutex
第 4 步：只是个计数/标志？
  └─ atomic_t（统计）/ refcount_t（生命周期）
```

### 模板 C：把「不确定就用 spin_lock_irqsave」换成正确的做法

原 Q1 的答案最后一句是「不确定 → `spin_lock_irqsave()`（最安全）」。
**这个建议在 v6.6 上需要加限定**：

| 情况 | 建议 |
|------|------|
| 真的不确定在哪儿被调用 | ✅ `spin_lock_irqsave()` 确实是保守正确的选择 |
| 但代价是： | ⚠️ 每次都关中断 → 中断延迟变差；RT 上行为又不一样 |
| 更好的做法 | 先搞清楚调用上下文，用 `lockdep_assert_*()` 把假设**固化成断言**： |

```c
void my_func(void)
{
	/* 把「我假设自己只在进程上下文被调用」写成断言 */
	lockdep_assert_in_task();        /* 或 lockdep_assert_no_hardirq() */
	lockdep_assert_irqs_enabled();

	mutex_lock(&my_lock);            /* 于是可以放心用便宜的 mutex */
	...
	mutex_unlock(&my_lock);
}
```

**「不确定就上最重的锁」是回避思考，不是设计。**
正确做法是把上下文假设写成断言，让 lockdep 在开发期就逼你确认它。

---

## §11 自检清单（10 条）

1. ✅ 我知道这段代码会在哪些上下文被调用（进程 / softirq / hardirq / NMI）吗？断言写了吗？
2. ✅ 临界区里有没有任何可能睡眠的调用？
3. ✅ 我用的锁在 PREEMPT_RT 上属于哪一类？嵌套顺序（§2 金字塔）对吗？
4. ✅ per-CPU 数据是用 `local_lock` 保护的吗（而不是裸 `preempt_disable`）？
5. ✅ 引用计数用的是 `refcount_t` 吗？
6. ✅ 需要互斥的地方，我用的是 `mutex` 而不是 `semaphore` 吗？
7. ✅ 「等事件」用的是 `completion` 吗？
8. ✅ 嵌入 `rw_semaphore` 时，它离结构体的 hot 字段够远吗？
9. ✅ 跑过 `CONFIG_LOCKDEP=y` 和 `CONFIG_KCSAN=y` 吗？有记录吗？
10. ✅ 我能不看别人的代码就算出这条路径的最坏延迟吗？

---

### 常见陷阱

1. 在所有场景都用 spinlock——短临界区用 spinlock，长临界区用 mutex
2. 忽略 RCU——读极多写极少时 RCU 是最优解（读端零开销）
3. 忘记 lockdep——开发阶段开 lockdep 检测死锁/锁顺序问题
4. **在 RT 上假设 `spinlock_t` 还关着抢占**——它在 RT 上是 rt_mutex（§1.1）
5. **把 `preempt_disable()` 机械替换成 `local_lock()` 却不重审作用域**——
   两个不同名的 local_lock 之间不互斥（§2.3 陷阱 C）
6. **还以为 rwlock 会饿死写者**——v3.16 起是 qrwlock，排队公平（§3）
7. **但不知道 RT 上 rwlock 的写者饥饿又回来了**——反向的：低优先级读者饿死高优先级写者（§3.6）
8. **用 `atomic_t` 做引用计数**——应该用 `refcount_t`，有饱和保护（§5）
9. **用 semaphore 做互斥或等事件**——官方明确劝退，用 mutex + completion（§7）
10. **把 `rw_semaphore` 嵌在结构体的 hot 字段旁边**——有争用时 owner 被 spin，整行 bounce（§6.2）
11. **用 `up_write(); down_read();` 做锁降级**——中间有窗口，应该用 `downgrade_write()`（§6.1）
12. **「不确定就用 spin_lock_irqsave」当挡箭牌**——应该把上下文假设写成 lockdep 断言（§10 模板 C）
13. **以为 `barrier()` 等价于 C++ 的 `atomic_thread_fence`**——它只等价于 `atomic_signal_fence`（§9.4）
14. **在 x86 上调好的无锁代码直接搬到 ARM64**——x86 上 0 指令的屏障在 ARM64 上全是真指令（§9.4）
15. **以为 bit spinlock 在 RT 上也会被换成睡眠锁**——不会，1 bit 塞不下 rt_mutex（§1.3）
16. **没开 RT 就不关心 RT 兼容性**——lockdep 在非 RT 上也会按 RT 规则报嵌套违规（§2.2）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 给定场景，如何快速选择同步原语？

<details><summary>答案</summary>

中断上下文 → spin_lock_irqsave()。softirq → spin_lock_bh()。进程上下文 + 短临界区（<1us） → spin_lock()。进程上下文 + 长临界区 → mutex。读极多写极少 + 简单数据 → seqlock。读极多写极少 + 复杂数据 → RCU。一次性等待 → completion。引用计数 → refcount_t。不确定 → spin_lock_irqsave()（最安全）。

</details>

<details><summary>按 v6.6 修订/补充</summary>

这条答案大体可用，但有 **4 处需要修订**：

**① 「不确定 → spin_lock_irqsave()」这个兜底建议要加限定。**
它确实是保守正确的（关中断 + 关抢占 + 自旋，任何上下文都不会出错），
但代价是每次都关中断，会推高中断延迟，不利于 HFT 场景的尾延迟。
更重要的是——**它回避了「搞清楚调用上下文」这个本来该做的工作**。
正确做法是用 lockdep 断言把假设固化下来：

```c
lockdep_assert_in_task();          /* 我假设自己在进程上下文 */
lockdep_assert_irqs_enabled();
mutex_lock(&lock);                 /* 于是可以安全地用便宜的 mutex */
```

断言在 `CONFIG_LOCKDEP=n` 时展开成空，生产内核零开销。

**② 「引用计数 → refcount_t」这条是对的，而且比原答案说的更重要。**
`refcount_t` 不只是「更好看」，它有三个实打实的差别：
- **饱和语义**：溢出时钉死在 `0xc000_0000`，不会回绕成负数导致 UAF（§5.2）
- **内存序更便宜**：`refcount_inc()` 是 **relaxed** 的，不插多余屏障（§5.3）
- **接口收紧**：只暴露引用计数该用的函数，从 API 层面堵住 `refcount_add(1000, ...)` 这类危险操作

**③ 缺了两类 v6.6 的常用原语：**
- **`local_lock`**：per-CPU 数据的现代保护方式，非 RT 上零开销（空结构体），
  但能被 lockdep 校验（§4）——原答案的「不确定就 spin_lock_irqsave」在 per-CPU 场景下应该用它
- **`rw_semaphore`**：进程上下文的读写锁。如果临界区要睡（比如 `copy_to_user`），
  `rwlock` 根本不能用，只能用 `down_read()` / `down_write()`（§6）

**④ 「进程上下文 + 短临界区 → spin_lock()」在 RT 上要重新想。**
RT 上 `spinlock_t` 变成 rt_mutex，能睡眠、不关抢占。
这时「短临界区用 spinlock」的理由（避免 schedule 开销）在 RT 上不成立——
RT 上 spinlock 也可能 schedule。RT 场景下真正该问的是：
**「我需要它不关抢占吗？」** 不需要 → 直接用 mutex。

**⑤ 补充一个原答案没提的硬约束：嵌套顺序（§2）。**
选定之后的组合还必须满足：

```
1) Sleeping locks          （mutex / rwsem / semaphore / ww_mutex）
2) spinlock_t / rwlock_t / local_lock
3) raw_spinlock_t / bit spinlock
```

只能外层套内层，不能反过来。而且 **lockdep 在非 RT 内核上也会按这套规则报错**。

</details>

**Q2.** 同步原语的性能排序？

<details><summary>答案</summary>

最快 → 最慢：① atomic 操作（~20ns）。② RCU 读端（~0ns，只禁抢占）。③ seqlock 读端（~10ns）。④ spinlock 无争用（~20ns）。⑤ rwlock 读端无争用（~20ns）。⑥ mutex 无争用（~20ns）。⑦ spinlock 有争用（~100ns-spin）。⑧ mutex 有争用（~1-5us，schedule）。⑨ RCU 写端（~ms，等 grace period）。选择：热路径用 ①-⑤，冷路径可用 ⑥-⑨。

</details>

<details><summary>按 v6.6 修订/补充</summary>

**数字本身要打个折扣看。** 这类「ns 数」高度依赖硬件（CPU 主频、缓存拓扑、
是否跨 NUMA）、内核配置（是否 `CONFIG_PREEMPT`、是否 lockdep）和争用程度。
它们适合做**量级参考和排序参考**，不适合当设计预算。下面逐条订正：

**① 「atomic ~20ns」偏高。**
x86-64 上无争用的 `lock incl` 大致是 **15~20 个周期**，3 GHz 下约 **5~7 ns**。
20 ns 更像是**有争用**或**跨 NUMA** 的数字。
另外 `refcount_inc()` 因为内存序是 relaxed（不插额外屏障），
比一般的 `atomic_inc()` 还要再便宜一点（§5.3）。

**② 「RCU 读端 ~0ns，只禁抢占」需要澄清。**
- 非 RT + `CONFIG_PREEMPT=n`：`rcu_read_lock()` 展开后**就是 `barrier()`**，真的 0 指令
- 非 RT + `CONFIG_PREEMPT=y`：需要操作 `preempt_count`（但不关中断）
- **RT 上**：RCU 读端要拿一把 per-CPU 锁，开销和 spinlock 同量级，不再「免费」

**③ 「seqlock 读端 ~10ns」有个大坑：这是无争用数字。**
seqlock 读端是**重试循环**，一旦有写者并发写，读侧延迟**无界**。
HFT 场景引用这个数字时必须同时说清写者的频率上界（Ch10.8 §9）。

**④ 「mutex 无争用 ~20ns」与「spinlock 无争用 ~20ns」一样是不对的。**
`mutex_lock()` 的 fastpath 是一条 `cmpxchg` + 属主记账，慢于 `spin_lock()`。
而且 `mutex_unlock()` 的 fastpath 也要判等待队列。
同量级但**常数更大**，不应列成同一档。

**⑤ 缺了两类原语的成本：**
- **`local_lock()`**：非 RT、非 debug 下**展开就是 `preempt_disable()`**，
  和 atomic 不在一个量级——它就是一条 `incl` + `barrier()`（§4.1）
- **屏障**（§9.1）：x86 上 `smp_wmb()` / `smp_rmb()` 是 **0 指令**，
  `smp_mb()` 是一条 `lock addl $0,-4(%rsp)`。
  但**在 ARM64 上这些全是真指令**（`dmb ishst` / `dmb ishld`），成本完全不同

**⑥ 排序本身也要加一个维度：不只是快慢，还有「延迟上界可不可推理」（§9.3）。**

| 原语 | 快 | 上界可推理 |
|------|-----|-----------|
| `atomic` | ✅ | ✅ |
| `spinlock` | ✅ | ✅（= 竞争者临界区之和） |
| `mutex`（有 PI） | 中 | ✅ |
| `rwlock` | ✅ | ✅（v3.16+ 非 RT）；❌ RT |
| `seqlock` 读端 | ✅ | ❌ **重试无界** |
| `semaphore` | 中 | ❌ **无 PI，不可推理** |

**HFT 场景下最后一列比第一列重要。** 一个「平均快但有 1% 概率无界」的原语，
在尾延迟约束下是不可接受的。

</details>

**Q3.** HFT 同步原语选型决策树？

<details><summary>答案</summary>

```
热路径？
├─ 是 → 数据可 per-thread？
│       ├─ 是 → 无锁（per-thread 变量）
│       └─ 否 → SPSC 队列？
│               ├─ 是 → atomic<head,tail> + release/acquire
│               └─ 否 → 分片锁 / 无锁哈希表
└─ 否 → 临界区 <1us？
        ├─ 是 → spinlock / atomic
        └─ 否 → mutex（+ rt_mutex 优先级继承）
```

</details>

<details><summary>按 v6.6 修订/补充</summary>

这棵树的方向是对的（先消除共享，再选锁），但缺了**最关键的第 0 步**
和几处 v6.6 的具体对应：

**① 缺第 0 步：「内核里没有 per-thread，只有 per-CPU」。**
原树第一层「数据可 per-thread？→ per-thread 变量」在**内核语境下不成立**——
内核线程没有「线程局部变量」这回事，对应物是 **per-CPU 变量**（`this_cpu_*`）。
而且 per-CPU 数据仍需防**本 CPU 上的抢占 / 中断**重入，
所以内核里的完整写法是：

```c
local_lock(&my_lock);                 /* 或 preempt_disable() + this_cpu 系列 */
p = this_cpu_ptr(&my_var);
...
local_unlock(&my_lock);
```

**② per-CPU 分支漏了 `local_lock`。**
原树直接跳到「无锁」是危险的：per-CPU 只消除了**跨 CPU** 的竞争，
**本 CPU 上的抢占和中断仍然会重入**。v6.6 的正确答案是 `local_lock`（§4）。

**③ 「atomic<head,tail> + release/acquire」应该写得更具体。**
SPSC 队列的完整模式见 Ch10.10 §12（HFT 实践模板），要点是：

```c
/* 生产者 */
WRITE_ONCE(slot[tail & MASK], data);
smp_store_release(&head_or_tail, idx + 1);      /* 发布：先写数据，再发布序号 */

/* 消费者 */
idx = smp_load_acquire(&head_or_tail);          /* 获取：先读序号，再读数据 */
data = READ_ONCE(slot[idx & MASK]);
```

⚠️ 注意 release/acquire **不等于全屏障**（Ch10.10 §9），
它只保证这一对之间的顺序，不保证与其他变量的全局顺序。

**④ 「互斥 + rt_mutex 优先级继承」的写法要订正。**
`rt_mutex` **不是**一个「你显式选用的、带 PI 的 mutex」——
在 v6.6 上，**`mutex` 本身就内建 PI**（它底层就是 rt_mutex 机制）。
所以正确写法是直接用 `mutex_lock()`，而不是「mutex + rt_mutex」。
反过来，真正没有 PI 的是 **semaphore**（§7.1，因为无属主）。

**⑤ 补一个原树没有的分支：写完锁之后的验证（§8）。**

```
选完锁 →
├─ 用 lockdep 断言把上下文假设固化（lockdep_assert_in_task 等）
├─ CONFIG_LOCKDEP=y 跑一遍（验嵌套顺序 + 锁顺序）
├─ CONFIG_KCSAN=y 跑一遍（验数据竞争；它认识屏障）
├─ perf lock / lock tracepoint 量争用
└─ 如果目标平台是 RT：开 CONFIG_PREEMPT_RT 重测（spinlock 语义变了）
```

**⑥ 最重要的一条补充：热路径的目标是「延迟上界可推理」，不只是「快」。**
原树只按快慢分。应该在每个叶节点上再加一问：

```
这个选择的最坏延迟，我能不看别人的代码就算出来吗？
├─ 能 → 采用
└─ 不能 → 继续往下找（通常是「消除共享」那条路）
```

（`seqlock` 读端、无 PI 的 `semaphore`、RT 上的 rwlock 写者，都过不了这一关。）

</details>

**Q4.** v6.6 上 `local_lock` 相比裸 `preempt_disable()` 有什么好处？为什么它是「免费」的？

<details><summary>答案</summary>

**为什么免费**（`include/linux/local_lock_internal.h:14-21, 66-71`）：

```c
#ifndef CONFIG_PREEMPT_RT
typedef struct {
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
	struct task_struct	*owner;
#endif
} local_lock_t;                       /* ⭐ 非 debug 下：0 字节空结构体 */

#define __local_lock(lock)					\
	do {							\
		preempt_disable();				\
		local_lock_acquire(this_cpu_ptr(lock));		\
	} while (0)
```

而且非 debug 下 `local_lock_acquire()` 就是个空函数（`:50`）。
所以在「非 RT + 非 debug」的内核上，`local_lock(&l)` 展开后
**就是纯粹的 `preempt_disable()`**——一个字节的运行时开销都没有。

**那为什么还要用它**（`locktypes.rst:171-180`）：

1. **具名作用域**：`preempt_disable()` 是「scopeless and opaque」——
   读代码的人和静态分析工具看不出哪段数据被谁保护。
   `local_lock(&foo_lock)` 有名有姓。
2. **lockdep 可校验**：开了 lockdep 后它获得 lockmap，能抓到
   「用 preempt_disable 保护 per-CPU 数据的函数被从 softirq 调了」这类 bug
   （`preempt_disable()` 挡不住 softirq！）。
   同时 `lockdep_assert_held(&llock)` 也能用了。
3. **RT 上自动正确**：RT 上它变成 per-CPU spinlock（`:96-99, 108-112`），
   还自动带上 `migrate_disable()` 保证 `this_cpu_ptr()` 的指针有效。

**代价**：RT 上语义变了——`local_lock_irqsave()` 里 **`flags = 0`**（`:118-124`），
**根本不关中断**。所以只适用于「防本 CPU 并发」的语义，
不能用来表达「这里需要中断关闭」。

</details>

**Q5.** 为什么 v6.6 的 rwlock 不会饿死写者，而 RT 上又会？

<details><summary>答案</summary>

**非 RT：qrwlock 排队（v3.16 引入）**

老实现（v3.15，x86 `spinlock.h:222-239`）的不对称：

| | 指令 | 成功条件 |
|---|---|---|
| 读者 | `lock decl`（-1） | ≥ 0 |
| 写者 | `lock subl $RW_LOCK_BIAS`（-0x1000000） | == 0 |

读者只要「≥ 0」就成功，写者必须「计数恰好为 0」。
持续的读者流让计数永远回不到 `0x01000000`，写者永远失败——**饿死**。

新实现的反饥饿机制就是 `_QW_WAITING` 标志（`qrwlock.h:26-31`）：

```c
#define	_QW_WAITING	0x100		/* A writer is waiting	   */
#define	_QW_LOCKED	0x0ff		/* A writer holds the lock */
#define	_QW_WMASK	0x1ff
#define	_QR_SHIFT	9
#define _QR_BIAS	(1U << _QR_SHIFT)
```

v3.16 的 `kernel/locking/qrwlock.c:107-108` 注释：

> Set the waiting flag to **notify readers that a writer is pending**,
> or wait for a previous writer to go away.

写者等锁时先置上 `_QW_WAITING`；新读者看到这个标志就**不再插队**，
而是进等待队列（`:66` "Put the reader into the wait queue"）。
官方定性（`locktypes.rst:299-302`）：**"The implementation is fair, thus preventing writer starvation."**

**RT：写者饥饿反过来了（`locktypes.rst:305-315`）**

> Because an rwlock_t writer **cannot grant its priority to multiple readers**,
> a preempted low-priority reader will continue holding its lock,
> **thus starving even high-priority writers**. In contrast, because readers
> can grant their priority to a writer, a preempted low-priority writer will
> have its priority boosted...

根源是 **PI 只能点对点**：
- 写者 → 阻塞的写者：一对一，可以 boost ✅
- 写者 → N 个并发读者：一对多，无法同时 boost ❌

所以 RT 上的饥饿方向**反转**：低优先级读者饿死高优先级写者。
`rw_semaphore` 在 RT 上有完全相同的问题（`locktypes.rst:147-155`）。

**工程结论**：RT 内核 + 多并发读者 + 需要写侧延迟有界 →
不要用 rwlock，用 **seqlock**（写者只被写者阻塞）或 **RCU**。

</details>

**Q6.** 官方为什么劝退 semaphore？它唯一保留的用途是什么？

<details><summary>答案</summary>

**劝退理由一：无属主 → 无法优先级继承（`locktypes.rst:122-129`）**

> PREEMPT_RT does not change the semaphore implementation because counting
> semaphores have **no concept of owners**, thus preventing PREEMPT_RT from
> providing priority inheritance for semaphores. After all,
> **an unknown owner cannot be boosted**. As a consequence,
> **blocking on semaphores can result in priority inversion**.

对照 `locktypes.rst:86-90`：除 semaphore 外所有锁都有**严格属主语义**
（谁拿谁放）。属主是 PI 的前提——不知道持有者是谁，就没法把它的优先级顶上去。

**劝退理由二：职责混淆（`locktypes.rst:116-120`）**

> Semaphores are often used for **both** serialization and waiting, but
> **new use cases should instead use separate serialization and wait
> mechanisms, such as mutexes and completions**.

| 你要做的事 | 老做法 | v6.6 推荐 |
|---|---|---|
| 互斥 | `down()` / `up()` | **`mutex`**（有 PI） |
| 等事件 | 初值 0 的 semaphore | **`completion`** |
| **计数资源池（N > 1）** | `down()` / `up()` × N | **semaphore（唯一保留用途）** |

**关键判据**：`count > 1` 是 semaphore 的唯一正当理由。
一旦你的 `count` 是 1，它就是在假装 mutex——而且是个**没有 PI 的 mutex**。

**HFT 视角**：这是「延迟上界可推理性」的问题。
- `mutex`（有 PI）：高优先级任务等待时，持有者被 boost，等待时间有界 → 可推理 ✅
- `semaphore`（无 PI）：低优先级持有者可能被任何中优先级任务无限抢占 → **上界不可推理** ❌

</details>

**Q7.** 说说 v6.6 的锁嵌套三层金字塔，以及为什么非 RT 内核上 lockdep 也会按它报错。

<details><summary>答案</summary>

**三层顺序**（`locktypes.rst:513-519`）：

```
  1) Sleeping locks
  2) spinlock_t, rwlock_t, local_lock
  3) raw_spinlock_t and bit spinlocks
```

**推导过程**（`locktypes.rst:511-512`）：

> The fact that PREEMPT_RT changes the lock category of spinlock_t and
> rwlock_t **from spinning to sleeping** and substitutes local_lock with a
> **per-CPU spinlock_t** means that they **cannot be acquired while holding a
> raw spinlock**.

即：顺序是**由 RT 上的类别迁移倒推出来的**。
持有 `raw_spinlock_t`（第 3 层，关抢占、永不睡）时去拿 `spinlock_t`（第 2 层），
在 RT 上就等于「在原子上下文里调用可能睡眠的函数」——非法。

**四条基本规则**（`locktypes.rst:504-514`）：

1. 同类别内可任意嵌套（只要遵守锁顺序规则防死锁）
2. 睡眠锁**不能**嵌在 CPU-local / 自旋锁里
3. CPU-local / 自旋锁**可以**嵌在睡眠锁里
4. 自旋锁可以嵌在所有锁里

**为什么非 RT 上也报错**（`locktypes.rst:520-521`）：

> **Lockdep will complain if these constraints are violated, both in
> PREEMPT_RT and otherwise.**

这是刻意设计：让非 RT 上的开发也能提前发现 RT 兼容性问题。
否则开发者要等到切到 RT 内核才炸，排查成本高得多。

**两个典型违规**（`locktypes.rst:333-345, 396-410`）：

```c
/* ❌ A：RT 上 local_lock_irq 是 per-CPU spinlock，既不关中断也不关抢占 */
local_lock_irq(&local_lock);
raw_spin_lock(&lock);            /* 第 3 层套在第 2 层外面 → 违规 */
/* ✅ 改： */
local_lock_irq(&local_lock);
spin_lock(&lock);                /* 都在第 2 层 */

/* ❌ B：RT 上 spinlock 是 rt_mutex，要求完全可抢占上下文 */
local_irq_disable();
spin_lock(&lock);
/* ✅ 改： */
spin_lock_irq(&lock);            /* 让实现自己决定怎么拆 */
```

**HFT 视角**：这条规则和性能无关，是**正确性**约束。
违反它的代码在非 RT 上可能一直跑得好好的，
一旦切到 RT（低延迟场景的常见选择）就直接炸——
属于最贵的一类技术债。

</details>

**Q8.** `refcount_t` 的饱和语义是怎么防住 use-after-free 的？为什么选 `0xc000_0000`？

<details><summary>答案</summary>

**问题背景**：`atomic_t` 溢出会回绕。`INT_MAX + 1 = INT_MIN`（负数），
而引用计数的释放逻辑通常是「`dec_and_test` 到 0 就释放」。
回绕后计数继续减，可能再次经过 0 → **重复释放 / UAF**。
攻击者只要能反复触发 `refcount_inc()` 就能把计数推到回绕。

**饱和语义**（`refcount.h:11-14`）：

> the counter **saturates at REFCOUNT_SATURATED** and will not move once there.
> This avoids wrapping the counter and causing 'spurious' use-after-free issues.

一旦检测到上溢/下溢，把计数**钉死**在 `0xc000_0000`，从此不再变动。
攻击者没法把它搞回 0，也就触发不了释放。

**为什么是 `0xc000_0000`**（`:21-28`）：

```
   0                          INT_MAX     REFCOUNT_SATURATED   UINT_MAX
  (0x0000_0000)            (0x7fff_ffff)    (0xc000_0000)    (0xffff_ffff)
   +-------------------------------+----------------+----------------+
                                    <---------- bad value! ---------->
                                         (有符号视角下 = 负数)
```

> by placing REFCOUNT_SATURATED roughly **equidistant from 0 and INT_MAX**
> we minimise the scope for error

饱和检测**不是原子的**（先 `atomic_fetch_add_relaxed`，再判断，再 `atomic_set`），
存在 race：两个线程同时溢出时，中间可能有人继续加减。
把饱和值放在 **0 和 INT_MAX 的中点**，意味着要「逃逸」出饱和区
需要把计数从 `0xc000_0000` 一路推到 0，跨度 `0x40000000`。

**安全性论证**（`:44-52`）：

```
(UINT_MAX+1-REFCOUNT_SATURATED) / PID_MAX_LIMIT =
0x40000000 / 0x400000 = 0x100 = 256
```

`PID_MAX_LIMIT = 0x400000`（受 `FUTEX_TID_MASK` 限制，不能轻易再调大）。
攻击者需要在单个 task 内**嵌套 256 次以上**引用才能逃出饱和区，
再叠加卡准调度时机连续命中同一个 race——
文档结论：**"there doesn't appear to be a practical avenue of attack"**。

⚠️ **但有个前提**：`"if no batched refcounting operations are used"`。
用 `refcount_add(1000, &r)` 这类大批量加减时，这个论证不成立——
这是为什么接口会被刻意收紧（只暴露引用计数该用的函数）。

**顺带一个性能收益**（`:60-63`）：

> The increments are **fully relaxed**; these will not provide ordering.

`refcount_inc()` 不插额外屏障（只保留原子性本身必需的语义），
比一般的 `atomic_inc()` 更便宜。理由是「拿到对象这个动作本身已提供序」
（锁的 acquire，或 RCU 的依赖加载）。

</details>

</details>

---

### 快速索引：本篇的 Ch10 交叉引用

| 主题 | 本篇章节 | 详见 |
|------|---------|------|
| 原子操作 / refcount | §5 | [10.1](section-10.1-原子操作.md) |
| 自旋锁 / raw 锁 | §1.2, §2 | [10.2](section-10.2-自旋锁.md) |
| 读写自旋锁（qrwlock 断崖） | §3 | [10.3](section-10.3-读-写自旋锁.md) |
| 信号量（劝退理由） | §7 | [10.4](section-10.4-信号量.md) |
| 互斥体 / PI / OSQ | §1.4, §6.2 | [10.5](section-10.5-互斥体.md) |
| 完成变量 | §7 | [10.6](section-10.6-完成变量.md) |
| 大内核锁（迁移原则） | §10 模板 B | [10.7](section-10.7-大内核锁.md) |
| 顺序锁（RT 活锁 / seqlock 无界重试） | §3.6, §9.2 | [10.8](section-10.8-顺序锁.md) |
| 禁止抢占 / migrate_disable / guard() | §4 | [10.9](section-10.9-禁止抢占.md) |
| 排序和屏障 / SPSC 模板 | §9.4 | [10.10](section-10.10-排序和屏障.md) |

→ [Ch 9](../../chapter-09-kernel-sync-intro/) · [Ch 7–8](../../chapter-07-interrupts/) · 本章 README 小结表

> ↔ [ULK Ch5 §7 选型与实例](../../../16-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-7-选型与实例.md)
---
