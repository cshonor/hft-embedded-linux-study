## ⑤ 互斥体 · Mutexes

专为 **互斥** 设计的睡眠锁 — **新内核代码里「需要睡眠的互斥」首选**（优于用 semaphore 凑合）。

| 属性 | 说明 |
|------|------|
| 争用 | **睡眠** |
| 上下文 | **仅进程上下文** |
| 持有者 | **有明确 owner**（便于调试、优先级继承等） |

#### 四条严格规则（Love 强调）

| # | 规则 |
|---|------|
| 1 | **只有持有者可以 unlock** |
| 2 | **禁止递归加锁**（同任务二次 lock → 死锁） |
| 3 | **不可在中断/原子上下文使用** |
| 4 | **必须用正式 API 初始化**（勿手搓脏内存当 mutex） |

另：持锁期间 **可以调度/睡眠**（这是相对 spinlock 的意义），但勿在持锁时做无界长时间工作拖垮系统。

#### API 直觉

```c
struct mutex m;
mutex_init(&m);

mutex_lock(&m);
/* 临界区：可 sleep，但尽量短 */
mutex_unlock(&m);

/* 可中断等待 */
if (mutex_lock_interruptible(&m) == 0) {
    /* ... */
    mutex_unlock(&m);
}
```

| API | 用途 |
|-----|------|
| `mutex_lock` | 不可中断睡等 |
| `mutex_lock_interruptible` | 信号可打断 |
| `mutex_trylock` | 不睡 |
| `mutex_is_locked` | 查询（慎用于逻辑） |

#### 选型

| 场景 | 选 |
|------|-----|
| 短、不睡、可在中断 | **spinlock** |
| 长、可睡、仅进程上下文 | **mutex** |
| 计数资源 | **semaphore** |

**HFT：** 用户态 `std::mutex` / `pthread_mutex` 对应层；交易热路径用无锁/原子，**配置重载、会话管理** 用 mutex。内核驱动：`probe`/ioctl 慢路径用 mutex，硬中断里绝不用。

---

> **本篇分工**：上面速查表**原样保留**。本篇往下**不复述**"什么是互斥"，只做八件事，
> 全部用 v6.6 源码实证：
>
> ① 拆开 `struct mutex`（**32 字节**），讲清 `owner` 字段**借低 3 位**存状态的技巧
> （`MUTEX_FLAG_WAITERS` / `HANDOFF` / `PICKUP`）—— 书上完全没有；
> ② 讲全**三条**获取路径（书上只讲 fast/slow 两条，**漏了 midpath 乐观自旋**），
> 并指出 `struct optimistic_spin_queue osq` **只有 4 个字节** —— 因为 MCS 队列
> 其实在 per-CPU 上，不在 mutex 里；
> ③ ⭐ **HANDOFF / PICKUP 两标志**：v6.6 的 mutex 在队首等太久时会**要求解锁方直接
> 把锁交给自己**，不经过"释放→竞争"。这跟 10.4 §8 讲过的信号量"传令牌"是**同一个
> 设计模式**；顺带做版本断崖：**HANDOFF 出现在 v4.14**（v4.9 还没有）；
> ④ ⭐ **订正 Q2**：原答案说"mutex 无优先级继承"只对一半 —— 非 RT 上确实没有，
> 但 **`CONFIG_PREEMPT_RT` 下 `struct mutex` 就是 `rt_mutex_base`，优先级继承是有的**；
> ⑤ ⭐ **RT 上 mutex 反而变慢**：RT 的 `struct mutex` **没有 `osq` 字段**，
> 乐观自旋整条路径消失。这是 RT 换确定性所付的代价，HFT 选型时要算进去；
> ⑥ 解锁侧 `__mutex_unlock_slowpath()`：**先放锁再拿 wait_lock**，
> 并用 `wake_q` 批量唤醒（不是逐个 `wake_up_process`）；
> ⑦ **`ww_mutex`（wound/wait）**：一次锁多个 mutex 时防死锁的官方机制，书上没讲；
> ⑧ `mutex_trylock()` 的返回值警告（源码明写"与 `down_trylock()` 相反"）。
>
> 所有常量与代码均核对自缓存的 v6.6 源码，行号可查。

---

## 1. `struct mutex` 是 **32 字节**，但真正的队列不在里面

```c
/* include/linux/mutex.h —— v6.6（非 RT 分支） */
struct mutex {
	atomic_long_t		owner;
	raw_spinlock_t		wait_lock;
#ifdef CONFIG_MUTEX_SPIN_ON_OWNER
	struct optimistic_spin_queue osq; /* Spinner MCS lock */
#endif
	struct list_head	wait_list;
#ifdef CONFIG_DEBUG_MUTEXES
	void			*magic;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
};
```

x86-64 上逐字段算一遍（无 `CONFIG_DEBUG_MUTEXES` / `CONFIG_DEBUG_LOCK_ALLOC`）：

| 字段 | 大小 | 作用 |
|------|------|------|
| `atomic_long_t owner` | 8 B | 持有者 `task_struct *` **+ 低 3 位状态标志** |
| `raw_spinlock_t wait_lock` | 4 B | 保护 `wait_list`（又是 `raw_`，见 10.4 §1 的规律） |
| `struct optimistic_spin_queue osq` | **4 B** | 乐观自旋者的 MCS 队列**尾巴** |
| `struct list_head wait_list` | 16 B | 睡眠等待者队列 |
| **合计** | **32 B** | 与官方文档数字一致 |

### ⭐ `osq` 只有 4 个字节 —— 因为队列在 per-CPU 上

这是最容易误解的一点。`struct optimistic_spin_queue` 的定义短得意外：

```c
/* include/linux/osq_lock.h —— v6.6 */
struct optimistic_spin_node {
	struct optimistic_spin_node *next, *prev;
	int locked; /* 1 if lock acquired */
	int cpu; /* encoded CPU # + 1 value */
};

struct optimistic_spin_queue {
	/*
	 * Stores an encoded value of the CPU # of the tail node in the queue.
	 * If the queue is empty, then it's set to OSQ_UNLOCKED_VAL.
	 */
	atomic_t tail;          /* ← 就这一个字段！ */
};

#define OSQ_UNLOCKED_VAL (0)
```

`osq` 里**只有一个 `atomic_t tail`**（4 字节）—— 它存的是"队尾节点所在 CPU 的编号"。
真正的排队节点在 **per-CPU 变量**里：

```c
/* kernel/locking/osq_lock.c:14 */
static DEFINE_PER_CPU_SHARED_ALIGNED(struct optimistic_spin_node, osq_node);

/*
 * We use the value 0 to represent "no CPU", thus the encoded value
 * will be the CPU number incremented by 1.
 */
static inline int encode_cpu(int cpu_nr)
{
	return cpu_nr + 1;
}
```

`osq_lock()` 取节点的方式：

```c
bool osq_lock(struct optimistic_spin_queue *lock)
{
	struct optimistic_spin_node *node = this_cpu_ptr(&osq_node);   /* ← 自己的 per-CPU 节点 */
	...
	old = atomic_xchg(&lock->tail, curr);      /* 一次 xchg 入队：把自己挂到尾巴，换出旧尾巴 */
	if (old == OSQ_UNLOCKED_VAL)
		return true;                           /* 原来没人 → 直接拿到 */

	prev = decode_cpu(old);                    /* 旧尾巴 = 前驱 */
	node->prev = prev;
	smp_wmb();
	WRITE_ONCE(prev->next, node);              /* 前驱的 next 指向我 */
	...
}
```

三个值得记住的设计点：

| 点 | 说明 |
|----|------|
| **零分配** | 节点是 per-CPU 静态变量，**获取锁不需要任何内存分配** —— 这对"乐观自旋"这种高频快路径是必需的 |
| **`cpu + 1` 编码** | 和 10.2 讲过的 qspinlock `encode_tail()` 里 `(cpu + 1)` **完全同一个技巧**：用 0 表示"空"，否则 CPU 0 会和"队列为空"撞车 |
| **隐含"每 CPU 最多一个自旋者"** | 节点是 per-CPU 的，所以**同一个 CPU 上不可能同时有两个任务在 osq 队列里**。这天然成立 —— 因为乐观自旋期间是**关抢占**的 |

最后一条尤其关键。`mutex_optimistic_spin()` 的调用者 `__mutex_lock_common()` 在
进入前就 `preempt_disable()` 了，整个乐观自旋过程**不会被抢占**，所以本 CPU 上
不会有第二个任务来抢同一个 per-CPU 节点。这也是 `mutex_can_spin_on_owner()`
里那句注释的底气：

```c
	/*
	 * We already disabled preemption which is equal to the RCU read-side
	 * crital section in optimistic spinning code. Thus the task_strcut
	 * structure won't go away during the spinning period.
	 */
```

（顺带：这句注释里 `task_strcut` 是个拼写错误，v6.6 里就这么写着。）

### `CONFIG_MUTEX_SPIN_ON_OWNER` 用户选不了

翻 `kernel/Kconfig.locks`：

```
config MUTEX_SPIN_ON_OWNER
	def_bool y
	depends on SMP && ARCH_SUPPORTS_ATOMIC_RMW
```

`def_bool y` = **没有 `prompt`，用户不可选**。只要是 SMP 且架构支持原子 RMW，
它就**必然打开**。所以在任何正常的多核 x86/arm64 机器上：

> `struct mutex` 里**一定有** `osq`，乐观自旋**一定存在**。
> 唯一的例外是 `CONFIG_PREEMPT_RT`（见 §5，那里整个 `struct mutex` 都换掉了）。

---

## 2. `owner` 字段借低 3 位存状态

`struct mutex` 的 `owner` 字段存的是持有者的 `struct task_struct *`。
但内核还要在**同一个字里**塞三个状态位。做法是利用指针对齐：

```c
/* kernel/locking/mutex.c:59 —— v6.6 原文 */
/*
 * @owner: contains: 'struct task_struct *' to the current lock owner,
 * NULL means not owned. Since task_struct pointers are aligned at
 * at least L1_CACHE_BYTES, we have low bits to store extra state.
 *
 * Bit0 indicates a non-empty waiter list; unlock must issue a wakeup.
 * Bit1 indicates unlock needs to hand the lock to the top-waiter
 * Bit2 indicates handoff has been done and we're waiting for pickup.
 */
#define MUTEX_FLAG_WAITERS	0x01
#define MUTEX_FLAG_HANDOFF	0x02
#define MUTEX_FLAG_PICKUP	0x04

#define MUTEX_FLAGS		0x07
```

**原理**：`task_struct` 是按 `L1_CACHE_BYTES`（通常 64 或 128）对齐的 ——
内核的 `task_struct` 分配走 kmem cache，且结构体本身做了 cacheline 对齐优化。
既然对齐到 64 字节，**低 6 位恒为 0**，白放着也是浪费，于是拿低 3 位当标志位。

| 位 | 宏 | 含义 | 谁置位 | 谁消费 |
|----|----|----|-------|-------|
| bit0 | `MUTEX_FLAG_WAITERS` | `wait_list` 非空，解锁时必须唤醒 | `__mutex_add_waiter()` 发现自己是队首时 | `__mutex_unlock_slowpath()` |
| bit1 | `MUTEX_FLAG_HANDOFF` | 解锁方**必须**把锁直接交给队首 | 队首等待者（`__mutex_trylock_or_handoff(lock, first)`） | `__mutex_unlock_slowpath()` |
| bit2 | `MUTEX_FLAG_PICKUP` | 锁已经交给某人了，等他来取 | `__mutex_handoff()` | 队首醒来后 `__mutex_trylock()` |

配套的提取函数：

```c
static inline struct task_struct *__mutex_owner(struct mutex *lock)
{
	return (struct task_struct *)(atomic_long_read(&lock->owner) & ~MUTEX_FLAGS);
}

static inline unsigned long __owner_flags(unsigned long owner)
{
	return owner & MUTEX_FLAGS;
}
```

⚠️ **这个技巧的代价**：`mutex_is_locked()` 不能简单判 `owner != 0`，必须
`__mutex_owner(lock) != NULL`：

```c
bool mutex_is_locked(struct mutex *lock)
{
	return __mutex_owner(lock) != NULL;
}
```

因为"没人持有但 `WAITERS` 位置了"时，`owner` 值是 `0x1`，**不是 0**。
（这就是 `10.4` 里 semaphore 的那个"陷阱"在 mutex 上的对应版本：
`sem->count == 0` 不代表没人在等；`mutex->owner == 0` 才是真空闲。）

> 💡 **这个"借低位"的模式在内核里反复出现**，见到一次记住一次：
> `struct page` 的 `page_link` 低 2 位（scatterlist）、`qspinlock` 的位域切分、
> `rb_root_cached` 的 `rb_leftmost` 借 `rb_node.color`、以及这里的 `owner` 低 3 位。
> 共同前提都是**指针对齐保证低位恒零**。

---

## 3. 三条获取路径（书上讲的两条漏了中间那条）

`Documentation/locking/mutex-design.rst` 明确说是**三条**：

> When acquiring a mutex, there are three possible paths that can be
> taken, depending on the state of the lock:
>
> (i) **fastpath**: tries to atomically acquire the lock by cmpxchg()ing
>     the owner with the current task.
> (ii) **midpath**: aka **optimistic spinning**, tries to spin for acquisition
>      while the lock owner is running and there are no other tasks ready
>      to run that have higher priority (need_resched).
> (iii) **slowpath**: last resort, ... the task is added to the wait-queue
>      and sleeps until woken up by the unlock path.

对应到 v6.6 代码：

| 路径 | 函数 | 条件 | 量级 |
|------|------|------|------|
| **fastpath** | `__mutex_trylock_fast()` | 完全无争用（`owner` 整个字 == 0） | ~20 cycles |
| **midpath** | `mutex_optimistic_spin()` | 有争用但**持有者正在别的 CPU 上跑** | 几十 ~ 几百 cycles |
| **slowpath** | `__mutex_lock_common()` 后半段 | 上面两条都失败 | 数 µs（睡眠） |

### fastpath：一次 cmpxchg

```c
static __always_inline bool __mutex_trylock_fast(struct mutex *lock)
{
	unsigned long curr = (unsigned long)current;
	unsigned long zero = 0UL;

	if (atomic_long_try_cmpxchg_acquire(&lock->owner, &zero, curr))
		return true;

	return false;
}
```

注意它 CAS 的期望值是 `0UL` —— **整个字必须完全为 0**，
也就是"既没有持有者，三个标志位也都没置"。官方原文点明了这点：

> This only works in the uncontended case (cmpxchg() checks against 0UL,
> so all 3 state bits above have to be 0). If the lock is contended it
> goes to the next possible path.

**推论**：一旦某个 mutex 上有过等待者（置了 `WAITERS` 位），
后续的 fastpath **全部失效**，必须走 slowpath 把位清掉。
`__mutex_remove_waiter()` 里的这行就是干这个的：

```c
	list_del(&waiter->list);
	if (likely(list_empty(&lock->wait_list)))
		__mutex_clear_flag(lock, MUTEX_FLAGS);    /* 队列空了 → 清掉所有标志位 */
```

### midpath：乐观自旋（书上没有的那条）

```c
/*
 * Optimistic spinning.
 *
 * We try to spin for acquisition when we find that the lock owner
 * is currently running on a (different) CPU and while we don't
 * need to reschedule. The rationale is that if the lock owner is
 * running, it is likely to release the lock soon.
 *
 * The mutex spinners are queued up using MCS lock so that only one
 * spinner can compete for the mutex. However, if mutex spinning isn't
 * going to happen, there is no point in going through the lock/unlock
 * overhead.
 * ...
 */
static __always_inline bool
mutex_optimistic_spin(struct mutex *lock, struct ww_acquire_ctx *ww_ctx,
		      struct mutex_waiter *waiter)
{
	if (!waiter) {
		/*
		 * The purpose of the mutex_can_spin_on_owner() function is
		 * to eliminate the overhead of osq_lock() and osq_unlock()
		 * in case spinning isn't possible. As a waiter-spinner
		 * is not going to take OSQ lock anyway, there is no need
		 * to call mutex_can_spin_on_owner().
		 */
		if (!mutex_can_spin_on_owner(lock))
			goto fail;

		/*
		 * In order to avoid a stampede of mutex spinners trying to
		 * acquire the mutex all at once, the spinners need to take a
		 * MCS (queued) lock first before spinning on the owner field.
		 */
		if (!osq_lock(&lock->osq))
			goto fail;
	}

	for (;;) {
		struct task_struct *owner;

		/* Try to acquire the mutex... */
		owner = __mutex_trylock_or_owner(lock);
		if (!owner)
			break;                                    /* 拿到了 */

		/*
		 * There's an owner, wait for it to either
		 * release the lock or go to sleep.
		 */
		if (!mutex_spin_on_owner(lock, owner, ww_ctx, waiter))
			goto fail_unlock;

		/*
		 * The cpu_relax() call is a compiler barrier which forces
		 * everything in this loop to be re-loaded. We don't need
		 * memory barriers as we'll eventually observe the right
		 * values at the cost of a few extra spins.
		 */
		cpu_relax();
	}

	if (!waiter)
		osq_unlock(&lock->osq);

	return true;

fail_unlock:
	if (!waiter)
		osq_unlock(&lock->osq);

fail:
	/*
	 * If we fell out of the spin path because of need_resched(),
	 * reschedule now, before we try-lock the mutex. This avoids getting
	 * scheduled out right after we obtained the mutex.
	 */
	if (need_resched()) {
		/*
		 * We _should_ have TASK_RUNNING here, but just in case
		 * we do not, make it so, otherwise we might get stuck.
		 */
		__set_current_state(TASK_RUNNING);
		schedule_preempt_disabled();
	}

	return false;
}
```

三个决定成败的判断：

| 判断 | 代码 | 不成立时 |
|------|------|---------|
| **我该不该让位？** | `need_resched()` → 返回 0 | 有更高优先级任务要跑 → 不自旋 |
| **持有者在跑吗？** | `owner_on_cpu(owner)` | 持有者不在 CPU 上（睡了/被抢占）→ 不自旋 |
| **持有者还是他吗？** | `mutex_spin_on_owner()` 循环条件 `__mutex_owner(lock) == owner` | 换人了 → 回到循环顶部重试 trylock |

```c
static inline int mutex_can_spin_on_owner(struct mutex *lock)
{
	struct task_struct *owner;
	int retval = 1;

	lockdep_assert_preemption_disabled();

	if (need_resched())
		return 0;

	owner = __mutex_owner(lock);
	if (owner)
		retval = owner_on_cpu(owner);

	/*
	 * If lock->owner is not set, the mutex has been released. Return true
	 * such that we'll trylock in the spin path, which is a faster option
	 * than the blocking slow path.
	 */
	return retval;
}
```

⚠️ **特别注意最后那句注释**：如果 `owner` 为 NULL（锁已释放），
`mutex_can_spin_on_owner()` **返回 1（true）**。这不是 bug，是刻意的 ——
此时直接走 spin 路径去 trylock，比走阻塞慢路径更快。

### `fail` 分支那个反直觉的处理

```c
	if (need_resched()) {
		__set_current_state(TASK_RUNNING);
		schedule_preempt_disabled();
	}
```

自旋失败后，如果发现 `need_resched()`，**先主动调度一次，再去 trylock**。
注释解释了原因：

> reschedule now, before we try-lock the mutex. **This avoids getting
> scheduled out right after we obtained the mutex.**

这是一个很实在的经验之谈：如果你明知道马上要被抢占，却抢在那之前拿到了锁，
那么**你会在持锁状态下被切走** —— 这段时间内所有争用这个锁的任务都被你拖住。
不如先让出 CPU，等回来再干干净净地拿锁。

**这条对 HFT 特别重要**：持锁者被抢占是"锁持有时间"里最难预测的那一部分。
内核在这里主动规避它，用户态写锁时同样应该考虑（避免在临界区内做任何可能
触发调度的事）。

### slowpath 主循环（`__mutex_lock_common` 的骨架）

```c
	preempt_disable();
	mutex_acquire_nest(&lock->dep_map, subclass, 0, nest_lock, ip);

	trace_contention_begin(lock, LCB_F_MUTEX | LCB_F_SPIN);
	if (__mutex_trylock(lock) ||
	    mutex_optimistic_spin(lock, ww_ctx, NULL)) {
		/* got the lock, yay! */
		lock_acquired(&lock->dep_map, ip);
		...
		preempt_enable();
		return 0;
	}

	raw_spin_lock(&lock->wait_lock);
	/* After waiting to acquire the wait_lock, try again. */
	if (__mutex_trylock(lock))
		goto skip_wait;

	debug_mutex_lock_common(lock, &waiter);
	waiter.task = current;
	...
	/* add waiting tasks to the end of the waitqueue (FIFO): */
	__mutex_add_waiter(lock, &waiter, &lock->wait_list);

	set_current_state(state);
	trace_contention_begin(lock, LCB_F_MUTEX);
	for (;;) {
		bool first;

		/*
		 * Once we hold wait_lock, we're serialized against
		 * mutex_unlock() handing the lock off to us, do a trylock
		 * before testing the error conditions to make sure we pick up
		 * the handoff.
		 */
		if (__mutex_trylock(lock))
			goto acquired;

		if (signal_pending_state(state, current)) {
			ret = -EINTR;
			goto err;
		}

		raw_spin_unlock(&lock->wait_lock);
		schedule_preempt_disabled();

		first = __mutex_waiter_is_first(lock, &waiter);

		set_current_state(state);
		if (__mutex_trylock_or_handoff(lock, first))
			break;

		if (first) {
			trace_contention_begin(lock, LCB_F_MUTEX | LCB_F_SPIN);
			if (mutex_optimistic_spin(lock, ww_ctx, &waiter))
				break;
			trace_contention_begin(lock, LCB_F_MUTEX);
		}

		raw_spin_lock(&lock->wait_lock);
	}
	raw_spin_lock(&lock->wait_lock);
acquired:
	...
```

几个容易看漏的点：

**① `preempt_disable()` 包住了整条路径。** 所以 mutex 的临界区里
**抢占是被关掉的** —— 这解释了为什么乐观自旋期间可以安全使用 per-CPU 的
`osq_node`（§1 那条）。

**② 睡醒后不是直接睡回去，而是先试一次 `mutex_optimistic_spin()`。**
而且**只有队首（`first`）才有资格自旋**：

```c
		if (first) {
			if (mutex_optimistic_spin(lock, ww_ctx, &waiter))
				break;
		}
```

为什么只有队首？因为其他等待者即使自旋拿到了锁，也会破坏 FIFO 公平性。
**队首自旋 = 不会抢自己人的饭碗**。这是"乐观自旋"和"公平性"的折中点。

**③ 队首自旋时传的 `waiter` 非空**，于是在 `mutex_optimistic_spin()` 里
**跳过 `osq_lock()`**：
`if (!waiter) { ... osq_lock(&lock->osq) ... }`。
也就是说：**睡眠队列的队首直接在 `owner` 字段上自旋，不进 osq 队列**。
源码注释说明了原因：

> The waiter flag is set to true if the spinner is a waiter in the wait
> queue. The waiter-spinner will spin on the lock directly and concurrently
> with the spinner at the head of the OSQ, if present, until the owner is
> changed to itself.

（于是一个有意思的状态：可能同时有"osq 队首"和"wait_list 队首"两个自旋者
在竞争。它们之间靠 `__mutex_trylock()` 的原子性决出胜负。）

---

## 4. ⭐ HANDOFF / PICKUP：mutex 版的"直接传令牌"

这是本篇最该记住的机制。它和 10.4 §8 讲的信号量 `up()` 传令牌
**是同一个设计模式**，只是 mutex 这边复杂一些（要处理"等太久"的判断）。

### 为什么需要它

先想朴素做法有什么问题。如果解锁方只是"清掉 `owner`，然后唤醒队首"：

```
wait_list = [A]，A 在睡眠
持有者 unlock()：
  ① owner = 0（锁空出来了）
  ② wake_up(A)
  ③ unlock 返回

⚠️ 在 ② 和 ③ 之间、以及 A 真正被调度到之前，锁是"空闲"的
  → 新来的任务 B 走 fastpath 一次 cmpxchg 就抢走了
  → A 醒来发现锁没了，只好重新排到队尾
  → 如果一直有新任务来，A 可能永远拿不到 → 饥饿
```

这和 10.4 §8 分析信号量时**一模一样的问题**。mutex 的解法也是同一招
（**不做"归还+唤醒"两步，做"直接移交"一步**），但多了一层判断：
**只有当队首明确要求时才移交**，避免每次解锁都付出移交的代价。

### 三个步骤

**步骤 1：队首醒来后，如果拿不到锁，就"下订单"要求 handoff**

```c
		first = __mutex_waiter_is_first(lock, &waiter);

		set_current_state(state);
		if (__mutex_trylock_or_handoff(lock, first))
			break;
```

`__mutex_trylock_or_handoff(lock, first)` → `__mutex_trylock_common(lock, handoff=first)`：

```c
static inline struct task_struct *__mutex_trylock_common(struct mutex *lock, bool handoff)
{
	unsigned long owner, curr = (unsigned long)current;

	owner = atomic_long_read(&lock->owner);
	for (;;) { /* must loop, can race against a flag */
		unsigned long flags = __owner_flags(owner);
		unsigned long task = owner & ~MUTEX_FLAGS;

		if (task) {
			if (flags & MUTEX_FLAG_PICKUP) {
				if (task != curr)
					break;              /* 令牌是给别人的 */
				flags &= ~MUTEX_FLAG_PICKUP;   /* 令牌是给我的，取走 */
			} else if (handoff) {
				if (flags & MUTEX_FLAG_HANDOFF)
					break;              /* 订单已经下过了 */
				flags |= MUTEX_FLAG_HANDOFF;   /* 下单：下次解锁交给我 */
			} else {
				break;
			}
		} else {
			MUTEX_WARN_ON(flags & (MUTEX_FLAG_HANDOFF | MUTEX_FLAG_PICKUP));
			task = curr;                    /* 锁空闲 → 直接占 */
		}

		if (atomic_long_try_cmpxchg_acquire(&lock->owner, &owner, task | flags)) {
			if (task == curr)
				return NULL;            /* 成功 */
			break;
		}
	}

	return __owner_task(owner);
}
```

注意 `flags |= MUTEX_FLAG_HANDOFF` 这一步 —— **它并没有拿到锁**，
只是在 `owner` 字里"贴了张条子"，然后返回失败（返回的是持有者 task，非 NULL）。
持有者下次解锁时会看到这张条子。

**步骤 2：解锁方看到条子，直接把锁交给队首**

```c
static noinline void __sched __mutex_unlock_slowpath(struct mutex *lock, unsigned long ip)
{
	struct task_struct *next = NULL;
	DEFINE_WAKE_Q(wake_q);
	unsigned long owner;

	mutex_release(&lock->dep_map, ip);

	/*
	 * Release the lock before (potentially) taking the spinlock such that
	 * other contenders can get on with things ASAP.
	 *
	 * Except when HANDOFF, in that case we must not clear the owner field,
	 * but instead set it to the top waiter.
	 */
	owner = atomic_long_read(&lock->owner);
	for (;;) {
		MUTEX_WARN_ON(__owner_task(owner) != current);
		MUTEX_WARN_ON(owner & MUTEX_FLAG_PICKUP);

		if (owner & MUTEX_FLAG_HANDOFF)
			break;                              /* ← 有订单：不释放，直接走移交 */

		if (atomic_long_try_cmpxchg_release(&lock->owner, &owner, __owner_flags(owner))) {
			if (owner & MUTEX_FLAG_WAITERS)
				break;                          /* 有等待者 → 去唤醒 */

			return;                             /* 无等待者 → 完事 */
		}
	}

	raw_spin_lock(&lock->wait_lock);
	debug_mutex_unlock(lock);
	if (!list_empty(&lock->wait_list)) {
		/* get the first entry from the wait-list: */
		struct mutex_waiter *waiter =
			list_first_entry(&lock->wait_list,
					 struct mutex_waiter, list);

		next = waiter->task;

		debug_mutex_wake_waiter(lock, waiter);
		wake_q_add(&wake_q, next);
	}

	if (owner & MUTEX_FLAG_HANDOFF)
		__mutex_handoff(lock, next);

	raw_spin_unlock(&lock->wait_lock);

	wake_up_q(&wake_q);
}
```

**步骤 3：`__mutex_handoff()` 把 `owner` 直接设成队首，并置 PICKUP**

```c
/*
 * Give up ownership to a specific task, when @task = NULL, this is equivalent
 * to a regular unlock. Sets PICKUP on a handoff, clears HANDOFF, preserves
 * WAITERS. Provides RELEASE semantics like a regular unlock, the
 * __mutex_trylock() provides a matching ACQUIRE semantics for the handoff.
 */
static void __mutex_handoff(struct mutex *lock, struct task_struct *task)
{
	unsigned long owner = atomic_long_read(&lock->owner);

	for (;;) {
		unsigned long new;

		MUTEX_WARN_ON(__owner_task(owner) != current);
		MUTEX_WARN_ON(owner & MUTEX_FLAG_PICKUP);

		new = (owner & MUTEX_FLAG_WAITERS);     /* 保留 WAITERS 位 */
		new |= (unsigned long)task;
		if (task)
			new |= MUTEX_FLAG_PICKUP;

		if (atomic_long_try_cmpxchg_release(&lock->owner, &owner, new))
			break;
	}
}
```

移交后 `owner` = `A | PICKUP`。这时：

- 任何 B 走 fastpath：期望 `0UL`，实际是 `A|PICKUP` → **CAS 失败**；
- 任何 B 走 `__mutex_trylock()`：看到 `task != curr` 且 `PICKUP` 置位 → **`break`，返回失败**；
- 只有 A 走 `__mutex_trylock()`：`task == curr` → **清掉 PICKUP，拿走锁**。

**锁从始至终没有出现过"空闲"状态，插队窗口被完全关闭。**

### 对照 10.4 的信号量

| | semaphore（10.4 §8） | mutex（本篇 §4） |
|--|----------------------|-----------------|
| 令牌载体 | `waiter.up`（bool 标志） | `owner` 字段的 task 部分 + `PICKUP` 位 |
| 谁决定移交 | `up()` 看到 `wait_list` 非空就移交 | 解锁方看到 **`HANDOFF` 位**才移交 |
| 队首要做的事 | 无需（睡等即可） | 醒来发现自己还拿不到 → **主动置 `HANDOFF` 下单** |
| 保 FIFO | ✅ | ✅ |
| 额外代价 | 每次 `up()` 都移交 | **只在队首等太久时才移交**（平时正常释放） |

mutex 这边多一层判断的理由很实际：**handoff 是有代价的**
（解锁方必须拿 `wait_lock`、必须走 slowpath 的移交路径）。
如果每次解锁都移交，轻争用场景反而变慢。
所以 v6.6 的策略是"**平时正常释放，队首急了才移交**"。

### 版本断崖：HANDOFF 出现在 **v4.14**

抓多个版本的 `kernel/locking/mutex.c` 做统计：

| 版本 | 文件大小 | `MUTEX_FLAG_HANDOFF` 出现次数 | `MUTEX_FLAG_PICKUP` 出现次数 |
|------|---------|------------------------------|------------------------------|
| v4.9 | 25,834 B | **0** | **0** |
| **v4.14** | 31,691 B | **5** | **7** ← 断点 |
| v4.19 | 37,539 B | 5 | 7 |
| v5.0 | 37,529 B | 5 | 7 |
| v6.0 | 29,888 B | 6 | 7 |
| **v6.6** | 29,888 B | 6 | 7 |

**HANDOFF/PICKUP 机制是 v4.14 引入的**，v4.9 及以前完全没有。
（v4.14 也是 PREEMPT_RT 相关改动大量合入的时期，二者不是巧合 ——
handoff 解决的"持锁者被抢占导致的不确定性"正是 RT 关心的核心问题。）

---

## 5. ⭐ PREEMPT_RT 上的 mutex：**有**优先级继承，但**没有**乐观自旋

这一节要订正原自测题 Q2 的答案，并给出一个 HFT 选型时真正用得上的结论。

### 订正："mutex 无优先级继承"只对一半

`include/linux/mutex.h` 有两个分支：

```c
#ifndef CONFIG_PREEMPT_RT

/*
 * Simple, straightforward mutexes with strict semantics:
 * ...（9 条语义 + 5 项 debug 能力）
 */

struct mutex {
	atomic_long_t		owner;
	raw_spinlock_t		wait_lock;
#ifdef CONFIG_MUTEX_SPIN_ON_OWNER
	struct optimistic_spin_queue osq; /* Spinner MCS lock */
#endif
	struct list_head	wait_list;
	...
};

#else /* !CONFIG_PREEMPT_RT */
/*
 * Preempt-RT variant based on rtmutexes.
 */
#include <linux/rtmutex.h>

struct mutex {
	struct rt_mutex_base	rtmutex;        /* ← 整个换掉！ */
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
};
#endif /* CONFIG_PREEMPT_RT */
```

RT 上 `struct mutex` 就**只是 `rt_mutex_base` 的一层包装**。而 rtmutex 的
头一句话就写着：

```c
/* include/linux/rtmutex.h:3 */
/*
 * RT Mutexes: blocking mutual exclusion locks with PI support
 *                                            ^^^^^^^^^^^^^^^
 * started by Ingo Molnar and Thomas Gleixner:
 */
struct rt_mutex_base {
	raw_spinlock_t		wait_lock;
	struct rb_root_cached   waiters;        /* ← 按优先级排序的红黑树 */
	struct task_struct	*owner;
};
```

**所以：RT 内核上 `mutex_lock()` 是有优先级继承的**，等待者按优先级在
`rb_root_cached waiters` 里排队（不是 FIFO 链表）。

| | 非 RT | `CONFIG_PREEMPT_RT` |
|---|-------|---------------------|
| `struct mutex` 本体 | `owner` + `osq` + `wait_list` | `struct rt_mutex_base` |
| 等待队列 | `struct list_head wait_list`（**FIFO**） | `struct rb_root_cached waiters`（**按优先级**） |
| **优先级继承** | ❌ **没有** | ✅ **有** |
| **乐观自旋（osq）** | ✅ 有 | ❌ **没有** |
| 大小 | 32 B | **40 B**（`4 + 24 + 8` → 对齐 40） |

### ⭐ 关键推论：RT 上 mutex 争用**更慢**

上表最后两行合起来看，会得出一个反直觉但很重要的结论：

> **`CONFIG_PREEMPT_RT` 换来了优先级继承和确定性，代价是失去了乐观自旋。**

具体到代码：RT 的 `struct mutex` **没有 `osq` 字段**，
`mutex_optimistic_spin()` 在 RT 分支里根本不存在（它在 `#ifndef CONFIG_PREEMPT_RT`
块内，见 `kernel/locking/mutex.c` 的 `#endif /* !CONFIG_PREEMPT_RT */` 包围区）。
于是获取路径从三条退化为两条：

```
非 RT：fastpath（cmpxchg） → midpath（乐观自旋，几百 cycles） → slowpath（睡）
RT  ：fastpath（cmpxchg） →                                    slowpath（睡）
                            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ 这一档没了
```

**后果**：在 RT 内核上，一次 mutex 争用如果 fastpath 失败，
就直接是**调度器级延迟**（数 µs），而非 RT 上还有"自旋几百 cycle 赌一把"的机会。

再加上 rtmutex 的等待队列是**红黑树**（`rb_root_cached`），入队/出队是
`O(log n)` 的树操作 + 可能的优先级继承链调整，比非 RT 的 `list_add_tail` 更重。

**对 HFT 的意义**：

| 场景 | 建议 |
|------|------|
| 追求**最低延迟**（不怕尾延迟抖动） | **非 RT** 内核 + 绑核 + `sched_fifo`。乐观自旋能救回大量临界区很短的争用 |
| 追求**确定性**（怕的是不可预测的抖动） | **RT** 内核。接受 mutex 争用更贵，换来的是优先级反转可控、抢占延迟有界 |
| 两者都要 | **热路径别用 mutex**（用无锁），mutex 只留在控制面 —— 这样两边的差异对你就不重要了 |

第三条才是真正的工程答案：**无论 RT 与否，热路径都不该有 mutex**。
§5 这整节的差异分析，本质是"如果你非要在热路径上用睡眠锁，那么选内核时该怎么权衡"。

---

## 6. 解锁侧：先放锁再拿 `wait_lock`，用 `wake_q` 批量唤醒

`mutex_unlock()`：

```c
void __sched mutex_unlock(struct mutex *lock)
{
#ifndef CONFIG_DEBUG_LOCK_ALLOC
	if (__mutex_unlock_fast(lock))
		return;
#endif
	__mutex_unlock_slowpath(lock, _RET_IP_);
}
EXPORT_SYMBOL(mutex_unlock);

static __always_inline bool __mutex_unlock_fast(struct mutex *lock)
{
	unsigned long curr = (unsigned long)current;

	return atomic_long_try_cmpxchg_release(&lock->owner, &curr, 0UL);
}
```

快路径是"把 `owner` 从 `current` CAS 回 **0**" —— 期望值必须是完整的 `current`
（不带任何标志位）。**只要三个标志位里有任何一个置着，快路径就失败**，
必须走 slowpath。这解释了为什么"有等待者过一次"的 mutex 会"降级"一段时间
（要等队列清空后 `__mutex_clear_flag(lock, MUTEX_FLAGS)` 才恢复）。

### slowpath 的两个优化

**① 先放锁，再拿 `wait_lock`**

```c
	/*
	 * Release the lock before (potentially) taking the spinlock such that
	 * other contenders can get on with things ASAP.
	 *
	 * Except when HANDOFF, in that case we must not clear the owner field,
	 * but instead set it to the top waiter.
	 */
	owner = atomic_long_read(&lock->owner);
	for (;;) {
		...
		if (owner & MUTEX_FLAG_HANDOFF)
			break;                    /* HANDOFF 时不释放，交给后面的 handoff 处理 */

		if (atomic_long_try_cmpxchg_release(&lock->owner, &owner, __owner_flags(owner))) {
			if (owner & MUTEX_FLAG_WAITERS)
				break;                /* 释放成功但有等待者 → 继续去唤醒 */
			return;                       /* 释放成功且无等待者 → 直接返回 */
		}
	}

	raw_spin_lock(&lock->wait_lock);          /* ← 放锁之后才拿这把自旋锁 */
	...
```

**为什么先放锁**：`wait_lock` 是一把**自旋锁**。如果握着自己的 mutex 去抢
`wait_lock`，那么在抢 `wait_lock` 的这段时间内，mutex 还处于"被持有"状态，
其他争用者白白等着。先把 mutex 放掉，别人就能立刻竞争。

代价是"先放锁"会打开一个短暂的插队窗口（新来的任务可能抢在队首前面）——
但这是**刻意的取舍**：默认优先 throughput，只有队首明确下单（`HANDOFF`）时
才切换到"保公平"模式。

**② `wake_q` 批量唤醒**

```c
	DEFINE_WAKE_Q(wake_q);
	...
		wake_q_add(&wake_q, next);
	...
	raw_spin_unlock(&lock->wait_lock);

	wake_up_q(&wake_q);                       /* ← 放掉 wait_lock 之后才真正唤醒 */
```

不是逐个 `wake_up_process()`，而是先收集到一个 `wake_q` 队列里，
**等 `wait_lock` 释放后再统一唤醒**。

理由：`wake_up_process()` 内部会拿运行队列的锁（`rq->lock`）并可能触发
调度决策，这是一段不短的路径。**在持有 `wait_lock` 自旋锁的情况下做这件事，
会显著拉长 `wait_lock` 的持有时间**，进而拖慢所有争用这个 mutex 的人。

`wake_q` 是内核里的通用模式（waitqueue、futex、epoll 都在用）：
**收集 → 放锁 → 批量唤醒**。

---

## 7. `ww_mutex`（wound/wait）：一次锁多个 mutex 时防死锁

书上没讲，但工程上很有用。场景：驱动要同时锁住**多个**对象（比如 GPU 驱动
同时锁两个 buffer、文件系统 rename 同时锁两个 inode）。

朴素做法会 ABBA 死锁：

```
任务 1：lock(A) → lock(B)
任务 2：lock(B) → lock(A)
```

v6.6 的解法是 **wound/wait**（伤害/等待），`ww_mutex`：

```c
/* mutex.c 里的关键片段 */
	ww = container_of(lock, struct ww_mutex, base);
	if (ww_ctx) {
		if (unlikely(ww_ctx == READ_ONCE(ww->ctx)))
			return -EALREADY;
		...
	}
```

核心规则（对照 `__mutex_lock_common` 里的调用）：

| 机制 | 代码 | 含义 |
|------|------|------|
| **按 stamp 排序入队** | `__ww_mutex_add_waiter(&waiter, lock, ww_ctx)` | 不是 FIFO，而是按 `ww_ctx->stamp`（获取序列号）排序 |
| **Wound（伤害）** | `__ww_mutex_check_waiters(lock, ww_ctx)` | 遇到"比我年轻"的持锁者 → **要求他自杀**（返回 `-EDEADLK`） |
| **Wait（等待）** | 正常排队 | 遇到"比我年长"的持锁者 → 老实等 |
| **Kill 检查** | `__ww_mutex_check_kill(lock, &waiter, ww_ctx)` | 被 wound 的任务醒来后发现自己该退让 |

```c
		if (!use_ww_ctx) {
			/* add waiting tasks to the end of the waitqueue (FIFO): */
			__mutex_add_waiter(lock, &waiter, &lock->wait_list);
		} else {
			/*
			 * Add in stamp order, waking up waiters that must kill
			 * themselves.
			 */
			ret = __ww_mutex_add_waiter(&waiter, lock, ww_ctx);
			if (ret)
				goto err_early_kill;
		}
```

**判定逻辑一句话**：**老的赢**（先开始批量获取的那个）。年轻的任务遇到
年长的持锁者就等；年长的任务遇到年轻的持锁者就"伤害"他，让他回滚。

用法（GPU 驱动的经典套路）：

```c
struct ww_acquire_ctx ctx;
int ret;

ww_acquire_init(&ctx, &ww_class);

retry:
	ret = ww_mutex_lock(&obj1->lock, &ctx);
	if (ret)
		goto err;
	ret = ww_mutex_lock(&obj2->lock, &ctx);
	if (ret)
		goto err_obj1;

	/* ... 同时持有两个锁，干活 ... */

err_obj1:
	ww_mutex_unlock(&obj1->lock);
err:
	if (ret == -EDEADLK) {          /* 被"伤害"了：全部回滚，重新来过 */
		ww_mutex_unlock(&obj1->lock);
		ww_acquire_fini(&ctx);
		ww_acquire_init(&ctx, &ww_class);
		goto retry;
	}
	ww_acquire_fini(&ctx);
```

⚠️ 注意 `-EDEADLK` 的语义：它**不是**"死锁发生了"，而是
"**死锁被 w/w 机制检测并化解了，请你回滚重试**"。驱动必须处理它，
否则等于没启用这个机制。

---

## 8. API 全表与几个易忽略的变体

| API | 睡眠状态 | 返回值 | 说明 |
|-----|---------|--------|------|
| `mutex_lock()` | `TASK_UNINTERRUPTIBLE` | void | 不可中断 |
| `mutex_lock_interruptible()` | `TASK_INTERRUPTIBLE` | `0` / `-EINTR` | 任意信号可打断 |
| `mutex_lock_killable()` | `TASK_KILLABLE` | `0` / `-EINTR` | 只有致命信号可打断 |
| `mutex_lock_io()` | — | void | 见下 |
| `mutex_trylock()` | 不睡 | **`1`** / `0` | ⚠️ **遵循 `spin_trylock` 惯例** |
| `mutex_unlock()` | 不睡 | void | **不可在中断上下文** |
| `mutex_is_locked()` | 不睡 | bool | 查询用，别用于逻辑判断 |
| `mutex_destroy()` | — | void | 非 RT 上 debug 检查；RT 上是空函数 |
| `atomic_dec_and_mutex_lock()` | 同 `mutex_lock` | 1/0 | 见下 |

### `mutex_trylock()` 的返回值警告（源码明写）

```c
/*
 * NOTE: this function follows the spin_trylock() convention, so
 * it is negated from the down_trylock() return values! Be careful
 * about this when converting semaphore users to mutexes.
 *
 * This function must not be used in interrupt context. The
 * mutex must be released by the same task that acquired it.
 */
int __sched mutex_trylock(struct mutex *lock)
{
	bool locked;

	MUTEX_WARN_ON(lock->magic != lock);

	locked = __mutex_trylock(lock);
	if (locked)
		mutex_acquire(&lock->dep_map, 0, 1, _RET_IP_);

	return locked;
}
```

三家 trylock 的完整对照（**这是 10.4 §5 那张表的完整版**）：

| API | 成功 | 失败 | 惯例 |
|-----|------|------|------|
| `spin_trylock()` | 1 | 0 | C 真值 |
| `mutex_trylock()` | **1** | 0 | C 真值（**和 spin 一致**） |
| `down_trylock()` | **0** | **1** | errno 风格（**和上面两个相反**） |

**"converting semaphore users to mutexes"时最容易踩的坑**：

```c
/* semaphore 写法 */
if (down_trylock(&sem) == 0) { ... }      /* 0 = 成功 */

/* 机械翻译成 mutex，忘了取反 */
if (mutex_trylock(&m) == 0) { ... }       /* ❌ 完全反了！0 = 失败 */
if (mutex_trylock(&m)) { ... }            /* ✅ */
```

### `mutex_lock_io()`：告诉调度器"我在等 I/O"

```c
void __sched mutex_lock_io(struct mutex *lock)
{
	int token;

	token = io_schedule_prepare();
	mutex_lock(lock);
	io_schedule_finish(token);
}
EXPORT_SYMBOL_GPL(mutex_lock_io);
```

`io_schedule_prepare()` / `io_schedule_finish()` 这对函数的作用是
**把当前任务记入"本 CPU 的 I/O 等待计数"**（`rq->nr_iowait`）。
影响：

- 负载均衡器看到 `nr_iowait > 0` 时，认为这个 CPU 上有任务在等 I/O，
  CPU 是"空闲"的，会更积极地往这里迁移任务；
- 一些 CPU 频率调节器（如 `intel_pstate` 的某些模式）也会参考它。

**什么时候用**：当你明确知道这个 mutex 的持有者正在做 I/O
（比如持有者会在临界区里提交块设备请求）时，用 `mutex_lock_io()`
能让调度器做出更合理的决策。普通情况用 `mutex_lock()` 即可。

注意它是 `EXPORT_SYMBOL_GPL` —— **只有 GPL 模块能用**。

### `atomic_dec_and_mutex_lock()`：引用计数归零时顺手拿锁

```c
int atomic_dec_and_mutex_lock(atomic_t *cnt, struct mutex *lock)
{
	/* dec if we can't possibly hit 0 */
	if (atomic_add_unless(cnt, -1, 1))
		return 0;
	/* we might hit 0, so take the lock */
	mutex_lock(lock);
	if (!atomic_dec_and_test(cnt)) {
		/* when we actually did the dec, we didn't hit 0 */
		mutex_unlock(lock);
		return 0;
	}
	/* we hit 0, and we hold the lock */
	return 1;
}
```

两段式优化：**绝大多数情况下（计数 > 1）根本不碰 mutex**，
只在"可能归零"时才拿锁。这是"在锁外面做廉价检查"的经典范式。

典型用途：对象在最后一个引用释放时要做清理，清理过程需要睡眠（所以不能用 spinlock）：

```c
static void obj_put(struct obj *o)
{
	if (atomic_dec_and_mutex_lock(&o->refcnt, &o->lock)) {
		/* 计数归零，且我已经持有 o->lock —— 安全做清理 */
		... 释放资源，可以睡眠 ...
		mutex_unlock(&o->lock);
		kfree(o);
	}
}
```

⚠️ 注意 `kfree(o)` 的时机：必须在 `mutex_unlock()` **之后**，
因为 `struct mutex` 的 9 条语义里明确有
"**Memory areas where held locks reside must not be freed**"。

---

## 9. `CONFIG_DEBUG_MUTEXES`：那些凭空出现的检查从哪来

源码里到处可见的 `MUTEX_WARN_ON(...)` 和 `debug_mutex_*(...)`，
在不开 debug 时会被编译成空。开了之后它们撑起了 `mutex-design.rst`
里承诺的 5 项能力。`include/linux/mutex.h` 顶部的注释逐条列出：

```c
/*
 * These semantics are fully enforced when DEBUG_MUTEXES is
 * enabled. Furthermore, besides enforcing the above rules, the mutex
 * debugging code also implements a number of additional features
 * that make lock debugging easier and faster:
 *
 * - uses symbolic names of mutexes, whenever they are printed in debug output
 * - point-of-acquire tracking, symbolic lookup of function names
 * - list of all locks held in the system, printout of them
 * - owner tracking
 * - detects self-recursing locks and prints out all relevant info
 * - detects multi-task circular deadlocks and prints out all affected
 *   locks and tasks (and only those tasks)
 */
```

**`magic` 字段的用法**（在 debug 模式下才有）：

```c
#ifdef CONFIG_DEBUG_MUTEXES
	void			*magic;
#endif
```

`__mutex_init()` 里 `magic = lock`（**指向自己**），
然后 `mutex_lock()` 里第一行就检查：

```c
	MUTEX_WARN_ON(lock->magic != lock);
```

这是在抓"**未初始化 / 内存被踩 / 已被销毁**"的 mutex。
一个正常的 mutex 的 `magic` 必然等于它自己的地址；如果不等，
说明这个内存要么没初始化（`magic` 是 0 或垃圾），要么被别的东西覆盖了。

对照 10.2 里 spinlock 的 `SPINLOCK_MAGIC 0xdead4ead` 和 10.3 里 rwlock 的
`RWLOCK_MAGIC 0xdeaf1eed`：**mutex 用的是"自指指针"而不是魔数** ——
因为自指指针能顺带验证"这块内存的地址对不对"，比固定魔数更严格。

`kernel/locking/mutex-debug.c` 里还有一个 `CONFIG_DEBUG_MUTEXES`
专属的 `debug_mutex_lock_common()` / `debug_mutex_wake_waiter()` / 
`debug_mutex_add_waiter()` / `debug_mutex_remove_waiter()` 系列，
它们维护一个**全局的"所有已持有 mutex"链表**，用于死锁检测。

---

## HFT / 嵌入式关联

### 三档延迟，以及"持锁者被抢占"这个隐形杀手

把 §3 的三条路径换算成可感知的数字：

| 路径 | 条件 | 量级 | 备注 |
|------|------|------|------|
| fastpath | 完全无争用 | **~20 cycles**（一次 cmpxchg） | 走不到内核慢路径 |
| midpath | 争用 + 持有者在别的 CPU 上跑 | **几十 ~ 几百 cycles** | 靠 `owner_on_cpu()` 判断 |
| slowpath | 上面都失败 | **数 µs**（两次上下文切换） | 调度器级 |

**关键洞察在 midpath 的判据上**：

```c
	owner = __mutex_owner(lock);
	if (owner)
		retval = owner_on_cpu(owner);      /* ← 持锁者还在 CPU 上吗？ */
```

如果持锁者**不在 CPU 上**（被抢占了、或者自己在临界区里睡眠了），
乐观自旋**立即放弃**，直接去睡。这是对的 —— 自旋一个不在 CPU 上的持有者
纯属浪费，他不可能推进。

但反过来说明：**持锁者被抢占 = 争用者必定走 slowpath = 数 µs 延迟**。
所以优化 mutex 延迟的真正抓手不是"锁本身多快"，而是：

1. **缩短临界区**（降低被抢占的概率）；
2. **避免临界区内睡眠/做 I/O**；
3. **绑核 + `isolcpus` + `nohz_full`**，让持锁者不会被无关任务挤走；
4. 必要时在临界区前后 `preempt_disable()`（但要非常小心，见下）。

⚠️ 第 4 条要慎用：`mutex_lock()` 内部**已经** `preempt_disable()` 了，
但**临界区里抢占比默认状态是开的**（`mutex_lock` 成功返回前会 `preempt_enable()`）。
自己再加 `preempt_disable()` 会让"持锁者被抢占"消失，但也让
`mutex_can_spin_on_owner()` 里的 `need_resched()` 判断失效 ——
对 RT 是灾难。**非必要不要手动关抢占包住 mutex 临界区。**

### RT 与非 RT 的取舍（§5 的实操版）

| 需求 | 选 | 理由 |
|------|-----|------|
| P99 尾延迟优先，能接受偶发抖动 | **非 RT + 绑核** | 乐观自旋能救回大量"临界区很短"的争用 |
| 抖动上界优先（工业控制、机器人） | **RT** | 优先级继承 + 抢占延迟有界；接受 mutex 更贵 |
| 两者都要 | **热路径去掉 mutex** | 无锁环形队列 + 独占核，两边的差异对你不重要 |

实测建议（可用 `perf lock` 直接量化）：

```bash
# 记录锁事件（需要 CONFIG_LOCK_EVENTS 或 perf lock 支持）
perf lock record -- ./workload
perf lock report          # 看 acquired/contended/con-bounces

# 或者抓 tracepoint（10.4 §7 同款）
trace-cmd record -e lock:contention_begin -e lock:contention_end ./workload
```

看 `con-bounces`（争用反弹次数）：如果某个 mutex 的 `con-bounces` 高
但 `acquired` 也高，说明争用频繁但持有时间短 —— 这种正是"乐观自旋能救"的场景，
也是"该改成无锁"的场景。

### 用户态对照：`pthread_mutex` 的 adaptive 自旋

内核的 midpath（乐观自旋）在用户态有对应物：glibc 的
`PTHREAD_MUTEX_ADAPTIVE_NP`。它做的事几乎一样 ——
**先自旋一会儿，拿不到再进 futex 睡眠**。

| 内核 | 用户态 |
|------|--------|
| `mutex_optimistic_spin()` + `osq` | `PTHREAD_MUTEX_ADAPTIVE_NP` |
| `owner_on_cpu(owner)` 判断 | glibc 内部自旋计数上限 |
| 失败 → `schedule_preempt_disabled()` | 失败 → `futex(FUTEX_WAIT)` |

⚠️ 但有个重要区别：**用户态无法知道持锁者是否在 CPU 上**
（没有 `owner_on_cpu()` 的等价物，除非自己维护 owner 字段）。
所以 glibc 的自旋是"盲赌"固定次数，而内核是"看着 owner 的状态赌"。
这也是为什么**内核 mutex 的 midpath 比用户态 adaptive mutex 更有效率**。

用户态另外两个必须知道的选项：

| 属性 | 作用 | HFT 建议 |
|------|------|---------|
| `PTHREAD_PRIO_INHERIT` | 优先级继承（对应 RT 的 rtmutex） | RT 线程**必须开** |
| `PTHREAD_PRIO_PROTECT` | 优先级天花板 | 临界区短且已知上限时更高效 |

### 嵌入式：`-EDEADLK` 不是错误

`ww_mutex`（§7）返回 `-EDEADLK` 时，很多驱动新手会当成"死锁了"去报错。
实际上它是"**w/w 机制正常工作了，请你回滚重试**"。
嵌入式 GPU / DRM 驱动里这个路径很常见（多 buffer 竞争），
如果错误处理写成 `dev_err(...)` 然后放弃，设备会在偶发竞争时随机失败。

**正确做法永远是把 `-EDEADLK` 当"重试信号"处理**（见 §7 的 `retry:` 模板）。

---

## 实践模板

```c
#include <linux/mutex.h>
#include <linux/errno.h>

/* ---------- 模板一：最常用形态（interruptible + 检查返回值） ---------- */

struct my_dev {
	struct mutex lock;
	void __iomem *base;
};

static int my_dev_probe(struct platform_device *pdev)
{
	struct my_dev *dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	mutex_init(&dev->lock);          /* ✅ 必须用 API 初始化，不能 memset */
	platform_set_drvdata(pdev, dev);
	return 0;
}

static int my_dev_slow_path(struct my_dev *dev, void __user *arg)
{
	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;         /* ✅ 必须检查返回值 */

	/* ... 临界区：可以睡眠、可以做 I/O ... */

	mutex_unlock(&dev->lock);
	return 0;
}

static int my_dev_remove(struct platform_device *pdev)
{
	struct my_dev *dev = platform_get_drvdata(pdev);

	mutex_destroy(&dev->lock);       /* debug 模式下会检查"是否仍被持有" */
	return 0;
}


/* ---------- 模板二：明确知道持有者在做 I/O ---------- */

static void flush_and_lock(struct my_dev *dev)
{
	mutex_lock_io(&dev->lock);       /* 记入 rq->nr_iowait，帮调度器做决策 */
	/* ... 临界区里会提交块设备请求 ... */
	mutex_unlock(&dev->lock);
}


/* ---------- 模板三：引用计数归零时清理 ---------- */

static void obj_put(struct obj *o)
{
	if (atomic_dec_and_mutex_lock(&o->refcnt, &o->lock)) {
		/* 计数归零 + 已持有 o->lock */
		release_hw_resources(o);     /* 可以睡眠 */
		mutex_unlock(&o->lock);
		kfree(o);                    /* ⚠️ 必须在 unlock 之后 */
	}
}


/* ---------- 模板四：ww_mutex 多对象加锁（防 ABBA 死锁） ---------- */

static int lock_two(struct obj *a, struct obj *b, struct ww_acquire_ctx *ctx)
{
	int ret;

	ret = ww_mutex_lock(&a->lock, ctx);
	if (ret)
		return ret;

	ret = ww_mutex_lock(&b->lock, ctx);
	if (ret) {
		ww_mutex_unlock(&a->lock);
		return ret;                  /* 可能是 -EDEADLK，交给上层 retry */
	}
	return 0;
}

static int do_work(struct obj *a, struct obj *b)
{
	struct ww_acquire_ctx ctx;
	int ret;

	ww_acquire_init(&ctx, &ww_class);

retry:
	ret = lock_two(a, b, &ctx);
	if (ret == -EDEADLK) {           /* ✅ 不是"死锁了"，是"该回滚重试了" */
		ww_acquire_fini(&ctx);
		ww_acquire_init(&ctx, &ww_class);
		goto retry;
	}
	if (ret)
		goto out;

	/* ... 同时持有 a 和 b ... */

	ww_mutex_unlock(&b->lock);
	ww_mutex_unlock(&a->lock);
out:
	ww_acquire_fini(&ctx);
	return ret;
}
```

---

## 易错点核对表

| # | 易错点 | 正确做法 |
|---|--------|---------|
| 1 | `mutex_lock_interruptible()` 不检查返回值 | ❌ 被打断后仍会进临界区。必须 `if (...) return -ERESTARTSYS` |
| 2 | `memset(&m, 0, sizeof(m))` 当初始化 | ❌ 9 条语义里明令禁止。用 `mutex_init()` / `DEFINE_MUTEX()` |
| 3 | 把信号量代码机械翻译成 mutex | ❌ `mutex_trylock()` 返回 **1 = 成功**，和 `down_trylock()` **相反** |
| 4 | 中断上下文用 mutex | ❌ 9 条语义第 9 条明令禁止（含 tasklet / timer） |
| 5 | 递归加锁 | ❌ 必然自死锁；`DEBUG_MUTEXES` 会检测到 |
| 6 | 持锁期间 `kfree()` 含锁的内存 | ❌ 必须先 unlock 再 free |
| 7 | 用 `mutex_is_locked()` 做同步判断 | ❌ 查询和加锁之间有时间窗，不是原子操作 |
| 8 | 以为 mutex 永远有优先级继承 | ❌ 只有 `CONFIG_PREEMPT_RT` 下才有（非 RT 是纯 FIFO 链表） |
| 9 | 以为 RT 上 mutex 更快 | ❌ RT 上 **没有 `osq`**，乐观自旋整条路径消失，争用更贵 |
| 10 | 以为 `owner` 字段能直接当指针用 | ❌ 低 3 位是标志位，必须 `__mutex_owner()` 提取 |
| 11 | `ww_mutex` 返回 `-EDEADLK` 就报错放弃 | ❌ 那是"回滚重试"信号，必须 `goto retry` |
| 12 | 手动 `preempt_disable()` 包住 mutex 临界区 | ⚠️ 会让 `need_resched()` 判断失效，RT 上是灾难。非必要别做 |

---

## 常见陷阱

1. 在 mutex 和 semaphore 之间纠结——内核新代码始终优先 mutex
2. 以为 `mutex_lock()` 一定睡眠——无争用时直接原子获取（fast path），不进内核
3. 忽略优先级继承——mutex 默认不支持，需要 rt_mutex
   ⚠️ **（v6.6 修正）**：准确说法是——**非 RT 内核上 mutex 无 PI**；
   `CONFIG_PREEMPT_RT` 下 `struct mutex` 就是 `rt_mutex_base`，**PI 是有的**。
   需要 PI 时不必换用 `rt_mutex`（RT 内核上二者是同一个东西）；
   非 RT 上确实得显式用 `rt_mutex`。
4. **（v6.6 补充）** 把 `down_trylock()` 的 `== 0` 判据照搬到 `mutex_trylock()`
5. **（v6.6 补充）** 以为 `mutex_unlock()` 可以在中断里调 —— 明确禁止
6. **（v6.6 补充）** 以为"mutex 有等待者"和"mutex 被持有"是一回事 ——
   看 `owner` 的 task 部分和 `MUTEX_FLAG_WAITERS` 位是两件事
7. **（v6.6 补充）** 以为 `osq` 里存着队列 —— 它只有 4 字节的 `tail`，
   真正的节点在 per-CPU 上

---

## 自测题

<details>
<summary>自测题（点击展开）</summary>

**Q1.** mutex 的 fast path / slow path 是什么？

<details><summary>答案</summary>

Fast path（无争用）：`mutex_lock()` → 原子 CAS 将 owner 从 NULL 设为 current → 成功返回。开销 ~20ns。Slow path（有争用）：CAS 失败 → `__mutex_lock_slowpath()` → 加入等待队列 → `schedule()` 睡眠 → 被唤醒后重试 CAS。开销 ~1-5us。大部分场景 fast path 命中，mutex 性能接近 spinlock。

<details><summary>按 v6.6 修订/补充</summary>

**少了一条路径 —— 官方文档说的是三条，不是两条。**
`Documentation/locking/mutex-design.rst` 的原文：

> (i) **fastpath**: tries to atomically acquire the lock by cmpxchg()ing
>     the owner with the current task.
> (ii) **midpath**: aka **optimistic spinning**, tries to spin for acquisition
>      while the lock owner is running and there are no other tasks ready
>      to run that have higher priority (need_resched).
> (iii) **slowpath**: last resort, ... the task is added to the wait-queue
>      and sleeps until woken up by the unlock path.

漏掉的那条 **midpath（乐观自旋，§3）恰恰是 mutex 比 semaphore 快的原因**：

| 路径 | 条件 | 量级 | 对应函数 |
|------|------|------|---------|
| fastpath | `owner` 整个字 == 0 | ~20 cycles | `__mutex_trylock_fast()` |
| **midpath** | **持有者在别的 CPU 上跑** | **几十~几百 cycles** | `mutex_optimistic_spin()` |
| slowpath | 上面都失败 | 数 µs | `__mutex_lock_common()` 后半段 |

midpath 的两个准入条件（`mutex_can_spin_on_owner()`）：
① `!need_resched()`（没有更高优先级任务要跑）；
② `owner_on_cpu(owner)`（**持锁者确实还在 CPU 上**）。
第二条尤其重要 —— 如果持锁者被抢占了或在临界区里睡了，
**乐观自旋立即放弃**，直接去睡。所以"持锁者被抢占"是 mutex 延迟的隐形杀手。

另外：**10.4 §9 讲过，semaphore 没有 midpath** —— 它只有"拿到"和"睡"两档。
这就是为什么短临界区场景下 mutex 比 semaphore 快一个数量级。

还有一个细节值得更正：官方说 mutex 是 *"formally sleepable locks, it is
path (ii) that makes them more practically a **hybrid type**"* ——
**mutex 实际上是"自旋锁+睡眠锁"的混合体**，不只是睡眠锁。

</details>
</details>

**Q2.** mutex 和 rt_mutex 的区别？HFT 为什么要关心？

<details><summary>答案</summary>

mutex：无优先级继承。高优先级线程等低优先级线程持有的 mutex 时，低优先级线程不会被提升 → 优先级反转 → 高优先级线程延迟增大。rt_mutex：有优先级继承。高优先级等锁时，持有者的优先级被临时提升到等待者的级别。HFT 必须用 rt_mutex（或 `PTHREAD_PRIO_INHERIT`）防止优先级反转。

<details><summary>按 v6.6 修订/补充</summary>

**这条答案在 `CONFIG_PREEMPT_RT` 下不成立。** `include/linux/mutex.h` 有两个分支：

```c
#ifndef CONFIG_PREEMPT_RT
struct mutex {
	atomic_long_t		owner;
	raw_spinlock_t		wait_lock;
	struct optimistic_spin_queue osq;
	struct list_head	wait_list;      /* FIFO 链表 */
	...
};
#else /* !CONFIG_PREEMPT_RT */
/*
 * Preempt-RT variant based on rtmutexes.
 */
struct mutex {
	struct rt_mutex_base	rtmutex;        /* ← 整个换掉 */
	...
};
#endif
```

而 `struct rt_mutex_base`（`include/linux/rtmutex.h:17`）的等待队列是
**`struct rb_root_cached waiters`** —— 按优先级排序的红黑树，不是 FIFO 链表。
头文件第一句就写着 *"RT Mutexes: blocking mutual exclusion locks with **PI support**"*。

| | 非 RT | `CONFIG_PREEMPT_RT` |
|---|-------|---------------------|
| 等待队列 | `list_head wait_list`（**FIFO**） | `rb_root_cached waiters`（**按优先级**） |
| **优先级继承** | ❌ 没有 | ✅ **有** |
| **乐观自旋（osq）** | ✅ 有 | ❌ **没有** |
| 大小 | 32 B | 40 B |

**所以要区分两件事**：
- **"mutex vs rt_mutex"这个说法只在非 RT 上有意义。** RT 上它们是同一个东西，
  不需要"换用 rt_mutex" —— `mutex_lock()` 本身就有 PI。
- ⭐ **代价要说清楚**：RT 的 `struct mutex` **没有 `osq` 字段**，
  乐观自旋整条路径消失（获取路径从三条退化为两条）。
  所以 **RT 内核上 mutex 的争用反而更慢** —— fastpath 一失败就直接是调度器级延迟。

**HFT 该关心的结论**（§5 实操版）：
- 追**最低延迟** → 非 RT + 绑核，靠乐观自旋救回短临界区争用；
- 追**确定性上界** → RT，接受 mutex 更贵，换取优先级反转可控；
- **两个都要 → 热路径别用 mutex**（无锁环形队列 + 独占核），
  这样两边的差异对你就不重要了 —— 这才是真正的工程答案。

用户态对应：`PTHREAD_PRIO_INHERIT`（对应 rtmutex 的 PI）。
注意用户态**没有** `owner_on_cpu()` 的等价物，所以 glibc 的
`PTHREAD_MUTEX_ADAPTIVE_NP` 是"盲赌"固定次数，**不如内核的乐观自旋有效率**。

</details>
</details>

**Q3.** HFT 中 mutex 的使用最佳实践？

<details><summary>答案</summary>

① 热路径避免 mutex——用无锁设计。② 必须用 mutex 时：短临界区 + `try_lock` + 超时。③ `PTHREAD_PRIO_INHERIT` 属性防止优先级反转。④ `PTHREAD_MUTEX_ADAPTIVE_NP`：先 spin 再 sleep（glibc 扩展）。⑤ `perf lock` 分析持有时间。⑥ 避免 `std::mutex` 在 RT 线程中使用——用 `std::atomic` 或无锁队列。

<details><summary>按 v6.6 修订/补充</summary>

这六条都对，补三条**内核侧**的、以及一条修正。

**补充一：内核没有 `mutex_lock_timeout()`。**
第 ② 条说的"+"超时"在用户态是 `pthread_mutex_timedlock()`，
但**内核 v6.6 没有 mutex 超时 API**（对照 semaphore 有 `down_timeout()`）。
内核里要超时只能：
- 用 `mutex_lock_killable()`（至少能被 `SIGKILL` 兜底）；
- 或者用 `wait_event_timeout()` / `completion` + 自己实现；
- 或者在驱动里干脆改用 semaphore 的 `down_timeout()`。

**补充二：`perf lock` 该看什么。**
`con-bounces`（争用反弹次数）是关键指标：
- `con-bounces` 高 + 持有时间短 → 正是**乐观自旋能救**的场景，也是**该改成无锁**的场景；
- `con-bounces` 高 + 持有时间长 → 临界区设计有问题，要拆。

```bash
perf lock record -- ./workload
perf lock report          # 看 acquired / contended / con-bounces
```

**补充三：真正的优化抓手不是"锁多快"，是"别让持锁者被抢占"。**
因为 midpath 的准入条件之一是 `owner_on_cpu(owner)`
—— 持锁者一旦被抢占，所有争用者**必定**走 slowpath（数 µs）。
所以该做的是：① 缩短临界区；② 临界区内不睡眠不做 I/O；
③ `isolcpus` + `nohz_full` + 绑核；④ 非必要**不要**自己加
`preempt_disable()` 包住临界区（会让 `need_resched()` 判断失效，RT 上是灾难）。

**一条修正**：第 ⑥ 条说"RT 线程避免 `std::mutex`"是对的，但理由要说准 ——
`std::mutex` 在 glibc 里默认**既不开 PI 也不开 adaptive**，
所以在 RT 线程里用它既没有优先级继承、也没有自旋优化，两头不占。
正确做法是显式 `pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT)`。

</details>
</details>

**Q4.** `struct mutex` 的 `owner` 字段为什么能同时存指针和状态位？

<details><summary>答案</summary>

因为 `task_struct` 是按 **`L1_CACHE_BYTES`**（通常 64 或 128 字节）对齐的，
所以 `task_struct *` 的**低 6 位恒为 0** —— 内核把低 3 位借出来存状态。
`kernel/locking/mutex.c:59` 的原文：

```c
/*
 * @owner: contains: 'struct task_struct *' to the current lock owner,
 * NULL means not owned. Since task_struct pointers are aligned at
 * at least L1_CACHE_BYTES, we have low bits to store extra state.
 *
 * Bit0 indicates a non-empty waiter list; unlock must issue a wakeup.
 * Bit1 indicates unlock needs to hand the lock to the top-waiter
 * Bit2 indicates handoff has been done and we're waiting for pickup.
 */
#define MUTEX_FLAG_WAITERS	0x01
#define MUTEX_FLAG_HANDOFF	0x02
#define MUTEX_FLAG_PICKUP	0x04
#define MUTEX_FLAGS		0x07
```

| 位 | 宏 | 含义 | 谁置位 | 谁消费 |
|----|----|----|-------|-------|
| bit0 | `WAITERS` | `wait_list` 非空，解锁时必须唤醒 | `__mutex_add_waiter()`（发现自己是队首时） | `__mutex_unlock_slowpath()` |
| bit1 | `HANDOFF` | 解锁方必须把锁**直接交给队首** | 队首等待者 | `__mutex_unlock_slowpath()` |
| bit2 | `PICKUP` | 锁已交给某人，等他来取 | `__mutex_handoff()` | 队首醒来后 `__mutex_trylock()` |

**两个必须记住的推论**：

① **`mutex_is_locked()` 不能判 `owner != 0`**：
```c
bool mutex_is_locked(struct mutex *lock)
{
	return __mutex_owner(lock) != NULL;      /* 必须 & ~MUTEX_FLAGS */
}
```
因为"无人持有但 `WAITERS` 置位"时 `owner == 0x1`，**不是 0**。
（和 10.4 里 `sem->count == 0` 不代表没人在等，是同一类陷阱。）

② **fastpath 要求整个字为 0**：`__mutex_trylock_fast()` CAS 的期望值是
`0UL`，所以**只要三个标志位有任何一个置着，fastpath 就失效**。
这解释了为什么"有等待者过一次"的 mutex 会降级一段时间 ——
要等队列清空后 `__mutex_remove_waiter()` 里的
`__mutex_clear_flag(lock, MUTEX_FLAGS)` 才恢复。

💡 **这个"借低位"模式内核里反复出现**：`struct page` 的 `page_link` 低 2 位
（scatterlist）、qspinlock 的位域切分、`rb_root_cached` 借 `rb_node.color`、
以及这里的 `owner` 低 3 位。共同前提都是**指针对齐保证低位恒零**。

</details>

**Q5.** 什么是 HANDOFF / PICKUP？v6.6 为什么需要它？

<details><summary>答案</summary>

**解决的问题**：朴素的"解锁 → 唤醒队首"两步之间，锁会处于**空闲**状态，
新来的任务可以一次 cmpxchg 抢在等了很久的队首前面 → **队首饥饿**。

这与 10.4 §8 讲过的信号量 `up()` 传令牌是**同一个设计模式**，
区别是 mutex 多了一层判断：**只在队首明确要求时才移交**（避免每次解锁都付代价）。

**三步流程**：

**① 队首"下单"** —— 醒来发现自己还拿不到锁，就在 `owner` 里置 `HANDOFF`：
```c
	first = __mutex_waiter_is_first(lock, &waiter);
	set_current_state(state);
	if (__mutex_trylock_or_handoff(lock, first))   /* handoff = first */
		break;
```
在 `__mutex_trylock_common()` 里：
```c
	} else if (handoff) {
		if (flags & MUTEX_FLAG_HANDOFF)
			break;                    /* 订单已下过 */
		flags |= MUTEX_FLAG_HANDOFF;      /* 下单 */
	}
```
注意这一步**没拿到锁**，只是"贴了张条子"然后返回失败。

**② 解锁方看到条子，跳过"释放"这一步**：
```c
	owner = atomic_long_read(&lock->owner);
	for (;;) {
		if (owner & MUTEX_FLAG_HANDOFF)
			break;                    /* ← 不释放，走移交 */
		if (atomic_long_try_cmpxchg_release(&lock->owner, &owner, __owner_flags(owner))) {
			if (owner & MUTEX_FLAG_WAITERS)
				break;                /* 正常释放，但有等待者要去唤醒 */
			return;
		}
	}
	...
	if (owner & MUTEX_FLAG_HANDOFF)
		__mutex_handoff(lock, next);
```

**③ `__mutex_handoff()` 直接把 `owner` 设成队首 + 置 `PICKUP`**：
```c
	new = (owner & MUTEX_FLAG_WAITERS);   /* 保留 WAITERS */
	new |= (unsigned long)task;
	if (task)
		new |= MUTEX_FLAG_PICKUP;
```

移交后 `owner = A | PICKUP`，效果是：
- B 走 fastpath：期望 `0UL`，实际 `A|PICKUP` → **失败**；
- B 走 `__mutex_trylock()`：看到 `task != curr` 且 `PICKUP` 置位 → **`break`，失败**；
- A 走 `__mutex_trylock()`：`task == curr` → **清 PICKUP，拿走锁**。

**锁从始至终没有出现过空闲状态，插队窗口被完全关闭。**

**版本断崖（实测）**：抓多个版本的 `kernel/locking/mutex.c` 统计
`MUTEX_FLAG_HANDOFF` 出现次数：

| 版本 | `HANDOFF` 次数 | `PICKUP` 次数 |
|------|---------------|---------------|
| v4.9 | **0** | **0** |
| **v4.14** | **5** | **7** ← 引入 |
| v4.19 | 5 | 7 |
| v6.6 | 6 | 7 |

**HANDOFF 机制出现在 v4.14**，v4.9 及以前完全没有。

</details>

**Q6.** `struct optimistic_spin_queue osq` 里到底存了什么？为什么只有 4 字节？

<details><summary>答案</summary>

```c
/* include/linux/osq_lock.h */
struct optimistic_spin_queue {
	/*
	 * Stores an encoded value of the CPU # of the tail node in the queue.
	 * If the queue is empty, then it's set to OSQ_UNLOCKED_VAL.
	 */
	atomic_t tail;              /* ← 就这一个字段 */
};
#define OSQ_UNLOCKED_VAL (0)
```

**它只存"队尾节点所在 CPU 的编号"**，真正的排队节点在 **per-CPU 变量**里：

```c
/* kernel/locking/osq_lock.c:14 */
static DEFINE_PER_CPU_SHARED_ALIGNED(struct optimistic_spin_node, osq_node);

static inline int encode_cpu(int cpu_nr)
{
	return cpu_nr + 1;      /* 用 0 表示"空"，所以 +1 */
}
```

`osq_lock()` 的入队就是一次 `xchg`：
```c
	node = this_cpu_ptr(&osq_node);      /* 自己的 per-CPU 节点 */
	old = atomic_xchg(&lock->tail, curr);/* 挂到队尾，换出旧尾巴 */
	if (old == OSQ_UNLOCKED_VAL)
		return true;                     /* 原来没人 → 直接拿到 */
	prev = decode_cpu(old);
	node->prev = prev;
	smp_wmb();
	WRITE_ONCE(prev->next, node);
```

**三个设计点**：

① **零分配**：节点是 per-CPU 静态变量，获取锁不需要任何内存分配 ——
这对乐观自旋这种高频快路径是必需的。

② **`cpu + 1` 编码**：和 10.2 讲过的 qspinlock `encode_tail()` 里
`(cpu + 1)` **完全同一个技巧** —— 用 0 表示"队列空"，
否则 CPU 0 会和"空"撞车。

③ **隐含"每 CPU 最多一个自旋者"**：节点是 per-CPU 的，
所以同一 CPU 上不可能有两个任务同时在 osq 队列里。这天然成立 ——
因为**乐观自旋期间是关抢占的**（`__mutex_lock_common()` 进入前
就 `preempt_disable()` 了）。这也是 `mutex_can_spin_on_owner()` 里那句注释的底气：
> *"We already disabled preemption ... Thus the task_strcut structure
> won't go away during the spinning period."*
（原文里 `task_strcut` 是个拼写错误，v6.6 里就这么写着。）

**另外记住一个 Kconfig 事实**（`kernel/Kconfig.locks`）：
```
config MUTEX_SPIN_ON_OWNER
	def_bool y
	depends on SMP && ARCH_SUPPORTS_ATOMIC_RMW
```
`def_bool y` = **用户不可选**。任何 SMP + 支持原子 RMW 的机器上它必然打开，
所以 **`osq` 一定有、乐观自旋一定存在**。唯一例外是 `CONFIG_PREEMPT_RT`
（那里整个 `struct mutex` 被换成 `rt_mutex_base`，**没有 `osq` 字段**）。

</details>

**Q7.** 睡醒后的等待者，为什么只有队首才有资格继续自旋？

<details><summary>答案</summary>

看 `__mutex_lock_common()` 主循环的后半段：

```c
	for (;;) {
		bool first;

		if (__mutex_trylock(lock))
			goto acquired;

		if (signal_pending_state(state, current)) {
			ret = -EINTR;
			goto err;
		}

		raw_spin_unlock(&lock->wait_lock);
		schedule_preempt_disabled();

		first = __mutex_waiter_is_first(lock, &waiter);

		set_current_state(state);
		if (__mutex_trylock_or_handoff(lock, first))
			break;

		if (first) {                                  /* ← 只有队首 */
			trace_contention_begin(lock, LCB_F_MUTEX | LCB_F_SPIN);
			if (mutex_optimistic_spin(lock, ww_ctx, &waiter))
				break;
			trace_contention_begin(lock, LCB_F_MUTEX);
		}

		raw_spin_lock(&lock->wait_lock);
	}
```

**理由：保 FIFO 公平性。** `wait_list` 是 FIFO 链表，队首是等得最久的人。
如果他之外的等待者也去自旋抢锁，就可能在队首之前拿到锁 ——
等于让"后来但运气好"的任务插队，**破坏了 FIFO**。
队首自旋则没有这个问题：**他不抢自己人的饭碗**，他抢的只是当前持有者的。

**另一个细节**：队首自旋时传给 `mutex_optimistic_spin()` 的
`waiter` **非空**，于是会跳过 `osq_lock()`：

```c
	if (!waiter) {
		if (!mutex_can_spin_on_owner(lock))
			goto fail;
		if (!osq_lock(&lock->osq))
			goto fail;
	}
```

也就是说：**睡眠队列的队首直接在 `owner` 字段上自旋，不进 osq 队列**。
源码注释：
> *The waiter-spinner will spin on the lock directly and concurrently
> with the spinner at the head of the OSQ, if present, until the owner is
> changed to itself.*

于是一个有意思的并发状态：可能同时有 **osq 队首**和 **wait_list 队首**
两个自旋者在竞争，它们之间靠 `__mutex_trylock()` 的原子性决出胜负。

**顺带看一眼 `fail` 分支那个反直觉的处理**：
```c
	if (need_resched()) {
		__set_current_state(TASK_RUNNING);
		schedule_preempt_disabled();
	}
```
自旋失败后如果发现自己该让位了，**先主动调度一次再去 trylock**。
原因是注释里那句：*"This avoids getting scheduled out right after we
obtained the mutex."* —— 如果明知马上要被抢占却抢在那之前拿到锁，
就会**在持锁状态下被切走**，把所有争用者一起拖住。

</details>

**Q8.** `mutex_unlock()` 的 slowpath 做了哪两个优化？

<details><summary>答案</summary>

**优化一：先放锁，再拿 `wait_lock`。**

```c
	owner = atomic_long_read(&lock->owner);
	for (;;) {
		if (owner & MUTEX_FLAG_HANDOFF)
			break;                  /* HANDOFF 时不释放，交给 handoff */

		if (atomic_long_try_cmpxchg_release(&lock->owner, &owner, __owner_flags(owner))) {
			if (owner & MUTEX_FLAG_WAITERS)
				break;              /* 释放了，但有等待者要唤醒 */
			return;                     /* 释放了且无等待者 → 完事 */
		}
	}

	raw_spin_lock(&lock->wait_lock);        /* ← 放锁之后才拿 */
```

源码注释：
> *"Release the lock before (potentially) taking the spinlock such that
> other contenders can get on with things ASAP."*

**为什么**：`wait_lock` 是一把**自旋锁**。如果握着自己的 mutex 去抢它，
那么在抢的这段时间内 mutex 还处于"被持有"状态，其他争用者白白等着。

**代价**：先放锁会打开一个短暂的插队窗口（新来的可能抢在队首前面）。
这是**刻意的取舍** —— 默认优先 throughput，
只有队首明确下单 `HANDOFF` 时才切换到"保公平"模式（见 §4）。

---

**优化二：`wake_q` 批量唤醒，且放在释放 `wait_lock` 之后。**

```c
	DEFINE_WAKE_Q(wake_q);
	...
		wake_q_add(&wake_q, next);
	...
	raw_spin_unlock(&lock->wait_lock);

	wake_up_q(&wake_q);                     /* ← 放掉 wait_lock 之后才真唤醒 */
```

不是逐个 `wake_up_process()`，而是先收集到 `wake_q`，**等 `wait_lock` 释放后统一唤醒**。

**为什么**：`wake_up_process()` 内部要拿运行队列的锁（`rq->lock`）并可能触发
调度决策，路径不短。在持有 `wait_lock` 自旋锁时做这件事，会**显著拉长
`wait_lock` 的持有时间**，拖慢所有争用这个 mutex 的人。

`wake_q` 是内核通用模式（waitqueue / futex / epoll 都在用）：
**收集 → 放锁 → 批量唤醒**。

---

**顺带记住快路径的失效条件**：

```c
static __always_inline bool __mutex_unlock_fast(struct mutex *lock)
{
	unsigned long curr = (unsigned long)current;
	return atomic_long_try_cmpxchg_release(&lock->owner, &curr, 0UL);
}
```

期望值是完整的 `current`（**不带任何标志位**）。所以
**只要三个标志位有任何一个置着，快路径就失败**。
这也是"有等待者过一次"的 mutex 会降级一段时间的原因 ——
要等 `__mutex_remove_waiter()` 里的
`if (likely(list_empty(&lock->wait_list))) __mutex_clear_flag(lock, MUTEX_FLAGS);`
把标志位清掉才恢复。

</details>

**Q9.** `ww_mutex` 返回 `-EDEADLK` 是什么意思？该怎么处理？

<details><summary>答案</summary>

⚠️ **`-EDEADLK` 不是"死锁发生了"，而是"w/w 机制检测到了潜在死锁，
请你回滚重试"**。把它当成错误去报错放弃，是驱动里最常见的误用。

**背景**：一次要锁多个 mutex 时（GPU 驱动锁两个 buffer、文件系统 rename 锁
两个 inode），朴素的 `lock(A) → lock(B)` 会 ABBA 死锁。
v6.6 的解法是 **wound/wait（伤害/等待）**。

**判定规则一句话：老的赢。**（按 `ww_acquire_ctx` 的 `stamp` 序列号）

| 遇到 | 行为 | 代码 |
|------|------|------|
| 持锁者**比我年长** | **Wait** —— 老实等 | 正常排队 |
| 持锁者**比我年轻** | **Wound** —— 要求他自杀 | `__ww_mutex_check_waiters()` |
| 我被 wound 了 | 醒来后发现自己该退让 | `__ww_mutex_check_kill()` → `-EDEADLK` |

入队也和 FIFO 不同：
```c
	if (!use_ww_ctx) {
		/* add waiting tasks to the end of the waitqueue (FIFO): */
		__mutex_add_waiter(lock, &waiter, &lock->wait_list);
	} else {
		/*
		 * Add in stamp order, waking up waiters that must kill
		 * themselves.
		 */
		ret = __ww_mutex_add_waiter(&waiter, lock, ww_ctx);
		if (ret)
			goto err_early_kill;
	}
```

**正确处理模板**（见 §7 实践模板四）：

```c
	ww_acquire_init(&ctx, &ww_class);

retry:
	ret = lock_two(a, b, &ctx);
	if (ret == -EDEADLK) {          /* ✅ 回滚重试，不是报错 */
		ww_acquire_fini(&ctx);
		ww_acquire_init(&ctx, &ww_class);
		goto retry;
	}
	if (ret)
		goto out;
	...
```

**嵌入式上尤其要注意**：GPU / DRM 驱动里多 buffer 竞争很常见，
这个路径会在偶发竞争时触发。如果错误处理写成 `dev_err()` 然后放弃，
设备就会**随机失败**，而且极难复现（测试环境通常不并发，永远走不到）。

</details>

</details>

---

→ [10.2](./section-10.2-自旋锁.md) · [10.4](./section-10.4-信号量.md) · [10.11](./section-10.11-选型速查Ch-9--Ch-10.md)

> ↔ [ULK Ch5 §6 信号量与完成变量](../../../16-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-6-信号量与完成变量.md)
---
