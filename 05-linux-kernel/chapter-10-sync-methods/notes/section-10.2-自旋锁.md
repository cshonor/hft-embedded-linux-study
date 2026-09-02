## ② 自旋锁 · Spin Locks

内核 **最常见** 的锁：争用时 **忙等（自旋）** 直到锁可用 — **绝不睡眠**。

| 属性 | 说明 |
|------|------|
| 持有者 | **至多一个** 执行上下文 |
| 争用 | 不断重试获取（空转 CPU） |
| 上下文 | **进程 / 中断 / softirq** 都可能用（配合关中断/关 BH） |

> **本篇分工**：实体书把自旋锁当作「一个 `spinlock_t` + `spin_lock()`」来讲，实现细节停在
> ticket lock。本篇**不复述**适用/不适用、不可递归、与中断配合这些（下面保留速查表），只做四件事：
>
> ① 拆开 **v6.6 的三层类型**（`spinlock_t` → `raw_spinlock_t` → `arch_spinlock_t`），
> 讲清 **为什么要有 `raw_*` 这一层**；
> ② 逐字讲 **qspinlock 的 32 位字布局与慢路径状态机**（这是 v4.2 就取代 ticket lock 的东西，
> 书上是 v2.6 年代，完全没有）；
> ③ 订正三个凭记忆会写错的点：**qspinlock 并不严格 FIFO**、**UP 上 `spin_trylock()` 恒返回 1**、
> **开了 PARAVIRT 时慢路径默认会被劫持成 test-and-set**；
> ④ 给出 **PREEMPT_RT 下 `spin_lock()` 会睡眠**的实证 —— 书上那句"自旋锁绝不睡眠"在 RT 内核上是错的。
>
> 所有常量与代码均核对自缓存的 v6.6 源码，行号可查。

---

## 1. v6.6 的自旋锁是**三层类型**，不是一层

书的年代 `spinlock_t` 就是全部。现在它是个套娃：

```
spin_lock(&my_lock)
      │
      ▼
┌─────────────────────────────────────────────────────────┐
│ spinlock_t          include/linux/spinlock_types.h      │
│   ├─ union { raw_spinlock rlock; ...dep_map }           │  ← 非 RT
│   └─ struct rt_mutex_base lock;                         │  ← PREEMPT_RT
└─────────────────────────────────────────────────────────┘
      │
      ▼
┌─────────────────────────────────────────────────────────┐
│ raw_spinlock_t      include/linux/spinlock_types_raw.h  │
│   ├─ arch_spinlock_t raw_lock;                          │
│   ├─ [DEBUG_SPINLOCK] unsigned int magic, owner_cpu;    │
│   ├─ [DEBUG_SPINLOCK] void *owner;                      │
│   └─ [DEBUG_LOCK_ALLOC] struct lockdep_map dep_map;     │
└─────────────────────────────────────────────────────────┘
      │
      ▼
┌─────────────────────────────────────────────────────────┐
│ arch_spinlock_t  ==  struct qspinlock                   │
│   asm-generic/qspinlock_types.h                         │
│   一个 32 位字：locked(8) + pending(8) + tail(16)        │
└─────────────────────────────────────────────────────────┘
```

### 为什么需要 `raw_spinlock_t` 这一层

因为 **PREEMPT_RT 把 `spinlock_t` 变成了可睡眠锁**（见 §8）。但内核里确实存在
**连 RT 都不能睡眠**的地方 —— 调度器、时钟、中断底层、真正的硬件临界区。
这些地方必须用 `raw_spinlock_t`，它在 RT 上**仍然真自旋**。

```c
/* include/linux/spinlock_types.h —— 同一个 typedef，两个完全不同的实现 */

#ifndef CONFIG_PREEMPT_RT
/* Non PREEMPT_RT kernels map spinlock to raw_spinlock */
typedef struct spinlock {
	union {
		struct raw_spinlock rlock;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
#define LOCK_PADSIZE (offsetof(struct raw_spinlock, dep_map))
		struct {
			u8 __padding[LOCK_PADSIZE];
			struct lockdep_map dep_map;
		};
#endif
	};
} spinlock_t;
#else /* !CONFIG_PREEMPT_RT */
/* PREEMPT_RT kernels map spinlock to rt_mutex */
typedef struct spinlock {
	struct rt_mutex_base	lock;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
} spinlock_t;
#endif /* CONFIG_PREEMPT_RT */
```

> 注意非 RT 那个 `union` 里 `__padding[LOCK_PADSIZE]` 的写法：
> 它让 `spinlock_t` 和 `raw_spinlock_t` 的 `dep_map` 落在**同一偏移**，
> 这样 `spin_lock()` 只是一次 `&lock->rlock` 的取地址，零开销。

---

## 2. 一个自旋锁**只有 4 个字节**：qspinlock 的位布局

```c
/* include/asm-generic/qspinlock_types.h */
typedef struct qspinlock {
	union {
		atomic_t val;
#ifdef __LITTLE_ENDIAN
		struct {
			u8	locked;      /* bit  0- 7  锁是否被持有 */
			u8	pending;     /* bit  8-15  是否有人在"预备"抢锁 */
		};
		struct {
			u16	locked_pending;
			u16	tail;        /* bit 16-31  队列尾 */
		};
#else
		/* 大端机器反过来，此处略 */
#endif
	};
} arch_spinlock_t;
```

位域划分（源码注释逐字）：

| 位 | 字段 | 含义 |
|----|------|------|
| 0–7 | **locked byte** | 锁持有标志。`0` = 没人持锁 |
| 8 | **pending** | 有 CPU 正在"排队前的最后一次乐观自旋" |
| 9–15 | *(未用)* | 当 `CONFIG_NR_CPUS < 16K` 时整字节给 pending，浪费 7 位换字节写性能 |
| 16–17 | **tail index** | 嵌套层级：0=task, 1=softirq, 2=hardirq, 3=NMI |
| 18–31 | **tail cpu (+1)** | 队尾 CPU 号 |

`NR_CPUS >= 16K` 时布局会变（pending 只占 1 位，tail idx 挪到 9–10，tail cpu 占 11–31），
所以**别硬编码位偏移**，一律用 `_Q_LOCKED_MASK` / `_Q_PENDING_VAL` / `_Q_TAIL_MASK` 这套宏。

### 为什么 `pending` 要给**整整一个字节**

源码注释说得很直白：

> *By using the whole 2nd least significant byte for the pending bit, we can allow
> better optimization of the lock acquisition for the pending bit holder.*

以及：

> *Even though we only need 1 bit for the lock, we extend it to a full byte
> to achieve better performance for architectures that support atomic byte write.*

解锁时只写 `lock->locked` 一个字节（`smp_store_release(&lock->locked, 0)`），
不需要动 `pending` 和 `tail` —— 这是一次 **单字节 store**，比读-改-写整个 32 位字便宜得多。

### 为什么 tail 里的 CPU 号要 `+1`

```c
/* kernel/locking/qspinlock.c:107 */
/*
 * We must be able to distinguish between no-tail and the tail at 0:0,
 * therefore increment the cpu number by one.
 */
static inline __pure u32 encode_tail(int cpu, int idx)
{
	u32 tail;
	tail  = (cpu + 1) << _Q_TAIL_CPU_OFFSET;
	tail |= idx << _Q_TAIL_IDX_OFFSET; /* assume < 4 */
	return tail;
}
```

CPU 0 的 task 层（idx=0）编码出来是 `0`，会和「没有队尾」撞车。所以 `+1` 把 0 让出来当哨兵。
`decode_tail()` 再 `-1` 还原。

---

## 3. 快路径：就一条 CAS

```c
/* include/asm-generic/qspinlock.h */
static __always_inline void queued_spin_lock(struct qspinlock *lock)
{
	int val = 0;
	if (likely(atomic_try_cmpxchg_acquire(&lock->val, &val, _Q_LOCKED_VAL)))
		return;

	queued_spin_lock_slowpath(lock, val);
}

static __always_inline void queued_spin_unlock(struct qspinlock *lock)
{
	/* unlock() needs release semantics: */
	smp_store_release(&lock->locked, 0);   /* 只写 1 个字节 */
}

static __always_inline int queued_spin_trylock(struct qspinlock *lock)
{
	int val = atomic_read(&lock->val);
	if (unlikely(val))
		return 0;                            /* ⚠️ 有任何一位非 0 就直接放弃 */
	return likely(atomic_try_cmpxchg_acquire(&lock->val, &val, _Q_LOCKED_VAL));
}
```

三件事值得注意：

1. **无争用时加锁 = 一条带 LOCK 前缀的 `cmpxchg`**，解锁 = 一条**普通 store**（x86 TSO 下 release 免费）。
2. `queued_spin_trylock()` 判的是**整个 32 位字** `val != 0`，不是只看 locked byte。
   所以队列里只要还有人等着（tail != 0），`trylock` 就**一定失败** —— 这是 qspinlock
   比朴素 TAS 公平得多的一面。
3. `queued_spin_unlock()` 写的是 `&lock->locked`（union 的 u8 成员），**只碰第一个字节**。

---

## 4. 慢路径：先"乐观自旋"一次，失败才排 MCS 队列

qspinlock 的精髓是：**真正的 MCS 队列是最后手段**。中间插了一层 `pending` 位，
让「第二个到达者」不必入队，只需盯着 locked byte 自旋。

### 状态机（源码注释原图，逐字）

```
 *              fast     :    slow                                  :    unlock
 *                       :                                          :
 * uncontended  (0,0,0) -:--> (0,0,1) ------------------------------:--> (*,*,0)
 *                       :       | ^--------.------.             /  :
 *                       :       v           \      \            |  :
 * pending               :    (0,1,1) +--> (0,1,0)   \           |  :
 *                       :       | ^--'              |           |  :
 *                       :       v                   |           |  :
 * uncontended           :    (n,x,y) +--> (n,0,0) --'           |  :
 *   queue               :       | ^--'                          |  :
 *                       :       v                               |  :
 * contended             :    (*,x,y) +--> (*,0,0) ---> (*,0,1) -'  :
 *   queue               :         ^--'                             :
```

三元组是 `(tail, pending, locked)`：

| 状态 | 含义 |
|------|------|
| `(0,0,0)` | 空闲。快路径 CAS 直接拿走 |
| `(0,1,1)` | 有人持锁，第二个到达者已占住 pending 位，正在自旋 |
| `(0,1,0)` | 持锁者释放，pending 者即将接手 |
| `(n,x,y)` | 队列里有人（tail = n），新来者一律入队 |
| `(*,0,1)` | 队列头拿到锁，但队尾还有人 —— 持锁者还得把锁"传"给下一个 |

### 慢路径代码（v6.6 实测，节选关键分支）

```c
/* kernel/locking/qspinlock.c:316 */
void __lockfunc queued_spin_lock_slowpath(struct qspinlock *lock, u32 val)
{
	struct mcs_spinlock *prev, *next, *node;
	u32 old, tail;
	int idx;

	BUILD_BUG_ON(CONFIG_NR_CPUS >= (1U << _Q_TAIL_CPU_BITS));

	if (pv_enabled())
		goto pv_queue;
	if (virt_spin_lock(lock))       /* ⚠️ 见 §10：虚拟机上可能在这里直接被劫持走 */
		return;

	/* ① 有人正在做 pending->locked 交接，有限次等它交接完（保证前进） */
	if (val == _Q_PENDING_VAL) {
		int cnt = _Q_PENDING_LOOPS;
		val = atomic_cond_read_relaxed(&lock->val,
					       (VAL != _Q_PENDING_VAL) || !cnt--);
	}

	/* ② 只要看到 tail 或 pending 有值 —— 已经有队了，别抢，去排队 */
	if (val & ~_Q_LOCKED_MASK)
		goto queue;

	/* ③ 否则去占 pending 位（0,0,* -> 0,1,*），这是"最后一次乐观自旋" */
	val = queued_fetch_set_pending_acquire(lock);

	/* ④ 占 pending 的过程中若发现有别人也来了，撤销 pending，老实排队 */
	if (unlikely(val & ~_Q_LOCKED_MASK)) {
		if (!(val & _Q_PENDING_MASK))
			clear_pending(lock);
		goto queue;
	}

	/* ⑤ 我是 pending 者，等持锁者走开；必须 load-acquire 配 store-release */
	if (val & _Q_LOCKED_MASK)
		smp_cond_load_acquire(&lock->locked, !VAL);

	/* ⑥ 接手：清 pending，置 locked（0,1,0 -> 0,0,1） */
	clear_pending_set_locked(lock);
	lockevent_inc(lock_pending);
	return;

queue:
	lockevent_inc(lock_slowpath);
pv_queue:
	/* ⑦ 真排队：取本 CPU 的 MCS node */
	node = this_cpu_ptr(&qnodes[0].mcs);
	idx  = node->count++;
	tail = encode_tail(smp_processor_id(), idx);
	...
	/* ⑧ 超过 4 层嵌套（极端情况）就退化成裸自旋，不用 node */
	if (unlikely(idx >= MAX_NODES)) {
		lockevent_inc(lock_no_node);
		while (!queued_spin_trylock(lock))
			cpu_relax();
		goto release;
	}
	node = grab_mcs_node(node, idx);

	/* ⑨ 初始化 node，然后再 trylock 一次（碰碰运气） */
	barrier();
	node->locked = 0;
	node->next   = NULL;
	if (queued_spin_trylock(lock))
		goto release;

	/* ⑩ 发布 tail，把自己挂进队列 */
	smp_wmb();
	old = xchg_tail(lock, tail);
	next = NULL;
	if (old & _Q_TAIL_MASK) {
		prev = decode_tail(old);
		WRITE_ONCE(prev->next, node);            /* 挂到前驱后面 */
		pv_wait_node(node, prev);
		arch_mcs_spin_lock_contended(&node->locked);  /* 自旋在【自己的 node】上 */
		next = READ_ONCE(node->next);
		if (next)
			prefetchw(next);                 /* 提前把后继的 cacheline 拉来写 */
	}
	...
locked:
	/* ⑪ 轮到我了：如果我是唯一的等待者（val 的 tail == 我的 tail），
	       一次 CAS 把整个字换成 (0,0,1)，直接走人 */
	if ((val & _Q_TAIL_MASK) == tail) {
		if (atomic_try_cmpxchg_relaxed(&lock->val, &val, _Q_LOCKED_VAL))
			goto release; /* No contention */
	}
	/* ⑫ 否则只置 locked 位，然后把锁传给 next */
	set_locked(lock);
	if (!next)
		next = smp_cond_load_relaxed(&node->next, (VAL));
	arch_mcs_spin_unlock_contended(&next->locked);
	pv_kick_node(lock, next);
release:
	__this_cpu_dec(qnodes[0].mcs.count);
}
```

### 关键设计点：每个等待者自旋在**自己 CPU 的本地变量**上

这是 MCS 锁相对 test-and-set 的核心优势。经典 MCS 锁每人一个 node，
自旋在 `node->locked`：

```c
/* kernel/locking/mcs_spinlock.h */
struct mcs_spinlock {
	struct mcs_spinlock *next;
	int locked; /* 1 if lock acquired */
	int count;  /* nesting count, see qspinlock.c */
};

#define arch_mcs_spin_lock_contended(l)		\
do {						\
	smp_cond_load_acquire(l, VAL);		\
} while (0)

#define arch_mcs_spin_unlock_contended(l)	\
	smp_store_release((l), 1)
```

对比 TAS：`N` 个 CPU 同时对一个全局 cacheline 做 `cmpxchg`，每次成功写都会让其余 `N-1` 个
CPU 的 cacheline 副本失效 —— **O(N²) 的 cacheline 乒乓**。MCS 下每人盯自己的私有 cacheline，
前驱释放时只失效后驱**一条** line —— **O(N)**。

> ⚠️ 一个**反直觉但重要**的例外：`node` 是 `DEFINE_PER_CPU_ALIGNED` 的，
> 但 **`node->next` 是被别人写的**。所以 §4 第 ⑩ 步里那个 `prefetchw(next)` 是在
> "预取后继 node 的 cacheline 到本 CPU 并置为可写态"，省掉后面传递锁时的一次 cache miss。

---

## 5. 为什么 per-CPU 只有 **4 个** node

```c
/* kernel/locking/qspinlock.c:70 */
static DEFINE_PER_CPU_ALIGNED(struct qnode, qnodes[MAX_NODES]);
```
`#define MAX_NODES 4`

源码注释逐字：

> *Since a spinlock disables recursion of its own context and there is a limit
> to the contexts that can nest; namely: **task, softirq, hardirq, nmi**.
> As there are at most 4 nesting levels, it can be encoded by a 2-bit number.*

于是 tail 里的 **2 位 tail index** 就是这么来的：

| idx | 上下文 |
|-----|--------|
| 0 | 进程上下文（task） |
| 1 | softirq 打断了 task |
| 2 | hardirq 打断了 softirq |
| 3 | NMI 打断了 hardirq |

每个 CPU 最多同时"等在 4 把不同的锁上"（每层一把），所以 4 个 node 够用。

**超过 4 层怎么办？** 源码明确说这是个「不优雅但够简单」的兜底：

```c
	/*
	 * 4 nodes are allocated based on the assumption that there will
	 * not be nested NMIs taking spinlocks. That may not be true in
	 * some architectures even though the chance of needing more than
	 * 4 nodes will still be extremely unlikely. When that happens,
	 * we fall back to spinning on the lock directly without using
	 * any MCS node. This is not the most elegant solution, but is
	 * simple enough.
	 */
	if (unlikely(idx >= MAX_NODES)) {
		lockevent_inc(lock_no_node);
		while (!queued_spin_trylock(lock))
			cpu_relax();
		goto release;
	}
```

**大小刚好一条 cacheline**：64 位下 `struct mcs_spinlock` 是 16 字节（指针 8 + int 4 + int 4），
4 个 = 64 字节 = 一条 cacheline。开 `CONFIG_PARAVIRT_SPINLOCKS` 时 node 膨胀到 32 字节，
4 个 = 128 字节（两条 line）—— 源码承认这是为 PV 付的代价：

> *We don't want to penalize pvqspinlocks to optimize for a rare case in
> native qspinlocks.*

---

## 6. 版本断崖：**qspinlock 取代 ticket lock 在 v4.2**

这是本篇最值钱的一条 —— 书上（LKD3rd，v2.6 年代）和大量中文资料讲的都是 **ticket lock**，
而主线上它早就没了。

用「抓多版本同名文件比大小」的手法实测：

| 版本 | `include/asm-generic/qspinlock.h` 大小 | 结论 |
|------|-------------------------------|------|
| v3.19 | 84 B（404 占位页） | 文件不存在 |
| v4.0 | 84 B | 文件不存在 |
| v4.1 | 84 B | 文件不存在 |
| **v4.2** | **4207 B** | ⭐ **qspinlock 合入** |

同时看 v4.1 的 x86 实现，确认它当时还是 ticket lock：

```c
/* arch/x86/include/asm/spinlock.h @ v4.1:18 */
 * These are fair FIFO ticket locks, which support up to 2^16 CPUs.
```

```c
/* arch/x86/include/asm/spinlock.h @ v4.1:47 */
static inline void __ticket_enter_slowpath(arch_spinlock_t *lock)
```

### ⚠️ 顺手抓到一处**过时源码注释**

`arch/x86/include/asm/spinlock.h` 在 **v6.6** 里居然还留着 ticket lock 时代的注释：

```c
/* arch/x86/include/asm/spinlock.h @ v6.6:17-22 */
/*
 * Your basic SMP spinlocks, allowing only a single CPU anywhere
 *
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * These are fair FIFO ticket locks, which support up to 2^16 CPUs.   ← ❌ 错的
 *
 * (the type definitions are in asm/spinlock_types.h)                 ← ❌ 也不对，
 */                                                                      类型在 asm/qspinlock.h
```

v6.6 的 x86 默认走 `#include <asm/qspinlock.h>`，跟 FIFO ticket 没关系了。
**又一次印证：要数字看定义，别看注释。**

---

## 7. ⭐ 订正：qspinlock **不是严格 FIFO**，pending 位就是"插队"机制

书上（以及很多资料）的说法是「ticket lock / qspinlock 是公平锁，先到先得」。
**这话只对了后半段。**

### 插队点一：`pending` 位

看慢路径 ②③ 步：新来者只要看到 `val & ~_Q_LOCKED_MASK == 0`（即 tail 和 pending 都为空），
就去**占 pending 位**，然后在 locked byte 上自旋等持锁者释放。

也就是说：**即便队列里已经有 1 个人在排，只要那个 pending 位还空着，新来者仍然可以插到队列前面。**

这是刻意的性能设计 —— 缓存局部性：让"第二个到达者"在本地自旋，比让它去走一整套
MCS 入队/出队便宜得多。代价就是**轻微的不公平**。

### 插队点二：PV 模式下明确的 "lock stealing"

```c
/* kernel/locking/qspinlock_paravirt.h:403 */
	/*
	 * Set the pending bit in the active lock spinning loop to
	 * disable lock stealing before attempting to acquire the lock.
	 */
	set_pending(lock);
	for (loop = SPIN_THRESHOLD; loop; loop--) {
		if (trylock_clear_pending(lock))
			goto gotlock;
		cpu_relax();
	}
	clear_pending(lock);
```

注释里直接用了 **"lock stealing"（偷锁）** 这个词。`SPIN_THRESHOLD = (1 << 15) = 32768`
次自旋，偷不到才老实排队。而且 `qspinlock_paravirt.h` 的设计注释写明了原因：

> *A queue node vCPU will stop spinning if the vCPU in the previous node is
> not running. **The one lock stealing attempt allowed at slowpath entry
> mitigates the slight slowdown for non-overcommitted guest** with this
> aggressive wait-early mechanism.*

（虚拟机的持锁 vCPU 可能被宿主调度走，严格 FIFO 会导致整条队列干等 —— 所以允许偷。）

### 结论表

| 场景 | 公平性 |
|------|--------|
| 无争用 | 快路径 CAS，谁先谁拿 —— **但这是真并发，谈不上公平不公平** |
| 2 个竞争者 | pending 位保证了**大致** FIFO |
| ≥3 个竞争者（native） | MCS 队列 FIFO，**但 pending 位可被新来者抢占** |
| ≥2 个竞争者（PV guest） | 明确的 lock stealing，**可能饿死队头** |
| TAS / `virt_spin_lock` | 完全不公平，纯抢 |

**HFT 推论**：别把 `spinlock_t` 当公平队列用。需要严格 FIFO 时用 `queued_spinlock` 之外的
机制（比如 MCS 手写，或业务层自己排队）。

---

## 8. ⭐ 订正：PREEMPT_RT 上 `spin_lock()` **会睡眠**

书上那句「自旋锁绝不睡眠」在非 RT 内核成立。开了 `CONFIG_PREEMPT_RT` 就不成立了：

```c
/* include/linux/spinlock_rt.h:43 */
static __always_inline void spin_lock(spinlock_t *lock)
{
	rt_spin_lock(lock);
}
```

配合 §1 的类型定义 —— RT 下 `spinlock_t` 里装的是 `struct rt_mutex_base`。
于是：

| 配置 | `spin_lock()` 的行为 | 临界区里能睡眠吗 |
|------|---------------------|-----------------|
| 非 RT | 真自旋 + `preempt_disable()` | ❌ 不能 —— `BUG: scheduling while atomic` |
| **PREEMPT_RT** | 走 rt_mutex，**可睡眠** | ✅ 能 |
| 任意配置下的 `raw_spin_lock()` | 真自旋 | ❌ 不能 |

所以 RT 内核里「持 spinlock 时不能调 `copy_from_user()` / `kmalloc(GFP_KERNEL)`」
这条铁律**失效了**，但「持 `raw_spinlock` 时不能」依然成立。

> 这正是 `raw_spinlock_t` 存在的全部理由：**给 RT 内核里那些真的不能睡的地方留一把真自旋锁。**

### 非 RT 上的睡眠检查实证

```c
/* kernel/sched/core.c:5904 */
static noinline void __schedule_bug(struct task_struct *prev)
{
	/* Save this before calling printk(), since that will clobber it */
	unsigned long preempt_disable_ip = get_preempt_disable_ip(current);

	if (oops_in_progress)
		return;

	printk(KERN_ERR "BUG: scheduling while atomic: %s/%d/0x%08x\n",
		prev->comm, prev->pid, preempt_count());

	debug_show_held_locks(prev);
	...
}
```

判据就是 **`preempt_count() != 0`**。`spin_lock()` 里的 `preempt_disable()` 会让
`preempt_count` 自增，所以一旦在临界区里触发调度，这条 `printk` 就会打出来。
注意它还会调 `debug_show_held_locks()` 把当前持有的锁全打出来 —— 这是排查时的第一手线索。

---

## 9. UP vs SMP：宏展开完全不是一回事

`spin_lock()` 在 UP 上**根本不产生任何原子指令**。

```c
/* include/linux/spinlock_api_up.h:30 */
#define __LOCK(lock) \
  do { preempt_disable(); ___LOCK(lock); } while (0)

#define __LOCK_BH(lock) \
  do { __local_bh_disable_ip(_THIS_IP_, SOFTIRQ_LOCK_OFFSET); ___LOCK(lock); } while (0)

#define __LOCK_IRQ(lock) \
  do { local_irq_disable(); __LOCK(lock); } while (0)

#define __LOCK_IRQSAVE(lock, flags) \
  do { local_irq_save(flags); __LOCK(lock); } while (0)
```

而 `___LOCK` 在非 debug 下是个**空操作**：

```c
/* include/linux/spinlock_api_up.h:28 */
#define ___LOCK(lock) \
  do { __acquire(lock); (void)(lock); } while (0)
```

`__acquire()` 只给 sparse 做 annotation，编译出来什么都没有。

### ⭐ 订正：`spin_trylock()` 在 UP 上**恒返回 1**

```c
/* include/linux/spinlock_api_up.h:72 */
#define _raw_spin_trylock(lock)			({ __LOCK(lock); 1; })
#define _raw_read_trylock(lock)			({ __LOCK(lock); 1; })
#define _raw_write_trylock(lock)		({ __LOCK(lock); 1; })
#define _raw_spin_trylock_bh(lock)		({ __LOCK_BH(lock); 1; })
```

**永远是 1（成功）**。这意味着：

> 在 UP 内核上，所有 `if (!spin_trylock(&l)) { ...失败分支... }` 里的失败分支
> **是死代码**，编译器会直接优化掉。而 SMP 上它会正常执行。
> → 任何依赖 trylock 失败路径做「降级 / 统计 / 回退」的逻辑，**必须 SMP + UP 双编译验证**，
> 否则 UP 编译能过、SMP 上炸，或者反过来。

### SMP 侧的展开

x86_64 没开 `CONFIG_GENERIC_LOCKBREAK`，所以走 `spinlock_api_smp.h` 的内联版：

```c
/* include/linux/spinlock_api_smp.h:121 */
static inline void __raw_spin_lock(raw_spinlock_t *lock)
{
	preempt_disable();
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
}

static inline unsigned long __raw_spin_lock_irqsave(raw_spinlock_t *lock)
{
	unsigned long flags;
	local_irq_save(flags);
	preempt_disable();
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
	return flags;
}

static inline void __raw_spin_lock_bh(raw_spinlock_t *lock)
{
	__local_bh_disable_ip(_RET_IP_, SOFTIRQ_LOCK_OFFSET);
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
}
```

两点观察：

1. **`LOCK_CONTENDED(lock, try, lock)` 是先 trylock 再退化为 spin**。
   所以无争用时 `spin_lock()` = `preempt_disable()` + 一次 `cmpxchg`，没有多余的读。
2. **`spin_lock_bh()` 里没有显式的 `preempt_disable()`** —— 因为
   `__local_bh_disable_ip()` 通过增加 `preempt_count` 的 softirq 域**隐含**禁了抢占。
   这是很多人的困惑点：`spin_lock()` 有 `preempt_disable`，`spin_lock_bh()` 没有，
   但两者都禁抢占。

`do_raw_spin_lock()` 在非 debug 下就是架构锁加个 MMIO 写屏障：

```c
/* include/linux/spinlock.h:184 */
static inline void do_raw_spin_lock(raw_spinlock_t *lock) __acquires(lock)
{
	__acquire(lock);
	arch_spin_lock(&lock->raw_lock);
	mmiowb_spin_lock();
}
```

`mmiowb_spin_lock()` 是为 MMIO 写乱序架构（部分 PowerPC / IA64）准备的，x86 上是空操作。

---

## 10. ⭐ 订正：虚拟机上慢路径默认被"劫持"成 test-and-set

这是本篇最反直觉的一条。`arch/x86/include/asm/qspinlock.h` 里有这么一段：

```c
/* arch/x86/include/asm/qspinlock.h —— CONFIG_PARAVIRT 下 */
DECLARE_STATIC_KEY_TRUE(virt_spin_lock_key);

#define virt_spin_lock virt_spin_lock
static inline bool virt_spin_lock(struct qspinlock *lock)
{
	if (!static_branch_likely(&virt_spin_lock_key))
		return false;

	/*
	 * On hypervisors without PARAVIRT_SPINLOCKS support we fall
	 * back to a Test-and-Set spinlock, because fair locks have
	 * horrible lock 'holder' preemption issues.
	 */

	do {
		while (atomic_read(&lock->val) != 0)
			cpu_relax();
	} while (atomic_cmpxchg(&lock->val, 0, _Q_LOCKED_VAL) != 0);

	return true;
}
```

三个要点：

1. **`virt_spin_lock_key` 是 `STATIC_KEY_TRUE`** —— **默认打开**。
   原生机器（native）和「vCPU 已绑定的 PV guest」会在初始化时关掉它。
2. 打开时，慢路径**第一步**就调 `virt_spin_lock()`，中了就直接 `return` ——
   **整个 qspinlock 逻辑（pending + MCS）都不执行**，退化成最朴素的 TAS。
3. 理由是注释里那句话：**fair locks have horrible lock 'holder' preemption issues**。
   在虚拟机里，持锁的 vCPU 可能被宿主调度走，队列里所有人干等到它被调度回来 ——
   公平锁反而把延迟放大到调度粒度（毫秒级）。TAS 虽然不公平，但至少谁能跑谁就抢到。

### HFT 直接结论

> **在云主机 / 容器上跑低延迟负载时，默认的自旋锁可能是 TAS 而非 qspinlock。**
> 想确认：查 `/proc/lock_stat` 与 `kernel/locking/qspinlock.c` 的 lockevent，
> 或者用 `nopvspin` 启动参数 + 确保 `virt_spin_lock_key` 被关（vCPU pinning 的场景）。
> 更彻底的：**别在 VM 上跑延迟敏感的自旋路径**，独占物理核是唯一可靠的答案。

---

## 11. 与中断配合（速查）

进程上下文持锁时若被中断，ISR 再抢 **同一把锁** → 死锁：

```
进程: spin_lock(L) ──► 临界区中
         │
      本地中断打进来
         │
ISR:  spin_lock(L) ──► 自旋等进程释放
         │
进程永远等 ISR 结束才能跑 ──► 死锁
```

| API | 抢占 | 本地中断 | softirq | 展开（SMP） |
|-----|------|---------|---------|------------|
| `spin_lock()` | **禁** | 不管 | 不管 | `preempt_disable()` + `LOCK_CONTENDED` |
| `spin_lock_irq()` | **禁** | **关**（无保存） | 关 | `local_irq_disable()` + `preempt_disable()` + ... |
| `spin_lock_irqsave()` | **禁** | **关 + 保存 flags** | 关 | `local_irq_save(flags)` + `preempt_disable()` + ... |
| `spin_lock_bh()` | **禁**（隐含） | 不管 | **禁** | `__local_bh_disable_ip()` + `LOCK_CONTENDED` |

| 场景 | 用哪个 |
|------|--------|
| 锁只被进程上下文碰 | `spin_lock()` |
| 锁也被 **hard IRQ** 碰 | `spin_lock_irqsave()` / `spin_lock_irq()` |
| 锁被 **softirq/tasklet** 碰，但 hard IRQ 不碰 | `spin_lock_bh()` |
| **拿不准** | `spin_lock_irqsave()`（最保守，代价是一条 `pushf`/`popf`） |

⚠️ **`spin_lock_irq()` 和 `spin_unlock_irq()` 是无条件开关中断**，不看调用前的状态。
如果调用前中断**本来就关着**，`spin_unlock_irq()` 会**把中断打开** —— 这是个隐蔽的坑，
所以用 `irqsave`/`irqrestore` 更安全。

---

## 12. 调试与观测

### `CONFIG_DEBUG_SPINLOCK` 会往锁里塞字段

```c
/* include/linux/spinlock_types_raw.h */
typedef struct raw_spinlock {
	arch_spinlock_t raw_lock;
#ifdef CONFIG_DEBUG_SPINLOCK
	unsigned int magic, owner_cpu;
	void *owner;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map dep_map;
#endif
} raw_spinlock_t;

#define SPINLOCK_MAGIC		0xdead4ead
#define SPINLOCK_OWNER_INIT	((void *)-1L)
```

开启后 `spinlock_t` 从 **4 字节**膨胀到几十字节，`spin_lock()` 会做 magic 校验和
owner 记录。**别在生产内核上开**——体积和路径长度都受影响，而且会改变 cacheline 布局，
掩盖真正的问题。

### lockdep 的 wait_type 标注

```c
# define RAW_SPIN_DEP_MAP_INIT(lockname)		\
	.dep_map = {					\
		.name = #lockname,			\
		.wait_type_inner = LD_WAIT_SPIN,	\
	}
# define SPIN_DEP_MAP_INIT(lockname)			\
	.dep_map = {					\
		.name = #lockname,			\
		.wait_type_inner = LD_WAIT_CONFIG,	\
	}
```

**`raw_spinlock` 用 `LD_WAIT_SPIN`**，**`spinlock` 用 `LD_WAIT_CONFIG`**。
`LD_WAIT_CONFIG` 的意思是「这个锁在不同配置下等待语义不同（非 RT 自旋 / RT 睡眠）」——
lockdep 据此判断「在持 X 锁时睡眠」是否合法。**这就是为什么 RT 上 `spin_lock` 能睡而
`raw_spin_lock` 不能睡，lockdep 还能正确报错。**

### 观测手段

| 手段 | 看什么 |
|------|--------|
| `perf lock` / `/proc/lock_stat` | 每把锁的争用次数、等待时间（`CONFIG_LOCK_STAT`） |
| `qspinlock_stat.h` 的 lockevent | `lock_pending` / `lock_slowpath` / `lock_no_node` / `lock_use_node2..4`，需 `CONFIG_LOCK_EVENT_COUNTS` |
| `lockdep` | 加锁顺序、IRQ-safety、睡眠违规 |
| lockevent 计数 | `lock_slowpath` 高 = 排队多；`lock_no_node` 非 0 = 有 >4 层嵌套（异常） |

---

## 13. `_Q_PENDING_LOOPS`：一个架构可覆盖的启发式

```c
/* kernel/locking/qspinlock.c:96 —— 通用默认值 */
#ifndef _Q_PENDING_LOOPS
#define _Q_PENDING_LOOPS	1
#endif

/* arch/x86/include/asm/qspinlock.h:11 —— x86 覆盖为 512 */
#define _Q_PENDING_LOOPS	(1 << 9)
```

注释解释了为什么**不能无限等**：

> *This heuristic is used to limit the number of lockword accesses made by
> `atomic_cond_read_relaxed` when waiting for the lock to transition out of the
> "== `_Q_PENDING_VAL`" state. **We don't spin indefinitely because there's no
> guarantee that we'll make forward progress.***

x86 敢给 512 是因为它的 `cmpxchg` 延迟已知且低；通用值 1 是保守兜底。
**这是个典型的「理论 vs 工程」折中**：理论上等 pending 交接完最省事，
但没人保证对方不会被调度走，所以给个上限。

---

## HFT / 嵌入式关联

| 现象 | 机制解释 | 应对 |
|------|---------|------|
| **自旋锁的"公平"反而拖慢尾延迟** | PV guest 上持锁 vCPU 被宿主调度走 → 队列干等 | 独占物理核；或 `nopvspin` + 关 `virt_spin_lock_key` |
| **`spin_trylock()` 在 UP 上恒真** | `spinlock_api_up.h:72` 硬编码 `1` | 业务回退逻辑必须 SMP/UP 双编译验证 |
| **无争用 path 只有一条 `cmpxchg`** | `queued_spin_lock()` 快路径 | 临界区做到几条指令级，争用概率压到最低 |
| **解锁只写 1 字节** | `smp_store_release(&lock->locked, 0)` | 和 `pending`/`tail` 不冲突，少一次 RMW |
| **MCS 的 O(N) vs TAS 的 O(N²)** | 每人自旋在自己 node 上 | 高并发下 qspinlock 的扩展性来源 |
| **RT 上 `spin_lock()` 会睡** | `spinlock_t` = `rt_mutex_base` | 延迟敏感路径必须改用 `raw_spinlock_t` |
| **核数 > 队列深度时退化** | 4 个 node 上限，`lock_no_node` 兜底 | 看 lockevent 确认没打到兜底路径 |

**嵌入式侧**：单核（UP）系统上 `spinlock_t` 就是 `preempt_disable()`，零成本。
但要注意**中断延迟** —— `spin_lock_irqsave()` 关中断的时长直接等于最长关中断时间，
硬实时要求下必须测 `cyclictest` 的最坏关中断窗口。

---

## 实践模板

```c
#include <linux/spinlock.h>

struct ring {
	spinlock_t      lock;      /* 4 字节（非 debug、非 RT） */
	unsigned int    head, tail;
	unsigned char   buf[SIZE];
};

static DEFINE_SPINLOCK(g_ring_lock);      /* 静态 */
static struct ring g_ring;

static void ring_init(struct ring *r)
{
	spin_lock_init(&r->lock);            /* 动态 */
	r->head = r->tail = 0;
}

/* 场景 A：只被进程上下文访问 */
static int ring_push(struct ring *r, unsigned char c)
{
	unsigned long flags;
	int ret = -1;

	/* 不确定会不会被中断碰 → 用 irqsave */
	spin_lock_irqsave(&r->lock, flags);
	if (((r->head + 1) & (SIZE - 1)) != r->tail) {
		r->buf[r->head] = c;
		r->head = (r->head + 1) & (SIZE - 1);
		ret = 0;
	}
	spin_unlock_irqrestore(&r->lock, flags);   /* 必须传同一个 flags */
	return ret;
}

/* 场景 B：NAPI poll（softirq）与进程共享 → _bh 就够 */
static int ring_pop_bh(struct ring *r, unsigned char *c)
{
	spin_lock_bh(&r->lock);              /* 隐含禁抢占，禁 softirq，但不管 hard IRQ */
	if (r->head != r->tail) {
		*c = r->buf[r->tail];
		r->tail = (r->tail + 1) & (SIZE - 1);
		spin_unlock_bh(&r->lock);
		return 0;
	}
	spin_unlock_bh(&r->lock);
	return -1;
}

/* 场景 C：硬实时 / 调度器路径 → raw_spinlock，RT 上也不睡 */
static raw_spinlock_t g_raw_lock;
static void critical_no_sleep(void)
{
	unsigned long flags;
	raw_spin_lock_irqsave(&g_raw_lock, flags);
	/* 这里连 RT 内核都不能睡眠 */
	raw_spin_unlock_irqrestore(&g_raw_lock, flags);
}
```

**易错点核对表**：

| ❌ 错误 | ✅ 正确 |
|--------|--------|
| `spin_lock()` 保护一个会被 ISR 碰的数据 | 用 `spin_lock_irqsave()` |
| `spin_unlock_irq()` 与 `spin_lock_irqsave()` 混用 | 配对使用：`irqsave`↔`irqrestore`，`irq`↔`irq` |
| 临界区里 `copy_from_user()` / `kmalloc(GFP_KERNEL)` | 非 RT 上会 `BUG: scheduling while atomic` |
| 递归拿同一把 `spinlock_t` | 自死锁；qspinlock 连检测都不做（4 个 node ≠ 递归深度） |
| 拿 `spinlock_t` 当公平队列 | pending 位 + PV lock stealing 都可能插队 |
| 虚拟机上默认认为在用 qspinlock | `virt_spin_lock_key` 默认 TRUE，实际是 TAS |

---

→ **Ch 7–8** · [10.1 原子操作](./section-10.1-原子操作.md) · [10.3 rwlock](./section-10.3-读-写自旋锁.md) · [10.5 互斥体](./section-10.5-互斥体.md) · [10.11 选型](./section-10.11-选型速查Ch-9--Ch-10.md)

> ↔ [ULK Ch5 §4 自旋锁](../../../16-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-4-自旋锁.md)

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `spin_lock()` / `spin_lock_irqsave()` / `spin_lock_bh()` 什么时候用哪个？

<details><summary>答案</summary>

`spin_lock()`：进程上下文 + 确认无中断/softirq 访问同一锁。`spin_lock_irqsave(flags)`：
中断也可能访问同一锁。`spin_lock_bh()`：softirq 可能访问但 hard IRQ 不会。
如果不确定 → 用 `spin_lock_irqsave()`（最安全）。

<details><summary>按 v6.6 修订/补充</summary>

原文说的"UP 上 `spin_lock()` 退化为 `preempt_disable()`，`spin_lock_irqsave()` 退化为
`local_irq_save()`"是对的，但**不完整**。v6.6 的准确展开是：

```c
/* include/linux/spinlock_api_up.h:30 */
#define __LOCK(lock)			do { preempt_disable(); ___LOCK(lock); } while (0)
#define __LOCK_BH(lock)			do { __local_bh_disable_ip(_THIS_IP_, SOFTIRQ_LOCK_OFFSET); ___LOCK(lock); } while (0)
#define __LOCK_IRQ(lock)		do { local_irq_disable(); __LOCK(lock); } while (0)
#define __LOCK_IRQSAVE(lock, flags)	do { local_irq_save(flags); __LOCK(lock); } while (0)
```

三点补充：

1. `spin_lock_irqsave()` 是 `local_irq_save(flags)` **外加** `__LOCK(lock)`，
   而 `__LOCK` 里还有 `preempt_disable()`。所以它是 **关中断 + 禁抢占**，两件事都做。
2. `spin_lock_bh()` **没有显式的 `preempt_disable()`** —— `__local_bh_disable_ip()`
   通过增加 `preempt_count` 的 softirq 域隐含禁了抢占。这是最容易记错的一点。
3. UP 上 `___LOCK(lock)` 展开为 `do { __acquire(lock); (void)(lock); } while (0)`，
   `__acquire()` 只给 sparse 做标注，**编译出来是空的**。所以 UP 上 `spin_lock()`
   = 纯 `preempt_disable()`，一条指令都不碰锁变量。
</details>
</details>

**Q2.** 持 spinlock 时为什么不能睡眠？

<details><summary>答案</summary>

Spinlock 假设等待者会忙等（spin）。如果持锁者睡眠（`schedule()`）：① 等待者无限 spin 浪费 CPU。
② `schedule()` 检查 `preempt_count > 0` → `BUG: scheduling while atomic` → panic。
③ 如果切换到的进程也请求同一锁 → 死锁。RT 内核把 spinlock 变成可睡眠锁后这个限制不成立。

<details><summary>按 v6.6 修订/补充</summary>

**② 的具体判据有源码**：`kernel/sched/core.c:5904` 的 `__schedule_bug()` 打的是
`preempt_count()` 的值，非零即触发。

**③ 要加一句限定**：RT 上限制解除，但**只对 `spinlock_t`**。`raw_spinlock_t`
在 RT 上仍然真自旋，持有时睡眠照样炸。RT 的类型定义在
`include/linux/spinlock_types.h`：

```c
#else /* !CONFIG_PREEMPT_RT */
/* PREEMPT_RT kernels map spinlock to rt_mutex */
typedef struct spinlock {
	struct rt_mutex_base	lock;
	...
} spinlock_t;
```

`include/linux/spinlock_rt.h:43`：
```c
static __always_inline void spin_lock(spinlock_t *lock)
{
	rt_spin_lock(lock);
}
```

**lockdep 能区分这两者**：`raw_spinlock` 的 dep_map 用 `LD_WAIT_SPIN`，
`spinlock` 用 `LD_WAIT_CONFIG`（`spinlock_types_raw.h`）。所以 RT 上
「持 `spin_lock` 睡眠」合法、「持 `raw_spin_lock` 睡眠」违规，lockdep 都能报对。
</details>
</details>

**Q3.** HFT 用户态的 spinlock 和内核有什么不同？

<details><summary>答案</summary>

用户态没有真正的 spinlock（不关中断/不禁抢占）。`std::atomic_flag` + test_and_set 自旋是最接近的。
区别：① 用户态 spinlock 被调度器抢占后仍 spin（浪费 CPU 时间片）。② 不能关中断防止抢占。
③ `sched_yield()` 可以让出 CPU 但不释放锁。HFT 用户态 spinlock 应限制在 <100ns 临界区，
超时改用 futex/mutex。

<details><summary>按 v6.6 修订/补充</summary>

内核 spinlock 比用户态多三样东西，这三样正是它能安全自旋的前提：

| 能力 | 用户态 | 内核 `spin_lock()` |
|------|--------|-------------------|
| 禁止本 CPU 抢占 | ❌ 做不到 | ✅ `preempt_disable()` |
| 关闭本地中断 | ❌ 做不到 | ✅ `spin_lock_irqsave()` 可选 |
| 知道自己被"调度走"了吗 | ❌ | ✅（`preempt_count` / PV 的 `vcpu_is_preempted`） |

第三点特别关键：内核在虚拟化下能感知「持锁的 vCPU 是不是被宿主调度走了」
（`arch/x86/include/asm/qspinlock.h` 的 `vcpu_is_preempted()`），
所以才能做 PV 的 wait-early / lock stealing。用户态**完全没有这个信息**，
这也是为什么用户态自旋锁在超线程/超售 VM 上尾延迟会爆炸。

HFT 结论：**用户态自旋锁只在"持锁者几乎不会在临界区被抢占"时才成立** ——
这需要 `isolcpus` + `NO_HZ_FULL` + RT 优先级 + 绑核共同保证，缺一个都不行。
</details>
</details>

**Q4.** qspinlock 的 32 位字是怎么划分的？为什么 `pending` 要给整整一个字节？

<details><summary>答案</summary>

划分（`NR_CPUS < 16K` 时）：

| 位 | 字段 |
|----|------|
| 0–7 | locked byte |
| 8 | pending |
| 9–15 | 未用（被 pending 那 8 位吃掉） |
| 16–17 | tail index（嵌套层级 0–3） |
| 18–31 | tail cpu（**+1 存储**） |

`pending` 占整字节的理由（`asm-generic/qspinlock_types.h` 注释逐字）：
*"By using the whole 2nd least significant byte for the pending bit, we can allow
better optimization of the lock acquisition for the pending bit holder."*

连带好处是解锁只写 `&lock->locked` 这一个 u8：
```c
static __always_inline void queued_spin_unlock(struct qspinlock *lock)
{
	smp_store_release(&lock->locked, 0);   /* 单字节 store，不碰 pending/tail */
}
```

同理 locked 也扩成一个字节：*"Even though we only need 1 bit for the lock, we extend
it to a full byte to achieve better performance for architectures that support atomic
byte write."*

`NR_CPUS >= 16K` 时布局会变（pending 缩到 1 位，tail idx 挪到 9–10，tail cpu 占 11–31），
所以必须走 `_Q_*_MASK` 宏，别硬编码偏移。

**tail cpu 为什么要 +1**：CPU 0 的 task 层（idx=0）编码为 0，会和"没有队尾"撞车，
`encode_tail()` 里 `(cpu + 1) << _Q_TAIL_CPU_OFFSET` 把 0 让出来当哨兵。
</details>

**Q5.** 为什么 per-CPU 只有 **4 个** MCS node？超过 4 层嵌套会怎样？

<details><summary>答案</summary>

因为上下文最多嵌套 4 层：**task → softirq → hardirq → NMI**。
源码注释：*"there is a limit to the contexts that can nest; namely: task, softirq,
hardirq, nmi. As there are at most 4 nesting levels, it can be encoded by a 2-bit number."*
这 2 位就是 tail index。

大小也刚好：64 位下 `struct mcs_spinlock` 16 字节 × 4 = 64 字节 = 一条 cacheline
（`DEFINE_PER_CPU_ALIGNED` 保证对齐）。开 PV 时 node 膨胀到 32 字节，占两条 line。

**超过 4 层的兜底**（源码自称"不优雅但够简单"）：

```c
if (unlikely(idx >= MAX_NODES)) {
	lockevent_inc(lock_no_node);
	while (!queued_spin_trylock(lock))
		cpu_relax();
	goto release;
}
```

即**退化成裸 TAS 自旋**，不用任何 MCS node。注释明确说这是针对"某些架构上 NMI
也可能嵌套拿锁"的极端情况。观测手段就是 lockevent 里的 `lock_no_node` 计数。
</details>

**Q6.** qspinlock 是严格 FIFO 吗？如果不是，"插队"发生在哪里？

<details><summary>答案</summary>

**不是。** 有两处插队：

**① pending 位（native 也有）** —— 慢路径第 ②③ 步：新来者只要看到
`val & ~_Q_LOCKED_MASK == 0`（tail 和 pending 都空），就去占 pending 位，
在 locked byte 上自旋。所以**即便队列已有 1 人，新来者只要抢到 pending 位就能插到前面**。

**② lock stealing（PV 专属，明写）** —— `qspinlock_paravirt.h:403` 的
`pv_wait_head_or_lock()`：

```c
	/*
	 * Set the pending bit in the active lock spinning loop to
	 * disable lock stealing before attempting to acquire the lock.
	 */
	set_pending(lock);
	for (loop = SPIN_THRESHOLD; loop; loop--) {
		if (trylock_clear_pending(lock))
			goto gotlock;
		cpu_relax();
	}
```

`SPIN_THRESHOLD = (1 << 15) = 32768`（`arch/x86/include/asm/spinlock.h:25`）。
设计注释给出理由：*"A queue node vCPU will stop spinning if the vCPU in the previous
node is not running. The one lock stealing attempt allowed at slowpath entry mitigates
the slight slowdown for non-overcommitted guest."*

**工程结论**：别把 `spinlock_t` 当公平队列。要严格 FIFO 得业务层自己排。
</details>

**Q7.** 为什么说"虚拟机上的自旋锁可能根本不是 qspinlock"？

<details><summary>答案</summary>

因为 `CONFIG_PARAVIRT` 下有个默认打开的 static key 会劫持整个慢路径：

```c
/* arch/x86/include/asm/qspinlock.h */
DECLARE_STATIC_KEY_TRUE(virt_spin_lock_key);    /* ← 默认 TRUE */

static inline bool virt_spin_lock(struct qspinlock *lock)
{
	if (!static_branch_likely(&virt_spin_lock_key))
		return false;

	/*
	 * On hypervisors without PARAVIRT_SPINLOCKS support we fall
	 * back to a Test-and-Set spinlock, because fair locks have
	 * horrible lock 'holder' preemption issues.
	 */
	do {
		while (atomic_read(&lock->val) != 0)
			cpu_relax();
	} while (atomic_cmpxchg(&lock->val, 0, _Q_LOCKED_VAL) != 0);

	return true;
}
```

而慢路径第一步就调它：

```c
	if (pv_enabled())
		goto pv_queue;
	if (virt_spin_lock(lock))
		return;                 /* ← 中了就直接返回，pending + MCS 全部跳过 */
```

理由就是注释那句：**公平锁在持锁者可能被宿主调度走的环境下，会把延迟放大到调度粒度**。
TAS 不公平，但谁能跑谁抢到，至少不空等。

**HFT 含义**：在云主机/容器上，`spinlock_t` 的争用行为与裸机上**完全不同** ——
是 O(N²) 的 cacheline 乒乓，不是 MCS 的 O(N)。要确认就查 lockevent /
`/proc/lock_stat`，或者 `nopvspin` + vCPU pinning 让 native 初始化关掉这个 key。
</details>

**Q8.** `spin_trylock()` 在 UP 内核上返回什么？这会带来什么坑？

<details><summary>答案</summary>

**恒返回 1（成功）。** `include/linux/spinlock_api_up.h:72`：

```c
#define _raw_spin_trylock(lock)			({ __LOCK(lock); 1; })
#define _raw_read_trylock(lock)			({ __LOCK(lock); 1; })
#define _raw_write_trylock(lock)		({ __LOCK(lock); 1; })
#define _raw_spin_trylock_bh(lock)		({ __LOCK_BH(lock); 1; })
```

**坑**：所有形如
```c
if (!spin_trylock(&l)) {
	stats->contended++;      /* UP 上这是死代码 */
	return -EAGAIN;
}
```
的失败分支，在 UP 编译下会被编译器**整体优化掉**。后果：

- 依赖 trylock 失败做**降级/回退**的逻辑，UP 上永远走不到 —— 行为与 SMP 不一致；
- 依赖 trylock 失败做**统计**的计数器，UP 上永远是 0；
- 更隐蔽的：某些 trylock 失败路径里会做**资源清理**，UP 上不会执行，
  如果 SMP 路径依赖这个清理的副作用，就会出现"只在 UP 上"的诡异 bug。

**对策**：任何用到 `spin_trylock()` 的代码，**必须 SMP 和 UP 各编译一次**做验证。
内核的 `CONFIG_SMP=n` 编译（比如 `allnoconfig` 系列）就是抓这类问题的。
</details>

**Q9.** 为什么 v6.6 的 `arch/x86/include/asm/spinlock.h` 注释是错的？怎么避免被它误导？

<details><summary>答案</summary>

v6.6 的这个文件还在：

```c
/* arch/x86/include/asm/spinlock.h @ v6.6:17-22 */
 * These are fair FIFO ticket locks, which support up to 2^16 CPUs.   ← ❌
 * (the type definitions are in asm/spinlock_types.h)                 ← ❌
```

两处都错：x86 从 **v4.2** 起默认走 `#include <asm/qspinlock.h>`，类型是
`struct qspinlock`（4 字节），跟 2^16 CPU 的 FIFO ticket lock 没关系；
而且类型定义在 `asm/qspinlock.h` → `asm-generic/qspinlock_types.h`，
不是 `asm/spinlock_types.h`。

用版本比对可以坐实这个断崖：

| 版本 | `asm-generic/qspinlock.h` | `arch/x86/.../spinlock.h` 类型 |
|------|--------------------------|-------------------------------|
| v4.1 | 不存在（404） | `__ticket_enter_slowpath()` — ticket lock |
| **v4.2** | **4207 字节** | qspinlock 合入 |

**避免被误导的通用做法**：注释会腐化，定义不会。要数字/行为，一律
① 抓当前版本源码看**定义**，② 用多版本同名文件比对坐实**断崖点**，
③ 注释和定义冲突时**信定义**，并把冲突记在笔记里。
（本仓库前几节已用同样的手法抓到过 khugepaged 的 "30 second"（实为 10000ms）
和 elf.h 的 "1GB"（实为 16GB）两处过时注释。）
</details>

---
