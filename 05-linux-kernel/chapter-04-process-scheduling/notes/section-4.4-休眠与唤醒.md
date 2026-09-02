## ④ 休眠与唤醒 · Sleeping and Waking

> 承接 [4.3 调度算法](./section-4.3-Linux-调度算法.md)。
> 本节回答：**任务怎么睡下去、怎么被叫醒、以及为什么"顺序"错一步就丢唤醒。**

任务不一定一直可运行：等 I/O、等锁、等事件时进入 **睡眠**，事件到达后 **唤醒** 再回运行队列。

Linux 里这件事的载体叫 **等待队列（wait queue）**。但等待队列不是唯一答案——内核里还有 **swait**（为确定性砍掉一半功能）、**wait_bit**（哈希表复用）、以及用户态的 **futex**（用 plist 而非 list）。本节从源码把这几条路一次讲透。

连贯顺序见本章 [README 推荐阅读](../README.md)。

---

### 一、任务状态位图：先看清"睡"有几种睡法

v6.6 `include/linux/sched.h:85-127` 里，任务状态是 **一个位图**（不是枚举），因此可以被"或"起来：

```c
/* Used in tsk->__state: */
#define TASK_RUNNING			0x00000000
#define TASK_INTERRUPTIBLE		0x00000001
#define TASK_UNINTERRUPTIBLE		0x00000002
#define __TASK_STOPPED			0x00000004
#define __TASK_TRACED			0x00000008
/* Used in tsk->exit_state: */
#define EXIT_DEAD			0x00000010
#define EXIT_ZOMBIE			0x00000020
#define EXIT_TRACE			(EXIT_ZOMBIE | EXIT_DEAD)
/* Used in tsk->__state again: */
#define TASK_PARKED			0x00000040
#define TASK_DEAD			0x00000080
#define TASK_WAKEKILL			0x00000100
#define TASK_WAKING			0x00000200
#define TASK_NOLOAD			0x00000400
#define TASK_NEW			0x00000800
#define TASK_RTLOCK_WAIT		0x00001000
#define TASK_FREEZABLE			0x00002000
#define __TASK_FREEZABLE_UNSAFE	       (0x00004000 * IS_ENABLED(CONFIG_LOCKDEP))
#define TASK_FROZEN			0x00008000
#define TASK_STATE_MAX			0x00010000
```

| 状态 | 值 | 含义 | 调度器可见？ |
|------|-----|------|--------------|
| **TASK_RUNNING** | `0x0000` | 可运行（含正在跑）。**注意值是 0**，不是"正在运行" | 在运行队列 / 红黑树上 |
| **TASK_INTERRUPTIBLE** | `0x0001` | 等事件，**可被任意信号打断** | **不在** 可运行集合 |
| **TASK_UNINTERRUPTIBLE** | `0x0002` | 等事件，信号也难打断（`kill -9` 无效，D 状态） | 同上 |
| **TASK_KILLABLE** | `0x0102` | `WAKEKILL \| UNINTERRUPTIBLE`：只被 **致命信号** 唤醒 | 同上 |
| **TASK_IDLE** | `0x0402` | `UNINTERRUPTIBLE \| NOLOAD`：睡且**不计入 loadavg** | 同上 |
| **TASK_STOPPED / TRACED** | `0x0104` / `0x0008` | 被 `SIGSTOP` / ptrace 停住 | 同上 |
| **TASK_RTLOCK_WAIT** | `0x1000` | RT 锁等待专用（见下） | 同上 |
| **TASK_FREEZABLE** | `0x2000` | 允许 freezer 冻结该睡眠 | 同上 |
| **TASK_WAKING / NEW** | `0x0200` / `0x0800` | 唤醒中 / 刚创建未上队列（内部态） | 过渡态 |

#### ⭐ 三个"复合宏"决定了唤醒语义

```c
/* Convenience macros for the sake of set_current_state: */
#define TASK_KILLABLE			(TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
#define TASK_STOPPED			(TASK_WAKEKILL | __TASK_STOPPED)
#define TASK_TRACED			__TASK_TRACED
#define TASK_IDLE			(TASK_UNINTERRUPTIBLE | TASK_NOLOAD)

/* Convenience macros for the sake of wake_up(): */
#define TASK_NORMAL			(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)
```

| 宏 | 组合 | 用在哪 |
|----|------|--------|
| `TASK_NORMAL` | `INTERRUPTIBLE \| UNINTERRUPTIBLE` | **`wake_up()` 的默认 mode**——两种睡眠都叫醒 |
| `TASK_KILLABLE` | `WAKEKILL \| UNINTERRUPTIBLE` | 想睡 D 状态但**保留"能被 kill"的退路** |
| `TASK_IDLE` | `UNINTERRUPTIBLE \| NOLOAD` | 内核线程常驻睡眠，别污染 loadavg |

> **关键理解**：唤醒判定是 `if (@state & p->__state)` —— **按位与**。所以 `wake_up()` 传 `TASK_NORMAL`（`0x3`）能同时命中 `0x1` 和 `0x2`；而 `wake_up_interruptible()` 传 `0x1` 就**只**命中可中断睡眠。这就是为什么"唤醒宏"和"睡眠宏"必须配对。

#### 状态扩展的版本断崖（实测）

| 特性 | 引入版本 | 判据（`sched.h` 探测） |
|------|---------|----------------------|
| **`TASK_KILLABLE`** | **v2.6.25** | v2.6.24 = 0 次 / v2.6.25 = 1 次命中 |
| **`TASK_RTLOCK_WAIT`** | **v5.15** | v5.14 = 0 / v5.15 = 3 次命中（文件 62510B → 66069B） |
| **`TASK_FREEZABLE`** | **v6.1** | v6.0 = 0 / v6.1 = 3 次命中（68580B → 69199B） |

> ⭐ `TASK_RTLOCK_WAIT` 出现的时间点（v5.15）与 PREEMPT_RT 主线化进程吻合——RT 上 `spinlock_t` 变睡眠锁后，必须有一种"我正在等 RT 锁、别按普通睡眠处理我"的独立状态位。

---

### 二、等待队列的两个结构体（v6.6 `include/linux/wait.h:20-41`）

```c
struct wait_queue_entry {
	unsigned int		flags;
	void			*private;
	wait_queue_func_t	func;
	struct list_head	entry;
};

struct wait_queue_head {
	spinlock_t		lock;
	struct list_head	head;
};
```

| 字段 | 作用 |
|------|------|
| `wait_queue_head.lock` | **一把 spinlock** 保护整条链表——所以唤醒路径是**原子上下文**，不能睡眠 |
| `wait_queue_head.head` | 等待者双向链表头 |
| `wait_queue_entry.private` | 通常指向等待的 `task_struct`（`current`） |
| `wait_queue_entry.func` | **唤醒回调**：`wake_up` 时对每个 entry 调用它，返回 <0 中止遍历 |
| `wait_queue_entry.entry` | 链表节点 |

#### ⭐ 6 个 flags（不是只有 EXCLUSIVE）

```c
#define WQ_FLAG_EXCLUSIVE	0x01
#define WQ_FLAG_WOKEN		0x02
#define WQ_FLAG_BOOKMARK	0x04
#define WQ_FLAG_CUSTOM		0x08
#define WQ_FLAG_DONE		0x10
#define WQ_FLAG_PRIORITY	0x20
```

| flag | 值 | 谁用 | 作用 |
|------|-----|------|------|
| **`WQ_FLAG_EXCLUSIVE`** | `0x01` | 等待者 | 独占等待——一次 `wake_up()` 只唤醒 **1 个** exclusive；**尾插** |
| `WQ_FLAG_WOKEN` | `0x02` | 唤醒路径 | 已被"标记唤醒"，避免二次唤醒 |
| **`WQ_FLAG_BOOKMARK`** | `0x04` | 唤醒路径 | **断点续扫**（见 §5），长队列不长期持锁，v4.14 引入 |
| `WQ_FLAG_CUSTOM` | `0x08` | 等待者 | `private` 里不是 `task_struct`，别用默认唤醒函数 |
| `WQ_FLAG_DONE` | `0x10` | 唤醒路径 | 该 entry 已处理完 |
| **`WQ_FLAG_PRIORITY`** | `0x20` | 等待者 | 优先唤醒（配合 `WQ_FLAG_EXCLUSIVE` 的排序） |

> 多数教材只讲 `EXCLUSIVE`，但 v6.6 实际有 6 个。`WQ_FLAG_CUSTOM` 常被忽略——它让"同一个等待队列上挂非 task 对象"成为可能（如 `wait_page` 机制）。

---

### 三、⭐ 防丢唤醒的核心：**先入队，后查条件**

这是整节最重要的一条。看 v6.6 `include/linux/wait.h:303` 的 `___wait_event`：

```c
#define ___wait_event(wq_head, condition, state, exclusive, ret, cmd)	\
({									\
	__label__ __out;						\
	struct wait_queue_entry __wq_entry;				\
	long __ret = ret;	/* explicit shadow */			\
	init_wait_entry(&__wq_entry, exclusive ? WQ_FLAG_EXCLUSIVE : 0);\
	for (;;) {							\
		long __int = prepare_to_wait_event(&wq_head, &__wq_entry, state);\
		if (condition)						\
			break;						\
		if (___wait_is_interruptible(state) && __int) {		\
			__ret = __int;					\
			goto __out;					\
		}							\
		cmd;							\
	}								\
	finish_wait(&wq_head, &__wq_entry);				\
__out:	__ret;								\
})
```

**顺序是：**
1. `prepare_to_wait_event()` —— **入队 + 设 `TASK_*SLEEP*` 状态**（持 `wq_head->lock`）
2. **然后** 才检查 `condition`
3. 条件不成立 → `cmd`（展开为 `schedule()`）
4. 条件成立 → `finish_wait()` 出队、状态改回 `TASK_RUNNING`

```
    ┌─ 1. prepare_to_wait_event()  ── 入队 + 设状态  ← 必须先做！
    │
    ├─ 2. if (condition) break      ← 后判条件
    │
    └─ 3. schedule()                ← 条件不成立才睡
```

#### 为什么顺序不能反？——看 futex 的官方论证

内核在用户态 futex 里写了**同一段逻辑**，并把理由写成了注释（v6.6 `kernel/futex/waitwake.c:577-593`）：

```c
	/*
	 * Access the page AFTER the hash-bucket is locked.
	 * Order is important:
	 *
	 *   Userspace waiter: val = var; if (cond(val)) futex_wait(&var, val);
	 *   Userspace waker:  if (cond(var)) { var = new; futex_wake(&var); }
	 *
	 * The basic logical guarantee of a futex is that it blocks ONLY
	 * if cond(var) is known to be true at the time of blocking, for
	 * any cond.  If we locked the hash-bucket after testing *uaddr, that
	 * would open a race condition where we could block indefinitely with
	 * cond(var) false, which would violate the guarantee.
	 *
	 * On the other hand, we insert q and release the hash-bucket only
	 * after testing *uaddr.  This guarantees that futex_wait() will NOT
	 * absorb a wakeup if *uaddr does not match the desired values
	 * while the syscall executes.
	 */
```

**两层机制的精确对照：**

| 层次 | 内核 wait_queue | 用户态 futex |
|------|----------------|-------------|
| 保护对象 | `wq_head->lock` | `hb->lock`（futex hash bucket） |
| 先做的事 | `prepare_to_wait_event()` 入队 + 设状态 | `futex_q_lock()` 拿桶锁 |
| 后做的事 | 判 `condition` | `futex_get_value_locked(&uval, uaddr)` 读用户内存 |
| 反了的后果 | 判完条件、准备睡之前被唤醒 → **唤醒丢失，永久睡眠** | "block indefinitely with cond(var) false" |
| 放锁时机 | 判完条件才放 | 判完值才放（值不对 → `-EWOULDBLOCK`） |

> ⭐⭐ **这是跨层的同一个模式**：先把自己挂进"可被唤醒"的集合、并处于"能被看见"的状态，**然后**再验证"是否真的需要睡"。验证与入队之间不能有窗口。

#### 对照：错误的写法长什么样

```c
/* ❌ 错误：先判条件后入队 —— 经典 lost wakeup */
if (!data_ready) {                    /* 判条件 */
        /* ⚠️ 这里如果发生 wake_up()，下面依然会睡下去 */
        prepare_to_wait(&wq, &entry, TASK_INTERRUPTIBLE);
        schedule();                   /* 永远没人叫醒 */
        finish_wait(&wq, &entry);
}

/* ✅ 正确：交给 wait_event 宏（内部即"先入队后判条件"） */
wait_event_interruptible(wq, data_ready);
```

---

### 四、`prepare_to_wait_event`：一个函数里藏了两条铁律

v6.6 `kernel/sched/wait.c:310`：

```c
long prepare_to_wait_event(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry, int state)
{
	unsigned long flags;
	long ret = 0;

	spin_lock_irqsave(&wq_head->lock, flags);
	if (signal_pending_state(state, current)) {
		/*
		 * Exclusive waiter must not fail if it was selected by wakeup,
		 * it should "consume" the condition we were waiting for.
		 *
		 * The caller will recheck the condition and return success if
		 * we were already woken up, we can not miss the event because
		 * wakeup locks/unlocks the same wq_head->lock.
		 *
		 * But we need to ensure that set-condition + wakeup after that
		 * can't see us, it should wake up another exclusive waiter if
		 * we fail.
		 */
		list_del_init(&wq_entry->entry);
		ret = -ERESTARTSYS;
	} else {
		if (list_empty(&wq_entry->entry)) {
			if (wq_entry->flags & WQ_FLAG_EXCLUSIVE)
				__add_wait_queue_entry_tail(wq_head, wq_entry);
			else
				__add_wait_queue(wq_head, wq_entry);
		}
		set_current_state(state);
	}
	spin_unlock_irqrestore(&wq_head->lock, flags);

	return ret;
}
```

**铁律 1 —— 一头一尾的插入策略**

| 等待者类型 | 插入方式 | 队列位置 | 语义 |
|-----------|---------|---------|------|
| 非 exclusive（默认） | `__add_wait_queue` | **头部** | 共享事件，`wake_up_all` 全唤醒；FIFO |
| **exclusive** | `__add_wait_queue_entry_tail` | **尾部** | 独占资源，一次只唤醒一个；**按排队顺序** |

> 为什么 exclusive 尾插？因为 `wake_up()` 只唤醒"遇到的第一个 exclusive"，而它从**头部**开始扫。尾插 = 先进先出 = 公平。

**铁律 2 —— ⭐ 被选中的 exclusive waiter 不能因信号临阵脱逃**

注释里那句 *"Exclusive waiter must not fail if it was selected by wakeup, it should 'consume' the condition we were waiting for"* 说的是：

- 一个 exclusive waiter 已被 `wake_up()` 选中（意味着"这份资源归你了"）
- 如果它此时因为有信号 pending 就直接 `-ERESTARTSYS` 返回
- 那这份资源就被**吞掉**了 —— 别的 exclusive waiter 还睡着，事件丢了

内核的解决办法：`___wait_event` 里 **先查 `condition`，再查信号**：

```c
	if (condition)			/* ← 先：被选中了？拿走条件，正常返回 */
		break;
	if (___wait_is_interruptible(state) && __int) {	/* ← 后：才有资格谈信号 */
		__ret = __int;
		goto __out;
	}
```

> ⭐ 顺序反过来的话：一个已经有信号 pending 的进程每次进 `prepare_to_wait_event` 都会立刻返回 `-ERESTARTSYS`，把唤醒名额白白浪费掉，队列后面的等待者被饿死。

---

### 五、唤醒侧：`__wake_up_common` 与 BOOKMARK

v6.6 `kernel/sched/wait.c:74`：

```c
static int __wake_up_common(struct wait_queue_head *wq_head, unsigned int mode,
			int nr_exclusive, int wake_flags, void *key,
			wait_queue_entry_t *bookmark)
{
	wait_queue_entry_t *curr, *next;
	int cnt = 0;

	lockdep_assert_held(&wq_head->lock);

	if (bookmark && (bookmark->flags & WQ_FLAG_BOOKMARK)) {
		curr = list_next_entry(bookmark, entry);

		list_del(&bookmark->entry);
		bookmark->flags = 0;
	} else
		curr = list_first_entry(&wq_head->head, wait_queue_entry_t, entry);

	if (&curr->entry == &wq_head->head)
		return nr_exclusive;

	list_for_each_entry_safe_from(curr, next, &wq_head->head, entry) {
		unsigned flags = curr->flags;
		int ret;

		if (flags & WQ_FLAG_BOOKMARK)
			continue;

		ret = curr->func(curr, mode, wake_flags, key);
		if (ret < 0)
			break;
		if (ret && (flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive)
			break;

		if (bookmark && (++cnt > WAITQUEUE_WALK_BREAK_CNT) &&
				(&next->entry != &wq_head->head)) {
			bookmark->flags = WQ_FLAG_BOOKMARK;
			list_add_tail(&bookmark->entry, &next->entry);
			break;
		}
	}

	return nr_exclusive;
}
```

#### ⭐ 陷阱：`wake_up_all` 的 `nr_exclusive = 0` 表示"全部"

```c
#define wake_up(x)			__wake_up(x, TASK_NORMAL, 1, NULL)
#define wake_up_nr(x, nr)		__wake_up(x, TASK_NORMAL, nr, NULL)
#define wake_up_all(x)			__wake_up(x, TASK_NORMAL, 0, NULL)   /* ← 0！ */
#define wake_up_interruptible(x)	__wake_up(x, TASK_INTERRUPTIBLE, 1, NULL)
#define wake_up_interruptible_all(x)	__wake_up(x, TASK_INTERRUPTIBLE, 0, NULL)
```

看判定 `!--nr_exclusive`：从 **0** 开始 `--` 得到 `-1`，**永远非 0**，于是永远不 `break` → 唤醒所有。

| 宏 | `nr_exclusive` | 唤醒几个 exclusive | 唤醒几个非 exclusive |
|----|---------------|------------------|-------------------|
| `wake_up()` | 1 | **1 个** | **全部** |
| `wake_up_nr(q, n)` | n | n 个 | 全部 |
| `wake_up_all()` | 0 | **全部** | 全部 |
| `wake_up_interruptible()` | 1 | 1 个（且只唤醒 `INTERRUPTIBLE` 态） | 全部 |

> ⭐ **`wake_up()` 永远唤醒所有"非 exclusive"等待者，只唤醒 1 个 exclusive** —— 这是 `WQ_FLAG_EXCLUSIVE` 的全部意义。

#### ⭐ BOOKMARK：长队列不长期持锁

```c
/* kernel/sched/wait.c:65 */
#define WAITQUEUE_WALK_BREAK_CNT 64
```

- 唤醒路径**持有 `wq_head->lock` 且关中断**（`spin_lock_irqsave`）
- 若队列上有 10 万个等待者（如热门 futex、大页锁），一次性扫完 → **中断关闭时间不可控**
- 解法：每扫 **64 个**就插一个 `WQ_FLAG_BOOKMARK` 节点，放锁、开中断一轮，下次从 bookmark 后继续

**版本断崖**：`WQ_FLAG_BOOKMARK` 在 **v4.14** 引入（实测：`wait.h` 命中数 v4.13 = 0 / v4.14 = 1，文件 35926B → 36126B）。

> 这跟 swait（§8）是**同一个问题的两种解法**：BOOKMARK 是"打断点续扫"，swait 是"换个数据结构让唤醒有界"。

---

### 六、`autoremove_wake_function`：唤醒即出队

```c
int autoremove_wake_function(struct wait_queue_entry *wq_entry, unsigned mode, int sync, void *key)
{
	int ret = default_wake_function(wq_entry, mode, sync, key);

	if (ret)
		list_del_init_careful(&wq_entry->entry);
	return ret;
}
```

- `default_wake_function` → 本质是 `try_to_wake_up(entry->private, mode, wake_flags)`
- 成功后用 `list_del_init_careful` 摘队 —— **`_careful` 版本保证并发的遍历者不会看到半截链表**（配合 `list_for_each_entry_safe_from` 的安全遍历）
- 这就是 `DEFINE_WAIT()` + `wait_event*()` 默认用的回调：睡醒自动出队，不用手动 `finish_wait` 摘

> `finish_wait()` 的作用是处理"**没被唤醒就退出循环**"的情况（条件本来就成立），此时 entry 还在队列上，必须手动摘掉。

---

### 七、虚假唤醒：内核官方说"多转一圈，cond 测试会救你"

`include/linux/sched.h:175-212` 的注释把这件事讲透了：

```c
/*
 * set_current_state() includes a barrier so that the write of current->__state
 * is correctly serialised wrt the caller's subsequent test of whether to
 * actually sleep:
 *
 *   for (;;) {
 *	set_current_state(TASK_UNINTERRUPTIBLE);
 *	if (CONDITION)
 *	   break;
 *
 *	schedule();
 *   }
 *   __set_current_state(TASK_RUNNING);
 *
 * If the caller does not need such serialisation (because, for instance, the
 * CONDITION test and condition change and wakeup are under the same lock) then
 * use __set_current_state().
 *
 * The above is typically ordered against the wakeup, which does:
 *
 *   CONDITION = 1;
 *   wake_up_state(p, TASK_UNINTERRUPTIBLE);
 *
 * where wake_up_state()/try_to_wake_up() executes a full memory barrier before
 * accessing p->__state.
 *
 * Wakeup will do: if (@state & p->__state) p->__state = TASK_RUNNING, that is,
 * once it observes the TASK_UNINTERRUPTIBLE store the waking CPU can issue a
 * TASK_RUNNING store which can collide with __set_current_state(TASK_RUNNING).
 *
 * However, with slightly different timing the wakeup TASK_RUNNING store can
 * also collide with the TASK_UNINTERRUPTIBLE store. Losing that store is not
 * a problem either because that will result in one extra go around the loop
 * and our @cond test will save the day.
 */
```

#### ⭐ 三点关键结论

| 结论 | 含义 |
|------|------|
| **`set_current_state()` 带屏障** | 保证"写状态"与"后续读 CONDITION"不乱序 |
| **条件与唤醒同锁时可用 `__set_current_state()`** | 无屏障、更快——锁已经提供了顺序 |
| ⭐ **丢掉一个 store 也没事** | *"Losing that store ... will result in one extra go around the loop and our @cond test will save the day"* —— **内核官方承认：正确性靠循环 + cond 兜底，不靠内存序的完美** |

#### 虚假唤醒的四个来源

| 来源 | 说明 |
|------|------|
| **信号打断** | `TASK_INTERRUPTIBLE` 下任意信号都会醒 |
| **多人等同一队列** | `wake_up_all` 全叫醒，只有第一个真正拿到条件 |
| **条件已变** | 醒来时条件又被别人抢走（如资源池只剩 1 个） |
| **内存序的 store 冲突** | 上面的注释：两个 store 碰撞，白转一圈 |

| 规则 | 做法 |
|------|------|
| 醒来后 **必须再检查条件** | `while (!condition) sleep_again;`（内核里是 `for(;;)` + `break`） |
| 别用 `if` | `if` 只判一次 → 虚假唤醒后带着错误前提往下走 |

---

### 八、swait：砍掉一半功能，换来确定性

v6.6 `include/linux/swait.h:12-40` 的头部注释——**这段是内核里少见的"设计取舍自白"**：

```c
/*
 * Simple waitqueues are semantically very different to regular wait queues
 * (wait.h). The most important difference is that the simple waitqueue allows
 * for deterministic behaviour -- IOW it has strictly bounded IRQ and lock hold
 * times.
 *
 * Mainly, this is accomplished by two things. Firstly not allowing swake_up_all
 * from IRQ disabled, and dropping the lock upon every wakeup, giving a higher
 * priority task a chance to run.
 *
 * Secondly, we had to drop a fair number of features of the other waitqueue
 * code; notably:
 *
 *  - mixing INTERRUPTIBLE and UNINTERRUPTIBLE sleeps on the same waitqueue;
 *    all wakeups are TASK_NORMAL in order to avoid O(n) lookups for the right
 *    sleeper state.
 *
 *  - the !exclusive mode; because that leads to O(n) wakeups, everything is
 *    exclusive. As such swake_up_one will only ever awake _one_ waiter.
 *
 *  - custom wake callback functions; because you cannot give any guarantees
 *    about random code. This also allows swait to be used in RT, such that
 *    raw spinlock can be used for the swait queue head.
 *
 * As a side effect of these; the data structures are slimmer albeit more ad-hoc.
 * For all the above, note that simple wait queues should _only_ be used under
 * very specific realtime constraints -- it is best to stick with the regular
 * wait queues in most cases.
 */

struct swait_queue_head {
	raw_spinlock_t		lock;
	struct list_head	task_list;
};
```

#### 砍掉了什么，换到了什么

| 维度 | 普通 `wait_queue_head` | `swait_queue_head` |
|------|----------------------|-------------------|
| 锁类型 | `spinlock_t`（RT 上变睡眠锁） | ⭐ **`raw_spinlock_t`（RT 上仍是真自旋）** |
| 中断/持锁时间 | 遍历唤醒期间持续持锁（靠 BOOKMARK 缓解） | ⭐ **严格有界**：每次唤醒都放一次锁 |
| 混合睡眠态 | 允许（靠 mode 位掩码过滤，O(n) 查找） | ❌ 只允许 `TASK_NORMAL` |
| 非 exclusive 模式 | 允许 | ❌ **全部 exclusive**，`swake_up_one` 只醒 1 个 |
| 自定义回调 | 允许 `func` 任意 | ❌ 禁止（*"cannot give any guarantees about random code"*） |
| 队列长度 | 可很长 | 通常很短（1~2 个） |
| 适用场景 | **默认选择** | ⭐ **仅 RT 约束下**（注释原话：*"only ... under very specific realtime constraints"*） |

**版本断崖**：`include/linux/swait.h` 在 **v4.6** 引入（实测：v4.5 = 14 字节 404 残片 / v4.6 = 5148 字节且含 `swait_queue_head` 定义 16 处命中）。

> ⭐ **为什么 RT 需要它**：PREEMPT_RT 上 `spinlock_t` 会变成可睡眠的 rt-mutex。若等待队列头用 `spinlock_t`，唤醒路径就可能睡眠——但唤醒路径常常在原子上下文调用。用 `raw_spinlock_t` 保证唤醒路径永远不睡。这就是注释里 *"allows swait to be used in RT"* 的意思。

---

### 九、wait_bit：一张哈希表服务所有"等某个 bit"

不想为每个 bit 都建一个 `wait_queue_head`？内核用**全局哈希表**复用：

v6.6 `kernel/sched/wait_bit.c:8-19`：

```c
#define WAIT_TABLE_BITS 8
#define WAIT_TABLE_SIZE (1 << WAIT_TABLE_BITS)

static wait_queue_head_t bit_wait_table[WAIT_TABLE_SIZE] __cacheline_aligned;

wait_queue_head_t *bit_waitqueue(void *word, int bit)
{
	const int shift = BITS_PER_LONG == 32 ? 5 : 6;
	unsigned long val = (unsigned long)word << shift | bit;

	return bit_wait_table + hash_long(val, WAIT_TABLE_BITS);
}
```

| 事实 | 数字 / 说明 |
|------|------------|
| 哈希表大小 | **256 个** `wait_queue_head_t`（`1 << 8`） |
| 哈希输入 | `(unsigned long)word << shift \| bit`，32 位机 `shift=5`、64 位机 `shift=6` |
| 对齐 | `__cacheline_aligned` —— 避免桶之间 false sharing |
| 使用者 | 页回写（`PG_writeback`）、folio 锁（`PG_locked`）、`wait_on_bit()` 系列 |

#### ⭐ 哈希冲突 → 唤醒回调必须三重校验

```c
int wake_bit_function(struct wait_queue_entry *wq_entry, unsigned mode, int sync, void *arg)
{
	struct wait_bit_key *key = arg;
	struct wait_bit_queue_entry *wait_bit = container_of(wq_entry, struct wait_bit_queue_entry, wq_entry);

	if (wait_bit->key.flags != key->flags ||
			wait_bit->key.bit_nr != key->bit_nr ||
			test_bit(key->bit_nr, key->flags))
		return 0;

	return autoremove_wake_function(wq_entry, mode, sync, key);
}
```

因为不同的 `(word, bit)` 可能落到同一个桶，`wake_bit_function` 必须校验：

1. `key.flags`（即 `word` 地址）相同
2. `key.bit_nr`（位号）相同
3. `test_bit(bit_nr, flags)` 为 0 —— **bit 真的被清了**

> 第 3 条是关键：**哈希冲突导致的"叫错人"不会造成错误唤醒**，因为最后还要看 bit 本身。这是"用哈希换内存"必须付的正确性代价。

等待侧用 `test_bit_acquire` 收尾（带 acquire 语义的读，配合释放侧的 release）：

```c
	do {
		prepare_to_wait(wq_head, &wbq_entry->wq_entry, mode);
		if (test_bit(wbq_entry->key.bit_nr, wbq_entry->key.flags))
			ret = (*action)(&wbq_entry->key, mode);
	} while (test_bit_acquire(wbq_entry->key.bit_nr, wbq_entry->key.flags) && !ret);
```

——**又是"先入队后判条件"**（§3 的老朋友）。

---

### 十、futex：用户态的等待队列，但不是同一个数据结构

HFT 最关心的场景（条件变量、`pthread_mutex` 争用）在内核里走 futex。它**不用 `wait_queue_head`**，而是自建一套：

v6.6 `kernel/futex/futex.h:45-49, 96-110`：

```c
struct futex_hash_bucket {
	atomic_t waiters;
	spinlock_t lock;
	struct plist_head chain;		/* ⭐ 不是 list_head */
} ____cacheline_aligned_in_smp;

struct futex_q {
	struct plist_node list;			/* ⭐ 不是 list_head */
	struct task_struct *task;
	spinlock_t *lock_ptr;
	union futex_key key;
	struct futex_pi_state *pi_state;
	struct rt_mutex_waiter *rt_waiter;
	union futex_key *requeue_pi_key;
	u32 bitset;
	atomic_t requeue_state;
#ifdef CONFIG_PREEMPT_RT
	struct rcuwait requeue_wait;
#endif
} __randomize_layout;
```

#### ⭐⭐ 核心差异：`plist_head` 是**按优先级排序**的链表

| 对比项 | `wait_queue_head` | `futex_hash_bucket` |
|--------|------------------|-------------------|
| 链表 | `struct list_head`（FIFO / 尾插 exclusive） | ⭐ **`struct plist_head`（优先级有序）** |
| 唤醒 nr=1 时叫醒谁 | **队列最前面那个**（先来先到） | ⭐ **优先级最高那个** |
| 能否做优先级继承 | 不能 | 能（配合 `pi_state` / `rt_waiter` 做 PI futex） |
| 额外过滤 | mode 位掩码 | ⭐ `bitset` 掩码 + `futex_match(key)` 哈希冲突过滤 |
| 缓存布局 | 无特殊 | `____cacheline_aligned_in_smp` + `__randomize_layout` |

看唤醒侧 v6.6 `kernel/futex/waitwake.c:143-183`：

```c
	/* Make sure we really have tasks to wakeup */
	if (!futex_hb_waiters_pending(hb))
		return ret;			/* ⭐ 无锁快路径：atomic_t waiters == 0 直接返回 */

	spin_lock(&hb->lock);

	plist_for_each_entry_safe(this, next, &hb->chain, list) {
		if (futex_match(&this->key, &key)) {		/* ⭐ 哈希冲突过滤 */
			if (this->pi_state || this->rt_waiter) {
				ret = -EINVAL;			/* ⭐ PI futex 不能用普通 wake 唤醒 */
				break;
			}

			/* Check if one of the bits is set in both bitsets */
			if (!(this->bitset & bitset))
				continue;			/* ⭐ bitset 掩码过滤 */

			futex_wake_mark(&wake_q, this);		/* ⭐ 只入 wake_q，不直接唤醒 */
			if (++ret >= nr_wake)
				break;
		}
	}

	spin_unlock(&hb->lock);
	wake_up_q(&wake_q);					/* ⭐ 放锁后才真正唤醒 */
```

#### ⭐ `wake_q` 模式：把唤醒推迟到放锁之后

| | 普通 wait_queue | futex |
|---|----------------|-------|
| 唤醒回调在哪执行 | **持锁期间** `curr->func(...)` 直接调 | 持锁期间只做"摘链 + 入 `wake_q`" |
| 真正唤醒 | 锁内 | ⭐ **`spin_unlock` 之后** `wake_up_q()` |
| 持锁窗口 | 包含 `try_to_wake_up`（可能触发调度决策、IPI） | 只有链表操作 |
| 效果 | 简单但持锁久 | ⭐ **持锁时间有界**，与 swait 的目标一致 |

> ⭐ 三种机制都在解决同一个问题——**唤醒路径的延迟上界**：
> - `wait_queue` → BOOKMARK 断点续扫（v4.14）
> - `swait` → 换 `raw_spinlock` + 禁自定义回调（v4.6）
> - `futex` → `wake_q` 把唤醒挪到锁外

#### 等待侧：`futex_wait` 的完整形态

v6.6 `kernel/futex/waitwake.c:632-690`（节选）：

```c
int futex_wait(u32 __user *uaddr, unsigned int flags, u32 val, ktime_t *abs_time, u32 bitset)
{
	...
retry:
	ret = futex_wait_setup(uaddr, val, flags, &q, &hb);   /* 锁桶 + 校验 val */
	if (ret)
		goto out;

	futex_wait_queue(hb, &q, to);                          /* 睡下去 */

	ret = 0;
	if (!futex_unqueue(&q))                                /* 被唤醒者会摘队 */
		goto out;
	ret = -ETIMEDOUT;
	if (to && !to->task)
		goto out;
	/*
	 * We expect signal_pending(current), but we might be
	 * the victim of a spurious wakeup as well.
	 */
	if (!signal_pending(current))
		goto retry;                                    /* ⭐ 虚假唤醒 → 再转一圈 */

	ret = -ERESTARTSYS;
	...
}
```

注意 `goto retry` —— **用户态 futex 也承认虚假唤醒的存在**，处理方式与内核 `for(;;)` 循环完全一致。

---

### 十一、睡眠禁忌（驱动 / HFT 都要命）

| 禁止 | 原因 | 违反后果 |
|------|------|---------|
| **持 spinlock 时睡眠** | 自旋等待者空转；RT 上 spinlock 变 rt-mutex 后更糟 | 死锁 / 长时间关抢占 |
| **中断上下文睡眠** | 无"当前进程"可换下（`current` 无意义） | `BUG: scheduling while atomic` |
| **原子上下文（softirq / 持 `preempt_disable`）里睡眠** | 同 ISR 规则 | 同上 |
| **持 `raw_spinlock` 时睡眠** | raw spinlock 在 RT 上也是真自旋 | 硬死锁，连 RT 都救不了 |
| **唤醒路径持 `wq_head->lock` 时做重活** | 该锁是 `spin_lock_irqsave`，关中断 | 中断延迟尖峰 → **HFT 尾延迟** |

> **判定工具**：内核提供 `might_sleep()` / `might_sleep_if()` —— 在 `CONFIG_DEBUG_ATOMIC_SLEEP` 下，原子上下文中调用会打印警告。**写驱动时，任何可能睡眠的函数入口都应放一个 `might_sleep()`**。

---

### 十二、HFT 视角

| 内核机制 | 用户态对应 | HFT 关注点 |
|---------|-----------|-----------|
| `wait_queue` + `schedule()` | `pthread_cond_wait` / `pthread_mutex_lock` 争用路径 | **阻塞 = 休眠**，唤醒延迟 = 调度延迟 + 可能的 CPU 迁移 |
| `wake_up()` 只入运行队列 | `futex(FUTEX_WAKE)` | **不保证立即运行**，还差一次调度点 |
| `WQ_FLAG_EXCLUSIVE` | mutex 的"只放一个进来" | 避免惊群 |
| `plist_head`（futex） | PI mutex（`PTHREAD_PRIO_INHERIT`） | ⭐ **优先级反转的官方解法**；HFT 里控制面线程不该卡住行情线程 |
| `swait` / `wake_q` / BOOKMARK | — | ⭐ 都是"**降低唤醒路径延迟上界**"，与 Ch10 的锁延迟是同一条账 |

**热路径的三条实操结论：**

1. **别睡。** `mutex` 争用 → 用无锁队列（SPSC ring buffer）；`epoll_wait` → DPDK/忙轮询（用 CPU 换延迟）。
2. **必须睡时，睡得可控。** `SCHED_FIFO` + 绑核（`isolcpus`）+ 关该核的 `NO_HZ`，让唤醒后无需跨核 IPI 抢 CPU。
3. ⭐ **唤醒 ≠ 立即运行。** `try_to_wake_up()` 只是把任务放回队列，是否抢占 current 取决于 `check_preempt_curr()` 与 CFS 的 `sched_wakeup_granularity` 阈值。RT 任务走 `SCHED_FIFO` 时才有"立即抢占"的强保证。

→ [4.5 抢占与切换](./section-4.5-抢占与上下文切换.md) · [Ch 10 完成变量](../../chapter-10-sync-methods/notes/section-10.6-完成变量.md) · [Ch 10 互斥体](../../chapter-10-sync-methods/notes/section-10.5-互斥体.md) · [Ch 3 进程状态](../../chapter-03-process-management/notes/section-3.3-进程状态.md) · [Ch 11 定时器](../../chapter-11-timers/)

### 常见陷阱

1. 把「休眠」当浪费 CPU——休眠释放 CPU 给其他任务，是高效的资源利用
2. 混淆 `TASK_INTERRUPTIBLE` 和 `TASK_UNINTERRUPTIBLE`——前者可被信号唤醒，后者不可；**更微妙的是 `TASK_KILLABLE`（v2.6.25+）是两者的折中**
3. 以为唤醒后立即运行——唤醒只是把进程放回运行队列，是否立即运行取决于调度器
4. 以为 `wake_up_all()` 传的 `nr=0` 表示"0 个"——**它表示"全部"**（`!--nr_exclusive` 从 0 变 -1，永不 break）
5. 以为 `wake_up()` 只唤醒一个——**只唤醒 1 个 exclusive，但唤醒全部非 exclusive**
6. 手写 `if (!cond) { prepare_to_wait(); schedule(); }`——**顺序反了，会丢唤醒**；必须用 `wait_event*()` 宏
7. 醒来后用 `if` 而不是 `while` 检查条件——虚假唤醒会带着错误前提往下走
8. 以为 swait 是"更快的 wait_queue"——**它是"更确定性的 wait_queue"，代价是砍掉混合睡眠态、非 exclusive 模式、自定义回调三样东西**；官方明确说大多数情况该用普通等待队列

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `TASK_INTERRUPTIBLE` 和 `TASK_UNINTERRUPTIBLE` 的区别？

<details><summary>答案</summary>

INTERRUPTIBLE：可被信号唤醒（`kill -9` 有效），进程可响应异步事件。UNINTERRUPTIBLE：不可被信号唤醒（`kill -9` 无效），通常在等磁盘 I/O 等不可中断操作。`TASK_KILLABLE` 是 2.6.25+ 新增：可被致命信号唤醒但不被普通信号打断。HFT 避免在热路径上进入 UNINTERRUPTIBLE（D 状态，无法 kill）。

</details>

**Q2.** 唤醒抢占的阈值是什么？为什么需要？

<details><summary>答案</summary>

`sched_wakeup_granularity`（默认 1ms）。唤醒的进程 vruntime 比 current 小这个阈值时才抢占。太低（0）会导致频繁抢占（thrashing）；太高会导致交互延迟。HFT 用 SCHED_FIFO 不走 CFS，不受此影响。

</details>

**Q3.** HFT 如何避免热路径上的休眠/唤醒延迟？

<details><summary>答案</summary>

① 预分配所有资源（内存/连接/FD），避免运行时等资源。② 无锁队列代替 mutex（mutex 阻塞 = 休眠）。③ DPDK 用户态轮询代替 epoll_wait（epoll_wait 阻塞 = 休眠）。④ `SCHED_FIFO` 确保唤醒后立即运行（RT 优先级 > CFS）。

</details>

**Q4.** `wait_queue_entry` 有哪些 flags？各自什么作用？

<details><summary>答案</summary>

v6.6 `include/linux/wait.h:20-25` 定义了 6 个：

| flag | 值 | 作用 |
|------|-----|------|
| `WQ_FLAG_EXCLUSIVE` | 0x01 | 独占等待，一次只唤醒 1 个，**尾插** |
| `WQ_FLAG_WOKEN` | 0x02 | 已被标记唤醒，避免二次唤醒 |
| `WQ_FLAG_BOOKMARK` | 0x04 | 唤醒遍历的断点标记（v4.14 引入） |
| `WQ_FLAG_CUSTOM` | 0x08 | `private` 不是 `task_struct`，别用默认唤醒函数 |
| `WQ_FLAG_DONE` | 0x10 | 该 entry 已处理完 |
| `WQ_FLAG_PRIORITY` | 0x20 | 优先唤醒 |

多数教材只提 `EXCLUSIVE`，v6.6 实际有 6 个。

</details>

**Q5.** 为什么 `___wait_event` 必须"先 `prepare_to_wait_event` 入队，后检查 condition"？反过来会怎样？

<details><summary>答案</summary>

`___wait_event`（v6.6 `wait.h:303`）的顺序是：

```c
long __int = prepare_to_wait_event(&wq_head, &__wq_entry, state);  /* 先入队+设状态 */
if (condition)                                                     /* 后判条件 */
	break;
...
cmd;   /* schedule() */
```

**原因**：如果先判条件再入队，那么"判完条件（为假）→ 准备睡"之间存在窗口。唤醒者若在这个窗口里 `wake_up()`，等待者还没在队列上，这次唤醒就丢了；随后等待者照常 `schedule()` 睡下，**再也没人叫醒它** —— 这就是 lost wakeup。

入队操作必须在"判条件"之前，且两者被同一把 `wq_head->lock` 保护。

**同一模式在用户态 futex 上也存在**，`kernel/futex/waitwake.c:586-592` 的注释是官方论证：

> "The basic logical guarantee of a futex is that it blocks ONLY if cond(var) is known to be true at the time of blocking... If we locked the hash-bucket after testing \*uaddr, that would open a race condition where we could block indefinitely with cond(var) false."

**结论**：内核等待队列与用户态 futex 用的是**完全同构**的协议——先挂进"可被唤醒的集合"，再验证是否真的需要睡。

</details>

**Q6.** exclusive 等待者被 `wake_up()` 选中后，为什么不能因为有信号 pending 就直接返回 `-ERESTARTSYS`？

<details><summary>答案</summary>

`kernel/sched/wait.c:310` 的 `prepare_to_wait_event()` 里有一段 12 行注释专门解释：

> "Exclusive waiter must not fail if it was selected by wakeup, it should **'consume' the condition** we were waiting for."

**逻辑链**：

1. `wake_up()` 在队列上只唤醒 **1 个** exclusive waiter，意味着"这份资源/这次事件归你了"
2. 如果这个 waiter 因为有信号 pending 就直接返回 `-ERESTARTSYS`，它就**把这次唤醒吞掉了**
3. 队列后面还有别的 exclusive waiter 在睡，它们**不会**被唤醒（名额已经用掉）
4. 结果：**事件丢失，资源无人认领，可能的永久睡眠**

**内核的解法**：`___wait_event` 里**先查 condition，再查信号**：

```c
if (condition)                                          /* 先：被选中了？正常拿走 */
	break;
if (___wait_is_interruptible(state) && __int) {         /* 后：才有资格谈信号 */
	__ret = __int;
	goto __out;
}
```

即：只要条件已成立，就算有信号也算"成功消费"，不会把唤醒名额浪费掉。

</details>

**Q7.** `wake_up_all()` 传的 `nr_exclusive = 0` 为什么表示"唤醒全部"而不是"0 个"？

<details><summary>答案</summary>

看 `__wake_up_common` 的判定：

```c
if (ret && (flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive)
	break;
```

- `nr_exclusive` 从 **0** 开始，`--nr_exclusive` 得 `-1`
- `!(-1)` = `!真` = **假** → 永远不 `break`
- 于是遍历扫完整条队列 → 唤醒所有

而 `wake_up()` 传 1：`--1 = 0`，`!0 = 真` → 遇到**第一个** exclusive 就 break。

| 宏 | `nr_exclusive` | exclusive 唤醒数 | 非 exclusive 唤醒数 |
|----|---------------|----------------|-------------------|
| `wake_up()` | 1 | 1 个 | **全部** |
| `wake_up_nr(q,n)` | n | n 个 | 全部 |
| `wake_up_all()` | 0 | **全部** | 全部 |

⚠️ 另一个易错点：**`wake_up()` 永远唤醒所有非 exclusive 等待者**，只有 exclusive 受 `nr` 限制。这是 `WQ_FLAG_EXCLUSIVE` 存在的全部意义。

</details>

**Q8.** `WQ_FLAG_BOOKMARK` 解决什么问题？什么时候引入的？

<details><summary>答案</summary>

**问题**：唤醒路径持 `wq_head->lock`（`spin_lock_irqsave`，**关中断**）遍历队列。若队列上有大量等待者（热门 futex、大 folio 锁争用），一次扫完 → **中断关闭时间不可控**，造成调度/中断延迟尖峰。

**解法**：每扫 `WAITQUEUE_WALK_BREAK_CNT`（= **64**，定义在 `kernel/sched/wait.c:65`）个 entry，就插入一个带 `WQ_FLAG_BOOKMARK` 的占位节点，放锁、开中断一轮；下次进入 `__wake_up_common` 时从 bookmark 之后继续：

```c
if (bookmark && (bookmark->flags & WQ_FLAG_BOOKMARK)) {
	curr = list_next_entry(bookmark, entry);
	list_del(&bookmark->entry);
	bookmark->flags = 0;
} else
	curr = list_first_entry(&wq_head->head, wait_queue_entry_t, entry);
```

**版本**：**v4.14** 引入（实测 `wait.h` 命中数：v4.13 = 0 / v4.14 = 1，文件 35926B → 36126B）。

**同类思路**：swait（v4.6）用换数据结构的方式解决同样的问题，futex 用 `wake_q` 把唤醒推迟到放锁后。三者目标一致——**让唤醒路径的延迟有界**。

</details>

**Q9.** swait 相比普通等待队列砍掉了什么？为什么要砍？

<details><summary>答案</summary>

`include/linux/swait.h:12-40` 的头部注释是内核的"设计取舍自白"。核心目标是 **"strictly bounded IRQ and lock hold times"**（严格有界的中断/持锁时间）。

**砍掉的三样：**

| 砍掉的东西 | 原因（注释原文要点） |
|-----------|-------------------|
| 混合 INTERRUPTIBLE/UNINTERRUPTIBLE 睡眠 | 统一 `TASK_NORMAL`，"avoid O(n) lookups for the right sleeper state" |
| 非 exclusive 模式 | "leads to O(n) wakeups"，全部 exclusive，`swake_up_one` 只醒 1 个 |
| 自定义唤醒回调 `func` | "you cannot give any guarantees about random code" |

**换到的两样：**

1. `struct swait_queue_head` 用 **`raw_spinlock_t`** 而非 `spinlock_t` —— RT 上 `spinlock_t` 是可睡眠的 rt-mutex，而唤醒路径常在原子上下文；raw spinlock 保证唤醒路径永不睡眠
2. 每次唤醒都放一次锁，给高优先级任务运行机会

**版本**：**v4.6** 引入（实测 `include/linux/swait.h`：v4.5 = 14 字节 404 残片 / v4.6 = 5148 字节且含定义）。

⚠️ **官方警告**：注释最后明确说 *"simple wait queues should only be used under very specific realtime constraints — it is best to stick with the regular wait queues in most cases"*。**swait 不是"更快的 wait_queue"，是"更确定性的 wait_queue"，默认不该用。**

</details>

**Q10.** futex 为什么不用 `wait_queue_head`，而要自建 `futex_hash_bucket` + `plist_head`？

<details><summary>答案</summary>

四个原因，逐个对应源码：

**① 优先级排序（最核心）**

```c
struct futex_hash_bucket {
	atomic_t waiters;
	spinlock_t lock;
	struct plist_head chain;   /* ⭐ priority-sorted list */
};
```

`plist_head` 是**按优先级排序**的链表。因此 `futex_wake(..., nr_wake=1)` 唤醒的是**优先级最高**的等待者，而不是"先来先到"。这是 PI（优先级继承）futex 的基础设施——没有它就无法实现 `PTHREAD_PRIO_INHERIT`。普通 `wait_queue_head` 用的是 FIFO `list_head`，做不到。

**② 哈希冲突过滤**

不同的 futex word 可能落到同一个桶（256 个桶，`____cacheline_aligned_in_smp`）。唤醒时必须三重校验：

```c
if (futex_match(&this->key, &key)) {           /* key 匹配 */
	if (this->pi_state || this->rt_waiter) {   /* PI futex 不能用普通 wake 唤醒 */
		ret = -EINVAL; break;
	}
	if (!(this->bitset & bitset))              /* bitset 掩码过滤 */
		continue;
	...
}
```

**③ bitset 支持**

一个 futex word 上可以有多个不同 `bitset` 的等待者（`FUTEX_BITSET`），`FUTEX_WAKE_BITSET` 按掩码选择性唤醒——相当于"一个地址上多路复用多个条件变量"。等待队列做不到。

**④ 无锁快路径 + wake_q**

```c
if (!futex_hb_waiters_pending(hb))     /* atomic_t waiters == 0，连锁都不用拿 */
	return ret;
spin_lock(&hb->lock);
...
	futex_wake_mark(&wake_q, this);     /* 持锁期间只入队，不唤醒 */
spin_unlock(&hb->lock);
wake_up_q(&wake_q);                     /* ⭐ 放锁后才真正 wake_up_process */
```

对比普通等待队列在**持锁期间**直接调 `curr->func()`（内含 `try_to_wake_up`），futex 把唤醒推迟到放锁后，**持锁窗口只包含链表操作**。

| | `wait_queue_head` | `futex_hash_bucket` |
|---|------------------|-------------------|
| 链表 | `list_head`（FIFO） | `plist_head`（优先级有序） |
| nr=1 唤醒谁 | 队首（先来先到） | 优先级最高者 |
| 唤醒执行位置 | 锁内回调 | 锁外 `wake_up_q` |
| 额外过滤 | mode 位掩码 | key 哈希匹配 + bitset |

**HFT 含义**：PI futex 是用户态避免**优先级反转**的唯一官方手段——控制面低优先级线程持锁时，会被临时提升到等待者的优先级，不会把行情线程卡在锁上。

</details>

**Q11.** 内核官方怎么看待"虚假唤醒"？`set_current_state()` 里的屏障是必须的吗？

<details><summary>答案</summary>

`include/linux/sched.h:175-212` 的注释给出了官方答案，三点：

**① `set_current_state()` 带屏障，保证"写 state"与"后续读 CONDITION"不乱序**

```
	for (;;) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (CONDITION)          /* 这个读不能被前面的写越过 */
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);
```

**② 如果 CONDITION 判断、条件变更、唤醒都在同一把锁下，可以省掉屏障**

> "If the caller does not need such serialisation (because, for instance, the CONDITION test and condition change and wakeup are under the same lock) then use `__set_current_state()`."

`__set_current_state()` 是无屏障版本（只有 `WRITE_ONCE`），更快。**锁已经提供了顺序保证时用它。**

**③ ⭐ 丢掉一个 store 也没事——正确性靠循环兜底，不靠内存序完美**

> "Losing that store is not a problem either because that will result in **one extra go around the loop and our @cond test will save the day**."

这是内核最诚实的一句：**内存序允许出现"白转一圈"，只要 `cond` 测试存在，正确性就不受影响。** 这也是为什么等待循环必须写成 `for(;;)` / `while`，而不能是 `if`。

**虚假唤醒的四个来源汇总：**

| 来源 | 说明 |
|------|------|
| 信号打断 | `TASK_INTERRUPTIBLE` 下任意信号都会醒 |
| 多人等同一队列 | `wake_up_all` 全叫醒，只有第一个真拿到条件 |
| 条件被抢占 | 醒来时资源已被别人拿走 |
| store 冲突 | 上面的内存序碰撞，白转一圈 |

</details>

**Q12.** `wait_bit` 机制为什么要哈希？哈希冲突会不会"叫错人"？

<details><summary>答案</summary>

**为什么哈希**：内核里大量地方需要"等某个字的某一位被清掉"（页回写 `PG_writeback`、folio 锁 `PG_locked`）。若每个 bit 都建一个 `wait_queue_head_t`，内存开销不可接受。于是用**全局哈希表复用**：

```c
#define WAIT_TABLE_BITS 8
#define WAIT_TABLE_SIZE (1 << WAIT_TABLE_BITS)     /* 256 个桶 */

static wait_queue_head_t bit_wait_table[WAIT_TABLE_SIZE] __cacheline_aligned;

wait_queue_head_t *bit_waitqueue(void *word, int bit)
{
	const int shift = BITS_PER_LONG == 32 ? 5 : 6;
	unsigned long val = (unsigned long)word << shift | bit;
	return bit_wait_table + hash_long(val, WAIT_TABLE_BITS);
}
```

**会不会叫错人**：会落到同一个桶，但**不会错误唤醒**，因为 `wake_bit_function` 做三重校验：

```c
	if (wait_bit->key.flags != key->flags ||        /* ① word 地址相同？ */
			wait_bit->key.bit_nr != key->bit_nr ||  /* ② 位号相同？ */
			test_bit(key->bit_nr, key->flags))      /* ③ bit 真的被清了？ */
		return 0;                                   /* 不匹配 → 不唤醒 */

	return autoremove_wake_function(wq_entry, mode, sync, key);
```

第 ③ 条最关键：**即使哈希撞车，最后还要看 bit 本身的值**。bit 仍为 1 就不唤醒 —— 这就是"用哈希换内存"必须付的正确性代价，代价形式是**遍历桶内链表时的 O(n) 过滤**。

等待侧用 `test_bit_acquire`（带 acquire 语义）收尾，与释放侧的 release 配对：

```c
	} while (test_bit_acquire(wbq_entry->key.bit_nr, wbq_entry->key.flags) && !ret);
```

同样的 **"先 `prepare_to_wait` 入队、后 `test_bit` 判条件"** 顺序（Q5 的老朋友）。

</details>

</details>

---

> ↔ [ULK Ch7 §2 调度策略与抢占](../../../16-linux-kernel-deep/chapter-07-process-scheduling/notes/section-2-调度策略与抢占.md)
---
