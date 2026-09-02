## ③ 读-写自旋锁 · Reader-Writer Spin Locks

允许多个读者 **并行** 持锁，写者 **独占**。仍是自旋锁家族 — **读者/写者都不能睡眠**。

| 角色 | 规则 |
|------|------|
| **读者** | 可与其他读者共存；见写者则等 |
| **写者** | 独占；等所有读者/写者离开 |

> **本篇分工**：实体书只讲了 rwlock 的语义和"偏袒读者"这个特性。本篇**不复述**语义，
> 只做四件事，全部用 v6.6 源码实证：
>
> ① 拆开 `rwlock_t` → `arch_rwlock_t` → `struct qrwlock`，给出**完整位布局**
> （书上那个"计数器从 0x01000000 往下减"的方案在 v6.6 上**已经不存在了**）；
> ② ⭐ **订正"偏袒读者"** —— v6.6 的 qrwlock 是 **FIFO 公平的、写者优先**的，
> 靠 `_QW_WAITING` 标志位 + 一把内嵌的 qspinlock 实现。原来的"写者饥饿"结论**不成立**；
> ③ 指出**唯一的"插队"例外**：中断上下文的读者不排队，直接自旋（并给出它为什么必须这样）；
> ④ 引 `Documentation/locking/spinlocks.rst` 的官方原话：**"除非读临界区很长，
> 否则你不如直接用 spinlock"** —— 这条和"读多写少就用 rwlock"的直觉**相反**。
>
> 所有常量与代码均核对自缓存的 v6.6 源码，行号可查。

---

## 1. `rwlock_t` 在 v6.6 里是 `struct qrwlock`，**8 个字节**

书的年代（v2.6）`rwlock_t` 就一个 32 位计数器加 `RW_LOCK_BIAS`。现在完全不同：

```c
/* include/asm-generic/qrwlock_types.h —— v6.6 全文核心 */
typedef struct qrwlock {
	union {
		atomic_t cnts;            /* 4 字节：读者计数 + 写者标志 */
		struct {
#ifdef __LITTLE_ENDIAN
			u8 wlocked;           /* 只取 cnts 的最低 1 字节 */
			u8 __lstate[3];
#else
			u8 __lstate[3];
			u8 wlocked;
#endif
		};
	};
	arch_spinlock_t		wait_lock;    /* 4 字节：一把完整的 qspinlock！ */
} arch_rwlock_t;

#define	__ARCH_RW_LOCK_UNLOCKED {		\
	{ .cnts = ATOMIC_INIT(0), },		\
	.wait_lock = __ARCH_SPIN_LOCK_UNLOCKED,	\
}
```

而 x86 直接就吃这个 asm-generic 版本，**没有自己的 rwlock 实现**：

```c
/* arch/x86/include/asm/spinlock_types.h —— v6.6 全文，只有 253 字节 */
#include <linux/types.h>
#include <asm-generic/qspinlock_types.h>
#include <asm-generic/qrwlock_types.h>
```

> 顺带一个可验证的小事实：`arch/x86/include/asm/rwlock.h` 在 v6.6 **不存在**
> （抓取返回 "Couldn't find the requested file"）。x86 的 rwlock 走
> `arch/x86/include/asm/spinlock.h` 里的 `#include <asm/qrwlock.h>`
> → `include/asm-generic/qrwlock.h`。

### 位布局

```c
/* include/asm-generic/qrwlock.h */
#define	_QW_WAITING	0x100		/* A writer is waiting	   */
#define	_QW_LOCKED	0x0ff		/* A writer holds the lock */
#define	_QW_WMASK	0x1ff		/* Writer mask		   */
#define	_QR_SHIFT	9		/* Reader count shift	   */
#define _QR_BIAS	(1U << _QR_SHIFT)   /* = 512 */
```

```
 31                        9  8    0
┌──────────────────────────┬─┬─────┐
│      reader count        │W│ 写者 │   cnts (4 bytes)
│   (23 bits, bias = 512)  │A│ 状态 │
└──────────────────────────┴─┴─────┘
                            │  └─ bit 0-7 : _QW_LOCKED  (0x0ff) 写者持锁
                            └──── bit 8   : _QW_WAITING (0x100) 写者在等
 ┌────────────────────────────────┐
 │ arch_spinlock_t wait_lock      │   另外 4 字节，一把完整的 qspinlock
 └────────────────────────────────┘
```

| 项 | 值 | 说明 |
|----|----|------|
| `_QR_BIAS` | `1 << 9` = **512** | 每个读者给计数值加 512 |
| 最大并发读者 | `2^23 - 1` = **8,388,607** | 计数位有 23 位 |
| `_QW_LOCKED` | `0x0ff` | 写者持锁标志（占满低字节） |
| `_QW_WAITING` | `0x100` | ⭐ **写者在等** —— 公平性的关键 |
| `_QW_WMASK` | `0x1ff` | 写者掩码 = LOCKED \| WAITING |
| 锁大小 | **8 字节** | 非 debug、非 RT |

> 注意 `wlocked` 是 `u8`，和 `struct qspinlock` 里 `locked` 给整整一个字节是**同一个套路**：
> 让写者解锁退化成一次**单字节 store**。

---

## 2. 快路径：读者加 512，写者一次 CAS

```c
/* include/asm-generic/qrwlock.h */
static inline void queued_read_lock(struct qrwlock *lock)
{
	int cnts;

	cnts = atomic_add_return_acquire(_QR_BIAS, &lock->cnts);
	if (likely(!(cnts & _QW_WMASK)))
		return;

	/* The slowpath will decrement the reader count, if necessary. */
	queued_read_lock_slowpath(lock);
}

static inline void queued_write_lock(struct qrwlock *lock)
{
	int cnts = 0;
	/* Optimize for the unfair lock case where the fair flag is 0. */
	if (likely(atomic_try_cmpxchg_acquire(&lock->cnts, &cnts, _QW_LOCKED)))
		return;

	queued_write_lock_slowpath(lock);
}

static inline void queued_read_unlock(struct qrwlock *lock)
{
	(void)atomic_sub_return_release(_QR_BIAS, &lock->cnts);
}

static inline void queued_write_unlock(struct qrwlock *lock)
{
	smp_store_release(&lock->wlocked, 0);   /* 只写 1 个字节 */
}
```

三个观察：

1. **读者是"先加后查"**：`atomic_add_return_acquire(_QR_BIAS, ...)` 先把计数加上去，
   再看有没有写者。有的话慢路径会**把计数减回来**。这个乐观 + 回滚的模式
   和慢路径里的 `atomic_sub(_QR_BIAS, ...)` 是配对的。
2. **写者快路径要求 `cnts == 0`**（`atomic_try_cmpxchg_acquire(&cnts, &0, _QW_LOCKED)`），
   即"既没有读者也没有写者"。比读者的"只要没写者就行"严格得多。
3. **写者解锁只写 1 字节**（`&lock->wlocked`），而**读者解锁是一次原子 RMW**
   （`atomic_sub_return_release`）。这就是官方文档那句"rwlock 比 spinlock
   需要更多原子操作"的来源 —— 见 §6。

### trylock 也是先加后查

```c
static inline int queued_read_trylock(struct qrwlock *lock)
{
	int cnts;

	cnts = atomic_read(&lock->cnts);
	if (likely(!(cnts & _QW_WMASK))) {
		cnts = (u32)atomic_add_return_acquire(_QR_BIAS, &lock->cnts);
		if (likely(!(cnts & _QW_WMASK)))
			return 1;
		atomic_sub(_QR_BIAS, &lock->cnts);      /* 回滚 */
	}
	return 0;
}
```

⚠️ 判据是 `_QW_WMASK = 0x1ff`，**同时覆盖 `_QW_LOCKED` 和 `_QW_WAITING`**。
也就是说：**只要有写者在等，`read_trylock()` 就失败。** 这是公平性的第一道闸。

---

## 3. ⭐ 订正：v6.6 的 rwlock **不是偏袒读者**，是 FIFO 公平的

旧笔记（以及书上）写的是：

> 持续有新读者进入 → **写者可能长时间拿不到锁**（写者饥饿）

**这在 v6.6 的 qrwlock 上不成立。** 看两个慢路径：

### 写者慢路径

```c
/* kernel/locking/qrwlock.c */
void __lockfunc queued_write_lock_slowpath(struct qrwlock *lock)
{
	int cnts;

	trace_contention_begin(lock, LCB_F_SPIN | LCB_F_WRITE);

	/* Put the writer into the wait queue */
	arch_spin_lock(&lock->wait_lock);            /* ① 先占住队列（那把内嵌 qspinlock） */

	/* Try to acquire the lock directly if no reader is present */
	if (!(cnts = atomic_read(&lock->cnts)) &&
	    atomic_try_cmpxchg_acquire(&lock->cnts, &cnts, _QW_LOCKED))
		goto unlock;

	/* Set the waiting flag to notify readers that a writer is pending */
	atomic_or(_QW_WAITING, &lock->cnts);         /* ② ⭐ 挂出"写者在等"的牌子 */

	/* When no more readers or writers, set the locked flag */
	do {
		cnts = atomic_cond_read_relaxed(&lock->cnts, VAL == _QW_WAITING);  /* ③ 等读者排空 */
	} while (!atomic_try_cmpxchg_acquire(&lock->cnts, &cnts, _QW_LOCKED));
unlock:
	arch_spin_unlock(&lock->wait_lock);
	trace_contention_end(lock, 0);
}
```

关键点：**写者整个等待期间一直握着 `wait_lock`**。

### 读者慢路径

```c
void __lockfunc queued_read_lock_slowpath(struct qrwlock *lock)
{
	/*
	 * Readers come here when they cannot get the lock without waiting
	 */
	if (unlikely(in_interrupt())) {                 /* ⭐ 唯一例外，见 §4 */
		atomic_cond_read_acquire(&lock->cnts, !(VAL & _QW_LOCKED));
		return;
	}
	atomic_sub(_QR_BIAS, &lock->cnts);              /* 把乐观加上的 512 还回去 */

	trace_contention_begin(lock, LCB_F_SPIN | LCB_F_READ);

	/* Put the reader into the wait queue */
	arch_spin_lock(&lock->wait_lock);               /* ⭐ 排队 —— 排在写者【后面】 */
	atomic_add(_QR_BIAS, &lock->cnts);              /* 重新加上 512 */

	/*
	 * The ACQUIRE semantics of the following spinning code ensure
	 * that accesses can't leak upwards out of our subsequent critical
	 * section in the case that the lock is currently held for write.
	 */
	atomic_cond_read_acquire(&lock->cnts, !(VAL & _QW_LOCKED));

	/* Signal the next one in queue to become queue head */
	arch_spin_unlock(&lock->wait_lock);
	trace_contention_end(lock, 0);
}
```

### 公平性是怎么成立的（三步连锁）

```
  时间线
   ─────────────────────────────────────────────────────────────►
  W 到达 ──► arch_spin_lock(&wait_lock)   【握住队列】
          ──► atomic_or(_QW_WAITING)      【挂牌子：写者在等】
          ──► 自旋等 cnts == _QW_WAITING（即读者排空）

  R1 到达 ─► atomic_add(512)
          ─► cnts & _QW_WMASK != 0  （WAITING 位亮了）
          ─► 慢路径：atomic_sub(512)      【还回去】
          ─► arch_spin_lock(&wait_lock) ──► 阻塞！卡在 W 后面
                                             ▲
                                             └── W 正握着它

  R2、R3 同理，全部排在 W 后面
```

三道闸门保证写者不被饿死：

| 闸门 | 位置 | 作用 |
|------|------|------|
| ① `_QW_WAITING` 位 | 写者慢路径 `atomic_or` | 让**新读者的快路径直接失败** |
| ② 读者回滚计数 | 读者慢路径 `atomic_sub(_QR_BIAS, ...)` | 读者不再"赖"在计数里 |
| ③ 读者去抢 `wait_lock` | 读者慢路径 `arch_spin_lock(&wait_lock)` | 排队在写者**之后**（FIFO） |

### 源码自己的说法

`include/asm-generic/qrwlock.h` 头部注释逐字：

> *These use generic atomic and locking routines, but **depend on a fair spinlock
> implementation in order to be fair themselves**.*

一句话点破：qrwlock 的公平性**不是自己实现的，是借来的** —— 借的是那把内嵌的
`arch_spinlock_t wait_lock`（qspinlock，本身是排队锁，见 10.2）。

**工程结论**：v6.6 上可以放心用 `rwlock_t`，不用担心"写者饥饿"。
（真正要担心的是 §6 的性能账。）

---

## 4. ⭐ 唯一的"插队"：中断上下文的读者不排队

读者慢路径开头有个特判：

```c
	if (unlikely(in_interrupt())) {
		/*
		 * Readers in interrupt context will get the lock immediately
		 * if the writer is just waiting (not holding the lock yet),
		 * so spin with ACQUIRE semantics until the lock is available
		 * without waiting in the queue.
		 */
		atomic_cond_read_acquire(&lock->cnts, !(VAL & _QW_LOCKED));
		return;
	}
```

### 为什么必须这样：否则会死锁

设想一个读者**不**走这个特判：

```
task 上下文:  read_lock() ──► 慢路径 ──► arch_spin_lock(&wait_lock)  【握住队列】
                    │
              本地中断打进来
                    │
ISR:          read_lock() ──► 慢路径 ──► arch_spin_lock(&wait_lock)
                    │
              ISR 自旋等 task 释放 wait_lock
              task 永远等 ISR 返回才能继续
                    ▼
                  死锁
```

所以中断里来的读者**必须绕过队列**，只在 `_QW_LOCKED`（写者是否**真的持锁**）上自旋。

### 这个例外带来的后果

| 项 | 说明 |
|----|------|
| 允许的插队 | ISR 里的读者只看 `_QW_LOCKED`，**不看 `_QW_WAITING`** |
| 后果 | 如果 `wait_lock` 队列里有写者在等、但还没拿到锁，ISR 读者**可以插进去** |
| 严格程度 | 只挡"写者已持锁"，不挡"写者在等" |
| 死锁避免 | 这是刻意的设计代价 —— 用轻微的不公平换掉一个必现死锁 |

**推论**：如果你的 ISR 里会 `read_lock()`，那么严格的 FIFO 公平性**不成立**，
写者仍可能被密集的中断读者延迟。要严格公平，得让 ISR 侧的读者改成
在进程上下文（如 tasklet/workqueue）里拿锁 —— 但那又引入了延迟。

> ⚠️ 这也解释了 `arch/x86/include/asm/spinlock.h` 里那段注释的意思：
> ```c
>  * NOTE! it is quite common to have readers in interrupts
>  * but no interrupt writers. For those circumstances we
>  * can "mix" irq-safe locks - any writer needs to get a
>  * irq-safe write-lock, but readers can get non-irqsafe
>  * read-locks.
> ```
> 即：读者可以不带 irqsave（因为读者之间不互斥），但**写者必须带** ——
> 否则 ISR 里的读者会和进程上下文的写者撞上。

---

## 5. 版本断崖：老的 `RW_LOCK_BIAS` 方案早没了

| 版本 | x86 rwlock 实现 | 证据 |
|------|----------------|------|
| v3.10 | `RW_LOCK_BIAS` 计数器 + 手写汇编 | `arch_write_lock`：`WRITE_LOCK_SUB(%1) "(%0)"`、`call __write_lock_failed` |
| v4.1 | **已换成 qrwlock** | `arch/x86/include/asm/spinlock.h:210`：*"On x86, we implement read-write locks using the generic qrwlock"* |
| v4.2 | x86 `select ARCH_USE_QUEUED_RWLOCKS` | `arch/x86/Kconfig` 从 v4.2 起出现该符号（v4.1 计数为 0） |
| v6.6 | 仍为 qrwlock | `asm/spinlock_types.h` 只有 3 行 include |

v3.10 老实现的形状（可验证部分）：

```c
/* arch/x86/include/asm/spinlock.h @ v3.10 */
static inline void arch_read_lock(arch_rwlock_t *rw)
{
	asm volatile(LOCK_PREFIX READ_LOCK_SIZE(dec) " (%0)\n\t"
		     "jns 1f\n"
		     "call __read_lock_failed\n\t"
		     "1:\n"
		     ::LOCK_PTR_REG (rw) : "memory");
}

static inline void arch_write_lock(arch_rwlock_t *rw)
{
	asm volatile(LOCK_PREFIX WRITE_LOCK_SUB(%1) "(%0)\n\t"
		     "jz 1f\n"
		     "call __write_lock_failed\n\t"
		     "1:\n"
		     ::LOCK_PTR_REG (&rw->write), "i" (RW_LOCK_BIAS)
		     : "memory");
}
```

```c
/* RW_LOCK_BIAS 定义（v3.10） */
#define RW_LOCK_BIAS		0x00100000        /* x86_32 */
#define RW_LOCK_BIAS		(_AC(1,L) << 32)  /* x86_64 */
```

> ⚠️ **存疑标注（诚实起见）**：老方案到底是不是"偏袒读者、写者会饥饿"，
> 取决于 `READ_LOCK_SIZE` 这个宏展开成什么后缀（`b`/`w`/`l`/`q`），
> 它决定读者的 `dec` 只影响哪几个字节 —— 而写者减去的是 64 位的 `RW_LOCK_BIAS`。
> **本次核对未定位到 `READ_LOCK_SIZE` 的定义位置**（v3.10 的
> `arch/x86/include/asm/spinlock.h` 里只有使用处和文件末尾的 `#undef`，
> 定义应在 `asm/rwlock.h` 或更早的 include 中）。因此**不下断言**。
>
> 可以确定的是：① 老方案**没有** `wait_lock` 队列，读者失败后进
> `__read_lock_failed` 自旋重试，**不存在"排队"概念**；
> ② v6.6 的 qrwlock 有明确的 `_QW_WAITING` + 队列，公平性**有源码可证**。
> 需要坐实老方案时，下一步应抓 `arch/x86/include/asm/rwlock.h@v3.10` 全文。

---

## 6. ⭐ 官方建议：**除非读临界区很长，否则别用 rwlock**

`Documentation/locking/spinlocks.rst` 的原话（Lesson 2: reader-writer spinlocks）：

> *NOTE! reader-writer locks require **more atomic memory operations** than
> simple spinlocks. **Unless the reader critical section is long, you
> are better off just using spinlocks.***

这条和"读多写少就用 rwlock"的流行直觉**相反**。为什么？

### 算一笔原子操作账

| 操作 | 普通 `spinlock_t` | `rwlock_t`（读者） |
|------|------------------|-------------------|
| 加锁 | 1 次 `cmpxchg` | 1 次 `atomic_add_return`（**RMW**） |
| 解锁 | 1 次 **普通 store**（单字节，**非原子**） | 1 次 `atomic_sub_return`（**RMW**） |
| 争用时 | 自旋在**自己的** MCS node（本地 cacheline，O(N)） | 回滚计数 + 抢 `wait_lock` + 重新加计数 |

关键差异：

1. **普通 spinlock 的解锁不是原子操作**（`smp_store_release(&lock->locked, 0)`，
   一次普通单字节 store）。**rwlock 的读解锁是原子 RMW**（`atomic_sub_return_release`），
   要独占 cacheline，多核下必然 cacheline 乒乓。
2. **普通 spinlock 争用时每人自旋在自己的 MCS node**（见 10.2，O(N)）；
   rwlock 的读者争用时全都在**同一把 `wait_lock`** 上排队（虽然是 qspinlock，
   但仍是全局竞争点，且读者是"排到写者后面"而不是"和写者并行"）。
3. 所以：**读者多 ≠ 用 rwlock 划算**。只有当"读临界区足够长，
   长到多个读者并行执行省下的时间 > 额外原子操作的开销"时才划算。

### 另外两条官方硬约束

```rst
Also, you cannot "upgrade" a read-lock to a write-lock, so if you at _any_
time need to do any changes (even if you don't do it every time), you have
to get the write-lock at the very beginning.
```

> **不能把读锁"升级"成写锁。** 只要代码里**任何一条路径**可能要改数据，
> 就必须**从一开始就拿写锁**。想"先读着，发现要改再升级"会直接死锁
> （`write_lock()` 等所有读者退出，而你自己就是那个读者）。

```rst
   NOTE! RCU is better for list traversal, but requires careful
   attention to design detail (see Documentation/RCU/listRCU.rst).
```

> 遍历链表，**RCU 比 rwlock 好**。

### 选型决策表（综合以上）

| 场景 | 推荐 | 理由 |
|------|------|------|
| 读临界区**很短**（几条~几十条指令） | **普通 `spinlock_t`** | 官方文档原话，原子操作更少 |
| 读临界区**长**、写极少 | `rwlock_t` | 并发读者的收益盖过开销 |
| 链表遍历、读极多 | **RCU** | 读端零原子操作，官方明确推荐 |
| 写者必须**立刻**可见、读者极多 | **seqlock**（10.8） | 读者无锁，靠序号重试 |
| 任何路径可能要改数据 | **直接拿写锁** / 换机制 | 不能升级读锁 |
| RT 内核 | 见 §7 —— `rwlock_t` 会睡眠 | |

---

## 7. PREEMPT_RT：`rwlock_t` 也变成可睡眠锁

和 `spinlock_t` 一样，RT 下 `rwlock_t` 也换了实现：

```c
/* include/linux/rwlock_types.h */
#ifndef CONFIG_PREEMPT_RT
typedef struct {
	arch_rwlock_t raw_lock;                  /* 非 RT：struct qrwlock，8 字节 */
#ifdef CONFIG_DEBUG_SPINLOCK
	unsigned int magic, owner_cpu;
	void *owner;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map dep_map;
#endif
} rwlock_t;

#define RWLOCK_MAGIC		0xdeaf1eed

#else /* !CONFIG_PREEMPT_RT */
#include <linux/rwbase_rt.h>

typedef struct {
	struct rwbase_rt	rwbase;
	atomic_t		readers;
	...
} rwlock_t;
#endif
```

```c
/* include/linux/rwbase_rt.h:11 */
struct rwbase_rt {
	atomic_t		readers;
	struct rt_mutex_base	rtmutex;
};

#define __RWBASE_INITIALIZER(name)	\
{					\
	.readers = ATOMIC_INIT(READER_BIAS),	\
	.rtmutex = __RT_MUTEX_BASE_INITIALIZER(name.rtmutex),	\
}
```

```c
/* include/linux/rwlock_rt.h:35,82 */
static __always_inline void read_lock(rwlock_t *rwlock)  { rt_read_lock(rwlock); }
static __always_inline void write_lock(rwlock_t *rwlock) { rt_write_lock(rwlock); }
```

对照记忆：`SPINLOCK_MAGIC = 0xdead4ead`，`RWLOCK_MAGIC = 0xdeaf1eed`（都是 16 进制英文彩蛋）。

| 配置 | `read_lock()` / `write_lock()` |
|------|-------------------------------|
| 非 RT | 真自旋（qrwlock），不可睡眠 |
| **PREEMPT_RT** | `rt_read_lock()` / `rt_write_lock()`，**可睡眠** |

> 和 `spinlock_t` 的对应关系完全一样：想要 RT 上也不睡的 rwlock，
> 需要用 `raw_*` 那一族。

---

## 8. API 层：和 spinlock 同构

```c
/* include/linux/rwlock_api_smp.h:147 */
static inline void __raw_read_lock(rwlock_t *lock)
{
	preempt_disable();
	rwlock_acquire_read(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_read_trylock, do_raw_read_lock);
}

static inline unsigned long __raw_read_lock_irqsave(rwlock_t *lock)
{
	unsigned long flags;
	local_irq_save(flags);
	preempt_disable();
	rwlock_acquire_read(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_read_trylock, do_raw_read_lock);
	return flags;
}

static inline void __raw_read_lock_bh(rwlock_t *lock)
{
	__local_bh_disable_ip(_RET_IP_, SOFTIRQ_LOCK_OFFSET);
	rwlock_acquire_read(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_read_trylock, do_raw_read_lock);
}
```

和 10.2 的 spinlock 完全同构，两点相同：

1. **`read_lock()` 也是 `preempt_disable()` + `LOCK_CONTENDED(try, lock)`** ——
   跟 `spin_lock()` 一模一样的结构。
2. **`read_lock_bh()` 没有显式 `preempt_disable()`** —— `__local_bh_disable_ip()`
   隐含禁抢占（和 `spin_lock_bh()` 同理）。

| API | 作用 |
|-----|------|
| `read_lock()` / `read_unlock()` | 共享读 |
| `write_lock()` / `write_unlock()` | 独占写 |
| `read_trylock()` / `write_trylock()` | 失败立即返回，不阻塞 |
| `*_irqsave()` / `*_irq()` / `*_bh()` | 与 spinlock 同义 |
| `read_lock_irqsave()` | 读者**通常**不需要（读者之间不互斥） |
| `write_lock_irqsave()` | ⚠️ **写者一般需要** —— 见 §4 那条 x86 注释 |

---

## HFT / 嵌入式关联

| 现象 | 机制解释 | 应对 |
|------|---------|------|
| **rwlock 读多反而更慢** | 读解锁是**原子 RMW**，多核下 cacheline 乒乓；普通 spinlock 解锁只是普通 store | 读临界区短就用 spinlock（官方原话） |
| **写者延迟不可控** | qrwlock 虽有 `_QW_WAITING` 保公平，但**中断里的读者会插队** | ISR 里不要 `read_lock()` 热路径；改 tasklet 或直接换 RCU |
| **读者全部挤在 `wait_lock`** | 争用时读者回滚计数后统一排到内嵌 qspinlock 上 | 高并发读用 RCU（读端零原子操作） |
| **想"先读后改"会死锁** | 读锁无法升级为写锁 | 一开始就拿写锁，或改用 seqlock / RCU |
| **RT 上 rwlock 会睡** | `rwlock_t` = `rwbase_rt`（`atomic_t readers` + `rt_mutex_base`） | 延迟敏感路径用 raw 族 |
| 行情快照 + 偶发配置更新 | 想当然选 rwlock | 先看读侧临界区长度；配置更新用**双缓冲 + atomic 换指针**往往更好 |

**嵌入式侧**：`rwlock_t` 在 UP 上和 `spinlock_t` 一样退化成 `preempt_disable()`
（`rwlock_api_up.h` 里 `_raw_read_lock(lock)` → `__LOCK(lock)`）。
单核上用 rwlock **没有任何并发收益**（本来就不可能有两个读者真并行），
只有额外的原子操作开销 —— **UP 系统上一律用普通 spinlock**。

---

## 实践模板

```c
#include <linux/rwlock.h>

struct route_table {
	rwlock_t        lock;
	struct route   *head;
	unsigned int    nr;
};

static DEFINE_RWLOCK(g_rt_lock);
static struct route_table g_rt;

static void rt_init(struct route_table *t)
{
	rwlock_init(&t->lock);        /* 动态初始化 */
	t->head = NULL;
	t->nr = 0;
}

/* 读侧：临界区短的话，官方建议直接改用 spinlock */
static struct route *rt_lookup(unsigned int key)
{
	struct route *r;
	unsigned long flags;

	read_lock_irqsave(&g_rt_lock, flags);
	for (r = g_rt.head; r; r = r->next) {
		if (r->key == key)
			break;
	}
	read_unlock_irqrestore(&g_rt_lock, flags);
	return r;   /* ⚠️ 出了锁指针已不受保护 —— 需要 RCU 或引用计数 */
}

/* 写侧：必须 irqsave —— ISR 里可能有读者 */
static int rt_update(unsigned int key, struct route *new_r)
{
	unsigned long flags;
	int ret = -ENOENT;

	write_lock_irqsave(&g_rt_lock, flags);
	/* 一旦进写锁可以做任何修改；但注意【不能】从 read_lock 升级过来 */
	...
	write_unlock_irqrestore(&g_rt_lock, flags);
	return ret;
}
```

**易错点核对表**：

| ❌ 错误 | ✅ 正确 |
|--------|--------|
| "读多写少就用 rwlock" | 读**临界区长度**才是判据，官方原话：短临界区不如用 spinlock |
| 先 `read_lock()` 再 `write_lock()` 想升级 | 死锁。要改就从一开始拿写锁 |
| 认为 v6.6 上写者会饥饿 | qrwlock 有 `_QW_WAITING` + `wait_lock` 队列，是公平的 |
| ISR 里 `read_lock()` 还指望严格 FIFO | 中断读者不排队，只看 `_QW_LOCKED`（为避死锁的刻意设计） |
| 读者用 `read_lock()`、写者用 `write_lock()`（都不带 irq 保护） | 写者一般需要 `write_lock_irqsave()` |
| UP 系统上用 rwlock 图"读者并行" | UP 上没有真并行，rwlock 只有额外开销 |
| RT 内核上依赖 rwlock 不睡眠 | RT 下 `rwlock_t` = `rwbase_rt`，可睡眠 |

---

→ [10.2 spinlock](./section-10.2-自旋锁.md) · [10.5 互斥体](./section-10.5-互斥体.md) · [10.8 seqlock](./section-10.8-顺序锁.md) · [10.11 选型](./section-10.11-选型速查Ch-9--Ch-10.md)

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 读写自旋锁（rwlock）适合什么场景？有什么缺点？

<details><summary>答案</summary>

适合：读多写少（如路由表/配置表），多个读者可并发。缺点：① 写者饥饿：如果有持续的新读者进来，
写者可能无限等待。② 读端仍有原子操作开销（递增 reader count）。③ 公平性：部分实现有公平性保证
（防止写者饥饿），但会降低读吞吐。现代内核更推荐 RCU（读端零开销）替代 rwlock。

<details><summary>按 v6.6 修订/补充</summary>

**① "写者饥饿"这条在 v6.6 上不成立，需要订正。**

v6.6 的 x86 rwlock 是 `struct qrwlock`（`asm-generic/qrwlock_types.h`），
公平性靠三道闸门（详见 §3）：

| 闸门 | 代码位置 | 效果 |
|------|---------|------|
| 写者挂 `_QW_WAITING` 位 | `queued_write_lock_slowpath()`：`atomic_or(_QW_WAITING, &lock->cnts)` | 新读者的快路径直接失败 |
| 读者回滚计数 | `queued_read_lock_slowpath()`：`atomic_sub(_QR_BIAS, &lock->cnts)` | 读者不"赖"在计数里 |
| 读者去排 `wait_lock` | `queued_read_lock_slowpath()`：`arch_spin_lock(&lock->wait_lock)` | 排在写者**之后**（FIFO） |

而写者**整个等待期间都握着 `wait_lock`**：

```c
	arch_spin_lock(&lock->wait_lock);              /* ① 握住队列 */
	atomic_or(_QW_WAITING, &lock->cnts);           /* ② 挂牌子 */
	do {
		cnts = atomic_cond_read_relaxed(&lock->cnts, VAL == _QW_WAITING);
	} while (!atomic_try_cmpxchg_acquire(&lock->cnts, &cnts, _QW_LOCKED));
	arch_spin_unlock(&lock->wait_lock);            /* 拿到锁才放 */
```

源码自己的注释（`asm-generic/qrwlock.h` 头部）：
*"These use generic atomic and locking routines, but **depend on a fair spinlock
implementation in order to be fair themselves**."* —— 公平性是"借"内嵌 qspinlock 的。

**唯一的例外**：中断上下文的读者**不排队**（`if (unlikely(in_interrupt()))` 分支直接
自旋在 `_QW_LOCKED` 上），所以 ISR 读者仍可插队。这是为规避死锁的刻意设计（§4）。

**② "读端有原子操作开销"这条对，但理由要说准**：
`queued_read_lock()` = `atomic_add_return_acquire(_QR_BIAS, &lock->cnts)`，
`queued_read_unlock()` = `atomic_sub_return_release(_QR_BIAS, &lock->cnts)`，
**加解锁都是原子 RMW**。对比普通 spinlock：**解锁只是 `smp_store_release(&lock->locked, 0)`，
一次普通 store，不是原子操作**。这才是"rwlock 更贵"的真正来源 ——
官方原话（§6）：*"reader-writer locks require more atomic memory operations than
simple spinlocks. **Unless the reader critical section is long, you are better off
just using spinlocks.**"*
</details>
</details>

**Q2.** RCU 相比 rwlock 有什么优势？

<details><summary>答案</summary>

RCU 读端：`rcu_read_lock()` 只禁抢占（无原子操作，零开销）。rwlock 读端：`read_lock()`
原子递增 reader count（有 cache line bouncing）。RCU 写端：复制 + 替换指针 + 等 grace period。
rwlock 写端：等所有读者退出。RCU 适合：读极多写极少。rwlock 适合：读写都有但读多。
RCU 缺点：写端延迟大（等 grace period）。

<details><summary>按 v6.6 修订/补充</summary>

**"RCU 读端无原子操作"这条在 v6.6 上需要加一句限定**。
`rcu_read_lock()` 在**非 PREEMPT_RCU** 配置下确实是纯 `preempt_disable()`（零原子操作）。
但开 `CONFIG_PREEMPT_RCU` 后它要操作 per-CPU 的 `rcu_read_lock_nesting` 计数
（`__rcu_read_lock()`），虽然仍是**本地 per-CPU 变量、不产生 cacheline 乒乓**，
但严格说不是"零操作"。准确的对比是：

| | 原子操作 | 触碰全局 cacheline | 阻塞写者 |
|---|---------|------------------|---------|
| RCU 读端 | 无（非 PREEMPT_RCU） | ❌ 不碰 | ❌ 不阻塞 |
| rwlock 读端 | **2 次原子 RMW**（加+减） | ✅ 碰 `cnts` | ✅ 阻塞 |
| spinlock | 1 次 CAS + 1 次普通 store | ✅ 碰 `val` | ✅ 阻塞 |

**"rwlock 写端等所有读者退出"这条要精确化**：v6.6 的写者等的是
`cnts == _QW_WAITING`，即"读者计数归零 **且** 没有别的写者持锁"。
而且因为写者握住 `wait_lock`，这期间**新读者进不来**，所以等待是有界的
（只等已在临界区内的读者）。

**RCU 的推荐是官方写进文档的**：`Documentation/locking/spinlocks.rst`：
*"NOTE! RCU is better for list traversal, but requires careful attention to
design detail."*
</details>
</details>

**Q3.** HFT 中如何选择读写锁 vs RCU vs 无锁？

<details><summary>答案</summary>

① 读极多写极少（路由表/配置）：RCU（内核）/ `std::shared_ptr<const T>`（用户态）。
② 读写都有但读多：`std::shared_mutex`（用户态）/ rwlock（内核）。
③ 热路径数据：无锁（SPSC 队列/per-thread 数据）。④ 配置变更：双缓冲（atomic swap pointer + 延迟释放旧版）。
HFT 原则：热路径零开销，冷路径可接受锁。

<details><summary>按 v6.6 修订/补充</summary>

**② 要加一个前置判据：读临界区的长度，而不只是读写比例。**

官方原话（§6）是"除非**读临界区很长**，否则不如用 spinlock"。落到选型上：

| 读临界区 | 读写比 | 内核推荐 | 理由 |
|---------|--------|---------|------|
| **短**（几条~几十条指令） | 任意 | **普通 `spinlock_t`** | rwlock 的原子 RMW 开销 > 并发收益 |
| **长** | 读 >> 写 | `rwlock_t` | 并发读者的并行收益盖过开销 |
| 任意 | 读 >>> 写，且是链表遍历 | **RCU** | 读端不碰全局 cacheline |
| **短** | 写者要求立即可见 | **seqlock** | 读者完全无锁，靠序号重试 |

**③ 的"无锁"在内核里要区分两层**：
真正无锁（`READ_ONCE`/`WRITE_ONCE` + 内存屏障，或 SPSC 环形缓冲）才是零同步开销；
`rwlock_t` / `spinlock_t` 都**不是**无锁（lock-free），它们是**阻塞**（blocking）同步。

**④ 的双缓冲在内核里对应 RCU 的 `rcu_assign_pointer()` + `call_rcu()` 延迟释放**，
不要自己用 `kfree` 直接释放旧版 —— 会有读者正在引用它。
</details>
</details>

**Q4.** `struct qrwlock` 的位布局是怎样的？`_QR_BIAS` 为什么是 512？

<details><summary>答案</summary>

```c
typedef struct qrwlock {
	union {
		atomic_t cnts;          /* 4 字节 */
		struct {
			u8 wlocked;         /* cnts 的最低字节 */
			u8 __lstate[3];
		};
	};
	arch_spinlock_t	wait_lock;      /* 4 字节，一把完整 qspinlock */
} arch_rwlock_t;      /* 共 8 字节 */
```

`cnts` 内部（32 位）：

| 位 | 宏 | 值 | 含义 |
|----|-----|-----|------|
| 0–7 | `_QW_LOCKED` | `0x0ff` | 写者持锁 |
| 8 | `_QW_WAITING` | `0x100` | 写者在等（公平性关键） |
| 9–31 | `_QR_BIAS` | `1U << 9` = **512** | 读者计数（每个读者 +512） |

`_QR_BIAS = 1 << _QR_SHIFT`，而 `_QR_SHIFT = 9` —— 因为低 9 位（0–8）
被写者的 `_QW_WMASK = 0x1ff` 占满了，读者计数只能从第 9 位开始。
所以 **bias 是 512 而不是 1**，纯粹是位域排布的结果。

最大并发读者 = `2^23 - 1` = **8,388,607**（计数位有 31-9+1 = 23 位）。

`wait_lock` 是一把**完整的 `arch_spinlock_t`**（即 qspinlock，4 字节）——
这就是 qrwlock 公平性的来源（§3）。

`wlocked` 取 `cnts` 的最低 1 字节，是为了让 `queued_write_unlock()`
退化成一次单字节 store：

```c
static inline void queued_write_unlock(struct qrwlock *lock)
{
	smp_store_release(&lock->wlocked, 0);
}
```

和 `struct qspinlock` 里 `locked` 独占一个字节是同一个套路（10.2）。
</details>

**Q5.** 读者快路径是"先加后查"，如果查到有写者会怎样？为什么要这样设计？

<details><summary>答案</summary>

```c
static inline void queued_read_lock(struct qrwlock *lock)
{
	int cnts;

	cnts = atomic_add_return_acquire(_QR_BIAS, &lock->cnts);   /* 先加 512 */
	if (likely(!(cnts & _QW_WMASK)))
		return;                                              /* 没写者 → 成功 */

	/* The slowpath will decrement the reader count, if necessary. */
	queued_read_lock_slowpath(lock);
}
```

慢路径里**还回去**：

```c
void __lockfunc queued_read_lock_slowpath(struct qrwlock *lock)
{
	if (unlikely(in_interrupt())) {
		atomic_cond_read_acquire(&lock->cnts, !(VAL & _QW_LOCKED));
		return;                                              /* ⚠️ 不回滚，直接自旋 */
	}
	atomic_sub(_QR_BIAS, &lock->cnts);                           /* 回滚 512 */
	arch_spin_lock(&lock->wait_lock);                            /* 排队 */
	atomic_add(_QR_BIAS, &lock->cnts);                           /* 重新加上 */
	atomic_cond_read_acquire(&lock->cnts, !(VAL & _QW_LOCKED));
	arch_spin_unlock(&lock->wait_lock);
}
```

**为什么不"先查后加"？** 因为先查后加会有竞态窗口：查完发现没写者，
准备加计数时写者插进来拿了锁 —— 读者就会在写者持锁期间进入临界区。
用 `atomic_add_return` 把"加"和"读回新值"合成**一条原子指令**，
新值里带着"此刻有没有写者"的信息，判据才有意义。

代价是失败时要一次额外的 `atomic_sub` 回滚。这是典型的
**"乐观执行 + 失败回滚"**（optimistic + rollback），
和 seqlock 的"乐观读 + 序号校验重试"是同一个思想。

**注意中断分支不回滚**：ISR 读者直接自旋在 `_QW_LOCKED` 上等写者走，
计数就留在那里了 —— 这是 §4 那个"插队"的实现细节。
</details>

**Q6.** 为什么中断上下文的读者必须"插队"？不插队会怎样？

<details><summary>答案</summary>

**不插队会死锁。** 推演：

```
task:  read_lock() ─► 快路径失败（有写者在等）
                   ─► 慢路径 ─► arch_spin_lock(&lock->wait_lock)  【握住队列】
                        │
                   本地中断打进来
                        │
ISR:   read_lock() ─► 快路径失败
                   ─► 慢路径 ─► arch_spin_lock(&lock->wait_lock)
                        │
                   ISR 自旋等 task 释放 wait_lock
                   task 永远要等 ISR 返回才能继续
                        ▼
                      死锁
```

所以源码给了特判（`queued_read_lock_slowpath()` 开头）：

```c
	if (unlikely(in_interrupt())) {
		/*
		 * Readers in interrupt context will get the lock immediately
		 * if the writer is just waiting (not holding the lock yet),
		 * so spin with ACQUIRE semantics until the lock is available
		 * without waiting in the queue.
		 */
		atomic_cond_read_acquire(&lock->cnts, !(VAL & _QW_LOCKED));
		return;
	}
```

**后果**：ISR 读者只看 `_QW_LOCKED`（写者是否**真的持锁**），
**不看 `_QW_WAITING`**（写者是否在等）。所以当写者还在排队、尚未持锁时，
ISR 读者**可以插进去**。

**工程含义**：如果 ISR 里会 `read_lock()`，严格的 FIFO 公平性就**不成立**，
写者仍可能被密集的中断读者延迟。源码选择的是
**"轻微不公平" > "必现死锁"**。
</details>

**Q7.** 官方为什么说"除非读临界区很长，否则不如用 spinlock"？这笔账怎么算？

<details><summary>答案</summary>

`Documentation/locking/spinlocks.rst` 原话（Lesson 2）：

> *NOTE! reader-writer locks require more atomic memory operations than
> simple spinlocks. **Unless the reader critical section is long, you
> are better off just using spinlocks.***

**账**：

| | 普通 `spinlock_t` | `rwlock_t`（读者） |
|---|---|---|
| 加锁 | 1× `cmpxchg`（无争用时） | 1× `atomic_add_return`（**每次都 RMW**） |
| **解锁** | **1× 普通 store**（`smp_store_release(&lock->locked, 0)`，单字节，**非原子**） | 1× `atomic_sub_return`（**RMW**） |
| 争用 | 每人自旋在**自己的** MCS node（本地 cacheline，O(N)） | 回滚 + 全挤在同一把 `wait_lock` 上 |

**最关键的差异是解锁**：普通 spinlock 的 `queued_spin_unlock()` 是
`smp_store_release(&lock->locked, 0)` —— 一次**普通单字节 store**，
不是原子操作，不产生 cacheline 独占。
而 rwlock 的 `queued_read_unlock()` 是 `atomic_sub_return_release()` ——
**原子 RMW，必须独占 cacheline**，N 个读者并发解锁就是 N 次 cacheline 乒乓。

**结论**：rwlock 的收益是"多个读者的临界区**并行执行**"，
成本是"每次加解锁多一次原子 RMW"。
只有当**临界区足够长**，长到并行执行省下的时间 > 额外原子开销，才划算。
**读写比例不是判据，临界区长度才是。**
</details>

**Q8.** 为什么不能把读锁"升级"成写锁？v6.6 上这条仍然成立吗？

<details><summary>答案</summary>

**成立，而且是结构性的，不是实现细节。**

官方文档原话（`Documentation/locking/spinlocks.rst`）：

> *Also, you cannot "upgrade" a read-lock to a write-lock, so if you at _any_
> time need to do any changes (even if you don't do it every time), you have
> to get the write-lock at the very beginning.*

**为什么结构性不可能**：

`queued_write_lock()` 的完成条件是 `cnts == 0`（快路径 CAS）
或慢路径里 `cnts == _QW_WAITING`（即**读者计数归零**）。
如果调用者自己还持着读锁，`cnts` 里至少有 `_QR_BIAS = 512`，
**永远不可能归零** —— 写者会一直等自己释放读锁，而自己在等写锁。**自死锁**。

```c
	/* 写者慢路径的等待条件 */
	do {
		cnts = atomic_cond_read_relaxed(&lock->cnts, VAL == _QW_WAITING);
	} while (!atomic_try_cmpxchg_acquire(&lock->cnts, &cnts, _QW_LOCKED));
```

`VAL == _QW_WAITING` 意味着"读者计数 == 0 且 WAITING 位亮着"。
自带读锁的升级者永远等不到这个条件。

**而且 qspinlock 也一样**：自旋锁家族设计上就没有"持有者可以重入"的概念
（per-CPU 的 4 个 MCS node 是给**不同嵌套层级**用的，不是给**同一层级重入**用的，见 10.2）。

**正确做法**：
① 如果只有部分路径要改 → **从一开始就拿写锁**（官方建议）；
② 或者先释放读锁、再拿写锁、然后**重新校验**条件（经典 double-check）；
③ 或者换 seqlock / RCU —— 它们的设计前提就是"读者可能被写者打断"。
</details>

**Q9.** RT 内核上 `read_lock()` 的行为和非 RT 有什么不同？

<details><summary>答案</summary>

| 配置 | `rwlock_t` 实际类型 | `read_lock()` 实现 | 可睡眠？ |
|------|-------------------|-------------------|---------|
| 非 RT | `{ arch_rwlock_t raw_lock; ... }` = `struct qrwlock`（8 字节） | `queued_read_lock()` 真自旋 | ❌ |
| **PREEMPT_RT** | `{ struct rwbase_rt rwbase; atomic_t readers; }` | `rt_read_lock()` | ✅ **可睡眠** |

```c
/* include/linux/rwbase_rt.h:11 */
struct rwbase_rt {
	atomic_t		readers;
	struct rt_mutex_base	rtmutex;
};

/* include/linux/rwlock_rt.h:35,82 */
static __always_inline void read_lock(rwlock_t *rwlock)  { rt_read_lock(rwlock); }
static __always_inline void write_lock(rwlock_t *rwlock) { rt_write_lock(rwlock); }
```

和 `spinlock_t` 的情况**完全同构**（10.2 §8）：RT 把整个"自旋"家族
（`spinlock_t` / `rwlock_t`）都换成了基于 `rt_mutex_base` 的可睡眠锁，
只留 `raw_spinlock_t` / `raw_` 族保持真自旋。

**记忆彩蛋**：`SPINLOCK_MAGIC = 0xdead4ead`，`RWLOCK_MAGIC = 0xdeaf1eed`
（都是 16 进制英文谐音），`CONFIG_DEBUG_SPINLOCK` 时会塞进结构里做校验。

**实践含义**：在 RT 内核上，rwlock 临界区里可以调 `copy_from_user()` /
`kmalloc(GFP_KERNEL)`；但**延迟敏感的路径一定要改用 raw 族**，
否则一次意外的睡眠就会把尾延迟打到毫秒级。
</details>

</details>

---
