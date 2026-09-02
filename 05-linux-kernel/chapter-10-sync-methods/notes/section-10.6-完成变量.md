## ⑥ 完成变量 · Completions

表达 **「某事件做完了」** — 一方等待完成，另一方发出完成信号。比「乱用等待队列 + 条件变量手搓」更贴语义。

| 角色 | 动作 |
|------|------|
| **等待方** | `wait_for_completion()` — 睡到完成 |
| **完成方** | `complete()` / `complete_all()` — 唤醒等待者 |

#### 典型场景

| 场景 | 例子 |
|------|------|
| 父等子做到某步 | 历史上与 `vfork` 等故事相关 |
| 驱动等硬件初始化线程 | 探针里等工作线程 `complete` |
| 模块卸载等引用归零 | 「最后一用户走了」 |

```
线程 A:  start work ──► wait_for_completion(&done)
线程 B:  ... finish ... ──► complete(&done)
              │
              └──► A 被唤醒继续
```

#### 与信号量/等待队列

| | completion | 裸 wait queue |
|--|------------|---------------|
| 语义 | **一次性/完成事件** 清晰 | 通用，易写错 |
| 多次 complete | `complete_all` 等变体 | 自己维护标志 |

注意：等待方通常在 **进程上下文**；完成方可以在原子上下文 `complete`（具体 API 约束以头文件为准）— 但等待方仍不能在 ISR 里 `wait_for_completion`。

**HFT：** 用户态 `promise/future`、一次性 latch 同类；热路径少用「等完成」睡眠，用无锁标志 + 忙等/轮询仅限微秒级且可证明正确。

---

> **本篇分工**：上面速查表**原样保留**。本篇往下**不复述**"完成变量是什么"，
> 只做七件事，全部用 v6.6 源码实证：
>
> ① 拆开 `struct completion`（**只有两个字段、24 字节**），并指出
> 它内部又是一把 `raw_spinlock_t` —— 这是 10.4 §1 那条规律的第三个实例；
> ② ⭐ **`done` 是计数器不是布尔量** —— 这是 completion 相对"裸等待队列 + 标志位"
> 的**核心价值**：`complete()` 可以在 `wait_for_completion()` **之前**调用而不丢事件；
> ③ ⭐ 讲清 `UINT_MAX` 这个魔法值：`complete_all()` 把它当"**永久完成**"标记，
> 代码里三处 `if (x->done != UINT_MAX)` 全是为了它；
> ④ 完整列出 **10 个等待变体的返回值**（各变体返回值语义都不一样，
> 抄错一个就是 bug）；
> ⑤ ⭐ `completion_done()` 里那个**"拿了锁立刻放、什么都不做"**的怪异写法 ——
> 它不是冗余，是防止 `complete()` 还在引用时内存被释放；
> ⑥ **版本断崖：v6.6 的 completion 用的是 `swait`，不是 `wait_queue`**
> （转换发生在 **v5.7**），并讲清为什么换成 swait；
> ⑦ 引出头文件里那段**官方对 completion vs semaphore 的定性**
> （"signal a completion" 不是 "exclusion"，所以不该用 semaphore）。
>
> 所有常量与代码均核对自缓存的 v6.6 源码，行号可查。

---

## 1. `struct completion` 只有 **两个字段、24 字节**

```c
/* include/linux/completion.h —— v6.6 */
struct completion {
	unsigned int done;
	struct swait_queue_head wait;
};
```

其中的 `swait_queue_head` 也很短：

```c
/* include/linux/swait.h */
struct swait_queue_head {
	raw_spinlock_t		lock;
	struct list_head	task_list;
};
```

x86-64 上算一下：

| 字段 | 大小 | 作用 |
|------|------|------|
| `unsigned int done` | 4 B | **计数器**（不是布尔量！见 §2） |
| `wait.lock`（`raw_spinlock_t`） | 4 B | 保护下面那个链表 |
| `wait.task_list`（`struct list_head`） | 16 B | 等待者 FIFO 队列 |
| **合计** | **24 B** | |

### 又是 `raw_spinlock_t` —— 10.4 §1 规律的第三个实例

在 10.4 §1 我们从信号量推导出一条规律：

> **所有"自己实现睡眠逻辑"的内核原语，内部锁一律用 `raw_spinlock_t`。**

completion 是这条规律的**第三个实例**（前两个是 semaphore 和 mutex）。
看它的等待循环在干什么：

```c
		do {
			if (signal_pending_state(state, current)) {
				timeout = -ERESTARTSYS;
				break;
			}
			__prepare_to_swait(&x->wait, &wait);
			__set_current_state(state);
			raw_spin_unlock_irq(&x->wait.lock);    /* ① 放锁 */
			timeout = action(timeout);             /* ② 睡 */
			raw_spin_lock_irq(&x->wait.lock);      /* ③ 醒来拿回 */
		} while (!x->done && timeout);
```

一模一样的"手工编排放锁→睡→拿锁"三段式，且用的是 `irq` 变体
（关中断下不能睡眠）。如果用会睡眠的 `spinlock_t`（RT 上），正确性无法保证。

**已验证的四个实例**（加上 10.5 的 mutex / rwsem）：

| 原语 | 内部锁 | 自己编排睡眠 |
|------|--------|-------------|
| `struct semaphore` | `raw_spinlock_t lock` | ✅ `___down_common` |
| `struct mutex` | `raw_spinlock_t wait_lock` | ✅ `__mutex_lock_common` |
| `struct rw_semaphore` | `raw_spinlock_t wait_lock` | ✅ `rwsem_down_*_slowpath` |
| **`struct completion`** | **`raw_spinlock_t wait.lock`** | ✅ **`do_wait_for_common`** |

**反过来说，这也解释了为什么 PREEMPT_RT 不用改 completion** ——
它本来就是睡眠锁，内部锁又是 `raw_spinlock_t`，没有任何要改的地方
（和 semaphore 同理，见 10.4 §10）。

---

## 2. ⭐ `done` 是**计数器**，不是布尔量

这是 completion 最容易被忽略、也最值钱的设计。

### 朴素做法（错误） vs completion（正确）

假设你用"裸等待队列 + 布尔标志"手搓一个"等硬件就绪"：

```c
/* ❌ 手搓版：有竞态 */
static bool hw_ready;
static DECLARE_WAIT_QUEUE_HEAD(hw_wq);

/* 等待方 */
wait_event(hw_wq, hw_ready);        /* 先检查条件，再睡 */

/* 完成方（ISR） */
hw_ready = true;
wake_up(&hw_wq);
```

这个写法看起来对，但它依赖 `wait_event()` 内部的"**先检查条件再睡**"顺序。
如果你自己写成了

```c
	prepare_to_wait(&hw_wq, &wait, TASK_UNINTERRUPTIBLE);
	if (!hw_ready)          /* ← 检查晚了 */
		schedule();
```

那么在 `prepare_to_wait()` 和 `if (!hw_ready)` 之间如果 ISR 已经
`hw_ready = true` + `wake_up()`，这次唤醒就**丢了** —— 因为那时你还没进
等待队列。这就是经典的 **lost wakeup**。

### completion 怎么解决：把"事件"计数，而不是置位

```c
static void complete_with_flags(struct completion *x, int wake_flags)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&x->wait.lock, flags);

	if (x->done != UINT_MAX)
		x->done++;                      /* ← 计数 +1，不是置位 */
	swake_up_locked(&x->wait, wake_flags);
	raw_spin_unlock_irqrestore(&x->wait.lock, flags);
}
```

等待侧消费：

```c
static inline long __sched
do_wait_for_common(struct completion *x,
		   long (*action)(long), long timeout, int state)
{
	if (!x->done) {                         /* ① 先查计数 */
		DECLARE_SWAITQUEUE(wait);

		do {
			if (signal_pending_state(state, current)) {
				timeout = -ERESTARTSYS;
				break;
			}
			__prepare_to_swait(&x->wait, &wait);   /* ② 再入队 */
			__set_current_state(state);
			raw_spin_unlock_irq(&x->wait.lock);
			timeout = action(timeout);            /* ③ 睡 */
			raw_spin_lock_irq(&x->wait.lock);
		} while (!x->done && timeout);
		__finish_swait(&x->wait, &wait);
		if (!x->done)
			return timeout;
	}
	if (x->done != UINT_MAX)
		x->done--;                          /* ④ 消费掉一个 */
	return timeout ?: 1;
}
```

**关键点 ②和③之间为什么不会丢唤醒**：因为整个序列是在
`x->wait.lock` **保护下**完成的（`__wait_for_common()` 在调用前就拿好锁了）。
ISR 里的 `complete()` 必须拿到同一把锁才能 `done++`，
所以它要么发生在你入队之前（那 `done` 已经 >0，你在 ① 就返回了），
要么发生在你睡眠之后（那 `swake_up_locked()` 会真的唤醒你）。
**不存在"你已入队但还没睡"这个中间态被 ISR 观察到的可能。**

### ⭐ 直接推论：`complete()` 可以在 `wait_for_completion()` 之前调用

这是 completion 相对裸等待队列最实用的优势：

```
场景：驱动 probe 启动一个工作线程，等它初始化完成

时间线（工作线程跑得特别快的情况）：
  主线程：init_completion(&done)
  主线程：kthread_run(worker)          ← worker 立刻跑完，调了 complete(&done)
                                          → done = 1
  主线程：wait_for_completion(&done)   ← ① 看到 done != 0，直接跳过等待
                                          → ④ done-- → 返回
  ✅ 事件没有丢失
```

**用裸 waitqueue + bool 也能做到，但你要自己保证"先查后睡"的顺序**；
completion 把这个保证**内建进了 API**。

### `done` 还能当"计数完成量"用

既然是计数器，那么：

```c
complete(&x);   /* done = 1 */
complete(&x);   /* done = 2 */
complete(&x);   /* done = 3 */

wait_for_completion(&x);   /* 消费 1 个，done = 2，立即返回 */
wait_for_completion(&x);   /* 消费 1 个，done = 1，立即返回 */
wait_for_completion(&x);   /* 消费 1 个，done = 0，立即返回 */
wait_for_completion(&x);   /* done == 0 → 睡眠等待 */
```

⚠️ **但这个"多次 complete 攒着"的用法要非常小心** —— 它和
"一个 completion 代表一个事件"的语义是冲突的（见 §8 的竞态讨论）。
实践中 99% 的场景是"一次 complete 对一次 wait"。

---

## 3. `complete()` vs `complete_all()`，以及 `UINT_MAX` 这个魔法值

### `complete()`：唤醒一个

```c
static void complete_with_flags(struct completion *x, int wake_flags)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&x->wait.lock, flags);

	if (x->done != UINT_MAX)
		x->done++;
	swake_up_locked(&x->wait, wake_flags);          /* 唤醒 1 个 */
	raw_spin_unlock_irqrestore(&x->wait.lock, flags);
}

void complete(struct completion *x)
{
	complete_with_flags(x, 0);
}
```

`swake_up_locked()` 的实现（`kernel/sched/swait.c:21`）—— **FIFO**：

```c
void swake_up_locked(struct swait_queue_head *q, int wake_flags)
{
	struct swait_queue *curr;

	if (list_empty(&q->task_list))
		return;

	curr = list_first_entry(&q->task_list, typeof(*curr), task_list);
	try_to_wake_up(curr->task, TASK_NORMAL, wake_flags);
	list_del_init(&curr->task_list);
}
```

配合入队侧的 `list_add_tail`：

```c
void __prepare_to_swait(struct swait_queue_head *q, struct swait_queue *wait)
{
	wait->task = current;
	if (list_empty(&wait->task_list))
		list_add_tail(&wait->task_list, &q->task_list);     /* ← 队尾入队 */
}
```

**`list_add_tail` + `list_first_entry` = FIFO**，与头文件注释一致：

> Completions currently use a **FIFO** to queue threads that have to wait for
> the "completion" event.

### `complete_all()`：唤醒全部 + 永久标记

```c
/**
 * complete_all: - signals all threads waiting on this completion
 * @x:  holds the state of this particular completion
 *
 * This will wake up all threads waiting on this particular completion event.
 * ...
 * Since complete_all() sets the completion of @x permanently to done
 * to allow multiple waiters to finish, a call to reinit_completion()
 * must be used on @x if @x is to be used again. The code must make
 * sure that all waiters have woken and finished before reinitializing
 * @x. Also note that the function completion_done() can not be used
 * to know if there are still waiters after complete_all() has been called.
 */
void complete_all(struct completion *x)
{
	unsigned long flags;

	lockdep_assert_RT_in_threaded_ctx();

	raw_spin_lock_irqsave(&x->wait.lock, flags);
	x->done = UINT_MAX;                 /* ← 不是 done++，是设成 UINT_MAX */
	swake_up_all_locked(&x->wait);
	raw_spin_unlock_irqrestore(&x->wait.lock, flags);
}
```

### ⭐ `UINT_MAX` 的三处判断

`done == UINT_MAX` 是"**永久完成**"的哨兵值。代码里**三处**都为它做了特判：

| 位置 | 代码 | 作用 |
|------|------|------|
| `complete_with_flags()` | `if (x->done != UINT_MAX) x->done++;` | **防止溢出**。已经是 UINT_MAX 就别再加了，否则绕回 0 |
| `do_wait_for_common()` | `if (x->done != UINT_MAX) x->done--;` | **永不被消费**。complete_all 之后所有等待者都能通过 |
| `try_wait_for_completion()` | `else if (x->done != UINT_MAX) x->done--;` | 同上 |

**为什么用 `UINT_MAX` 而不是一个单独的 bool？**
因为这样**不需要额外字段** —— `struct completion` 保持两个字段、24 字节。
代价是"计数"和"永久标记"两种语义挤在一个字段里，读代码时要记住这个约定。

**一个可以推出来的细节**：`complete()` 被调 `UINT_MAX` 次之后
`done` 会停在 `UINT_MAX`（因为 `if (x->done != UINT_MAX)` 挡住了），
于是这个 completion 意外地变成"永久完成"。当然实际上没人会调 40 亿次。

---

## 4. 等待侧状态机：`do_wait_for_common()` 逐行

```c
static inline long __sched
do_wait_for_common(struct completion *x,
		   long (*action)(long), long timeout, int state)
{
	if (!x->done) {                                  /* ① 快路径：已经有令牌 */
		DECLARE_SWAITQUEUE(wait);

		do {
			if (signal_pending_state(state, current)) {
				timeout = -ERESTARTSYS;
				break;
			}
			__prepare_to_swait(&x->wait, &wait);  /* ② 入队（尾插，FIFO） */
			__set_current_state(state);          /* ③ 置睡眠状态 */
			raw_spin_unlock_irq(&x->wait.lock);  /* ④ 放锁 */
			timeout = action(timeout);           /* ⑤ 睡 */
			raw_spin_lock_irq(&x->wait.lock);    /* ⑥ 醒来拿回锁 */
		} while (!x->done && timeout);               /* ⑦ 醒了还得再查 */
		__finish_swait(&x->wait, &wait);             /* ⑧ 出队 */
		if (!x->done)
			return timeout;                      /* 超时/被打断，没拿到 */
	}
	if (x->done != UINT_MAX)
		x->done--;                                   /* ⑨ 消费一个 */
	return timeout ?: 1;                                 /* ⑩ 返回值处理 */
}
```

几个要点：

**① 快路径是"先看 `done` 再决定要不要睡"。**
如果 `done` 已经非 0，整个 `if` 块都跳过，直接到 ⑨ 消费。
这就是 §2 讲的"complete 先于 wait 也不丢"的实现。

**③和④的顺序不能反。**
`__set_current_state(state)` 必须先于放锁。否则：放锁之后、置状态之前
如果 `complete()` 抢到锁调 `swake_up_locked()`，而你此时还是
`TASK_RUNNING`，这次唤醒就**不生效**（`try_to_wake_up` 对 RUNNING 的任务是 no-op），
然后你再去睡就睡死了。
**这是内核等待队列的通用纪律：`set_current_state()` 必须在放锁之前。**

**⑦ 醒了要再查一次** —— 和 10.4 §7 讲过的 `___down_common()` 的
`for(;;)` 是同一个道理：醒来原因可能是信号、超时、假唤醒，
只有 `x->done` 非 0 才算真的完成。

**⑩ `return timeout ?: 1;`** —— 这是 GCC 的 `?:` 省略中项扩展，
等价于 `return timeout ? timeout : 1;`。
意思是：**如果还剩超时时间就返回剩余 jiffies，如果已经归零就返回 1**。
这样调用者可以用 `> 0` 判断"成功完成"，同时还能知道剩了多少时间。
⚠️ 但对 `wait_for_completion()`（`timeout = MAX_SCHEDULE_TIMEOUT`）来说，
返回值是 `MAX_SCHEDULE_TIMEOUT`，它被丢弃了（函数返回 void）。

---

## 5. ⭐ 10 个等待变体的返回值（各不一样，抄错就是 bug）

`include/linux/completion.h` 导出了这些：

```c
extern void wait_for_completion(struct completion *);
extern void wait_for_completion_io(struct completion *);
extern int wait_for_completion_interruptible(struct completion *x);
extern int wait_for_completion_killable(struct completion *x);
extern int wait_for_completion_state(struct completion *x, unsigned int state);
extern unsigned long wait_for_completion_timeout(struct completion *x,
						   unsigned long timeout);
extern unsigned long wait_for_completion_io_timeout(struct completion *x,
						    unsigned long timeout);
extern long wait_for_completion_interruptible_timeout(
	struct completion *x, unsigned long timeout);
extern long wait_for_completion_killable_timeout(
	struct completion *x, unsigned long timeout);
extern bool try_wait_for_completion(struct completion *x);
extern bool completion_done(struct completion *x);
```

完整对照：

| API | 睡眠状态 | 超时 | 成功返回 | 超时返回 | 被打断返回 |
|-----|---------|------|---------|---------|-----------|
| `wait_for_completion()` | UNINTERRUPTIBLE | ❌ | （void） | — | — |
| `wait_for_completion_io()` | UNINTERRUPTIBLE | ❌ | （void） | — | — |
| `wait_for_completion_timeout()` | UNINTERRUPTIBLE | ✅ | **剩余 jiffies（≥1）** | **`0`** | — |
| `wait_for_completion_io_timeout()` | UNINTERRUPTIBLE | ✅ | **剩余 jiffies（≥1）** | **`0`** | — |
| `wait_for_completion_interruptible()` | INTERRUPTIBLE | ❌ | `0` | — | **`-ERESTARTSYS`** |
| `wait_for_completion_killable()` | KILLABLE | ❌ | `0` | — | **`-ERESTARTSYS`** |
| `wait_for_completion_state()` | 调用者指定 | ❌ | `0` | — | **`-ERESTARTSYS`** |
| `wait_for_completion_interruptible_timeout()` | INTERRUPTIBLE | ✅ | **剩余 jiffies（≥1）** | **`0`** | **`-ERESTARTSYS`（负数）** |
| `wait_for_completion_killable_timeout()` | KILLABLE | ✅ | **剩余 jiffies（≥1）** | **`0`** | **`-ERESTARTSYS`（负数）** |
| `try_wait_for_completion()` | **不睡** | — | `true` | — | `false`（没令牌） |
| `completion_done()` | **不睡** | — | `true`（已完成） | — | `false`（未完成） |

### 三个最容易踩的坑

**坑 1：超时版的返回值类型不一样**

```c
unsigned long wait_for_completion_timeout(struct completion *x, unsigned long timeout);   /* unsigned */
long wait_for_completion_interruptible_timeout(struct completion *x, unsigned long timeout); /* signed */
```

非 interruptible 的返回 **`unsigned long`**，interruptible 的返回 **`long`**。
因为后者要用负数表示 `-ERESTARTSYS`。

**所以判超时时**：
```c
/* ✅ interruptible_timeout：要分三态 */
long ret = wait_for_completion_interruptible_timeout(&x, HZ);
if (ret > 0)      { /* 完成，ret = 剩余 jiffies */ }
else if (ret == 0){ /* 超时 */ }
else              { /* 被打断，ret == -ERESTARTSYS */ }

/* ✅ timeout（非 interruptible）：两态 */
unsigned long ret = wait_for_completion_timeout(&x, HZ);
if (ret == 0)     { /* 超时 */ }
else              { /* 完成 */ }
```

⚠️ 千万别把 `wait_for_completion_timeout()` 的返回值赋给 `long` 然后判 `< 0` ——
它是 `unsigned long`，永远不为负。

**坑 2："成功"的返回值是剩余 jiffies，不是布尔真值**

`return timeout ?: 1;` 意味着成功时返回的是**剩余时间**（可能很大），
不是 `1`。只有剩余时间刚好为 0 时才返回 1（此时其实已经睡到点了，
但恰好 `done` 也置了）。所以判断只能写 `> 0`，不能写 `== 1`。

**坑 3：`wait_for_completion()` 是不可中断的**

它睡在 `TASK_UNINTERRUPTIBLE` 上且**没有超时**（`MAX_SCHEDULE_TIMEOUT`）。
这跟 10.4 §4 讲过的 `down()` 是**同一个问题** —— 如果那个 `complete()`
永远不来，任务就永久卡在 `D` 状态，`kill -9` 无效。

虽然 `wait_for_completion()` 没有像 `down()` 那样被标 deprecated，
但**在驱动里应该优先用带超时或可中断的变体**：

```c
/* ⚠️ 无上界，卡住就是 D 状态进程 */
wait_for_completion(&dev->probe_done);

/* ✅ 有上界 */
if (!wait_for_completion_timeout(&dev->probe_done, 5 * HZ))
	dev_err(dev, "probe timeout\n");

/* ✅ 可被 kill */
if (wait_for_completion_killable(&dev->probe_done))
	return -ERESTARTSYS;
```

---

## 6. `try_wait_for_completion()`：无锁快路径

```c
bool try_wait_for_completion(struct completion *x)
{
	unsigned long flags;
	bool ret = true;

	/*
	 * Since x->done will need to be locked only
	 * in the non-blocking case, we check x->done
	 * first without taking the lock so we can
	 * return early in the blocking case.
	 */
	if (!READ_ONCE(x->done))
		return false;                              /* ← 无锁快路径 */

	raw_spin_lock_irqsave(&x->wait.lock, flags);
	if (!x->done)
		ret = false;
	else if (x->done != UINT_MAX)
		x->done--;
	raw_spin_unlock_irqrestore(&x->wait.lock, flags);
	return ret;
}
```

**值得学习的地方**：先用 `READ_ONCE(x->done)` 做一次**无锁检查**，
如果是 0（"会阻塞"的情况）就直接返回，**完全不碰锁**。

注释说明了理由：

> Since x->done will need to be locked only in the non-blocking case,
> we check x->done first without taking the lock so we can **return early
> in the blocking case**.

也就是：**优化的是失败路径**。如果 `done == 0`（没令牌），
那这次调用注定返回 `false`，没必要花代价去抢一把自旋锁。

（顺带注意 `READ_ONCE()` 是必需的 —— 没有它，编译器可能把
`x->done` 的读取优化掉或重排，见 10.10 屏障那节。它是不睡的，
所以**可以在中断上下文调用**（对比 `wait_for_completion()` 绝对不行）。

---

## 7. ⭐ `completion_done()` 里那个"拿了锁立刻放"的怪异写法

看这个函数，会觉得最后两行很荒谬 —— **拿了自旋锁，什么都没做，立刻放掉**：

```c
/**
 *	completion_done - Test to see if a completion has any waiters
 *	@x:	completion structure
 *
 *	Return: 0 if there are waiters (wait_for_completion() in progress)
 *		 1 if there are no waiters.
 *
 *	Note, this will always return true if complete_all() was called on @X.
 */
bool completion_done(struct completion *x)
{
	unsigned long flags;

	if (!READ_ONCE(x->done))
		return false;

	/*
	 * If ->done, we need to wait for complete() to release ->wait.lock
	 * otherwise we can end up freeing the completion before complete()
	 * is done referencing it.
	 */
	raw_spin_lock_irqsave(&x->wait.lock, flags);
	raw_spin_unlock_irqrestore(&x->wait.lock, flags);
	return true;
}
```

### 为什么必须这样

**注释就是答案**：

> If ->done, we need to **wait for complete() to release ->wait.lock**
> otherwise we can end up **freeing the completion before complete() is
> done referencing it**.

推演一下那个竞态。假设有个 completion 在**栈上**（内核里很常见，
见 §8 的 `DECLARE_COMPLETION_ONSTACK`），等待方这样做：

```c
/* 等待方（典型用法）*/
DECLARE_COMPLETION_ONSTACK(done);
...
wait_for_completion(&done);       /* 睡 */
...
/* 函数返回 → done 这个栈变量生命周期结束 */

/* 完成方（另一个 CPU 或 ISR）*/
complete(&done);                  /* 拿 wait.lock → done++ → swake_up_locked → 放锁 */
```

问题在于：`wait_for_completion()` 返回时，`complete()` 那一侧**可能还没走完**
—— 它可能刚 `done++`，正准备 `swake_up_locked()`，或者刚唤醒完还没放锁。
此时等待方函数返回，栈帧销毁，而 `complete()` 还在读写 `&x->wait.lock` 和
`&x->wait.task_list` → **use-after-free / 栈内存踩踏**。

**`completion_done()` 里那两行就是把这个"等 complete() 走完"的动作补上**：
既然 `complete()` 全程持有 `wait.lock`，那么"我成功地拿到又放掉这把锁"
这件事本身就证明了 **"此刻没有 `complete()` 正在执行"**。

**这是一个很漂亮的技巧**：用"抢到锁"作为"对方已完成"的证明，
而不是引入额外的引用计数或 RCU。

### 使用规则

⚠️ **`completion_done()` 的名字有误导性** —— 它不是"查询有没有等待者"，
而是"**这个 completion 是否已完成，且可以安全销毁**"。

内核文档还有一条重要限定（在 `complete_all` 的 kernel-doc 里）：

> Also note that the function `completion_done()` **can not be used
> to know if there are still waiters** after `complete_all()` has been called.

也就是说 `complete_all()` 之后 `completion_done()` **恒返回 true**（因为
`done = UINT_MAX` 永不为 0），它**不能**告诉你"等待者们都醒了吗"。
要安全地 `reinit_completion()`，得用别的方法（见 §8）。

---

## 8. `reinit_completion()` 的竞态 —— 最危险的操作

```c
/**
 * reinit_completion - reinitialize a completion structure
 * @x:  pointer to completion structure that is to be reinitialized
 *
 * This inline function should be used to reinitialize a completion structure so it can
 * be reused. This is especially important after complete_all() is used.
 */
static inline void reinit_completion(struct completion *x)
{
	x->done = 0;
}
```

**就一行：把 `done` 清回 0。** 但它有严格的前置条件。

### 什么时候需要它

`complete_all()` 把 `done` 设成 `UINT_MAX`，而 `UINT_MAX` 是**永不被消费**的
（`if (x->done != UINT_MAX) x->done--`）。所以 `complete_all()` 之后，
这个 completion 就"永久完成"了，**不 reinit 就没法再用**。

### ⚠️ 竞态在哪

`complete_all` 的 kernel-doc 明确警告：

> Since complete_all() sets the completion of @x permanently to done
> to allow multiple waiters to finish, a call to reinit_completion()
> must be used on @x if @x is to be used again. **The code must make
> sure that all waiters have woken and finished before reinitializing
> @x.**

问题场景：

```
时间线（多个等待者的情况）：
  wait_list = [A, B, C]

  complete_all(&x)  → done = UINT_MAX，唤醒 A、B、C（都进了 runqueue，但都还没跑到）

  ⚠️ 此刻 A、B、C 的 do_wait_for_common() 还没执行到 ⑧ __finish_swait()
     他们的 task_list 节点还挂在 x->wait.task_list 上

  reinit_completion(&x) → done = 0

  A 醒来：⑧ __finish_swait（出队）→ ⑨ done != 0? 不，done 被清成 0 了
         → if (!x->done) return timeout  ← A 以为自己超时/失败了！
```

**A、B、C 会误判成"超时"**。这是真实的、会发生的 bug。

### 正确做法

`complete_all()` 后用 `wait_for_completion()` 之外的机制确保所有等待者"真的走完"了。
常见做法：

| 方法 | 说明 |
|------|------|
| **引用计数 + 另一个 completion** | 等待者走完后 `complete(&second_done)`，主等待方 `wait_for_completion(&second_done)` N 次 |
| **`kthread_stop()` 等自带机制** | 内核的 kthread 停止路径内部已经处理了这种同步 |
| **避免 `complete_all()`** | 如果只需要唤醒一个，用 `complete()` 就不需要 reinit |
| **`percpu_ref` / `kref`** | 计数归零回调里做清理，语义上更清晰 |

**实践中：能不用 `complete_all()` 就不用。**
它引入的"永久状态 + 需要 reinit + reinit 有竞态"这一串麻烦，
往往比它解决的问题更麻烦。多数"唤醒所有等待者"的需求，
用 `complete()` 循环调用、或者改用一个 `atomic_t` 标志 + `wake_up_all()`
反而更清楚。

---

## 9. 版本断崖：v6.6 用的是 **`swait`**，不是 `wait_queue`

这是会让人照着老代码写错的一条。

### 实测

抓多个版本的 `include/linux/completion.h` 统计关键词：

| 版本 | 文件大小 | `swait` 出现次数 | `wait_queue_head` 出现次数 |
|------|---------|-----------------|---------------------------|
| v4.9 | 3,557 B | **0** | **1** |
| v4.13 | 3,557 B | 0 | 1 |
| v4.14 | 4,858 B | 0 | 1 |
| v5.0 | 4,143 B | 0 | 1 |
| v5.5 | 4,143 B | 0 | 1 |
| v5.6 | 4,143 B | **0** | **1** |
| **v5.7** | 4,153 B | **3** | **2** ← 断点 |
| v5.10 | 4,153 B | 3 | 2 |
| v6.0 | 4,101 B | 3 | 2 |
| **v6.6** | 4,240 B | **3** | **2** |

**转换发生在 v5.7。** v5.6 及以前用的是 `wait_queue_head_t`，
v5.7 起换成 `struct swait_queue_head`。

### 为什么换成 swait（simple wait queue）

对比两个队列头：

```c
/* include/linux/wait.h —— 普通等待队列 */
struct wait_queue_head {
	spinlock_t		lock;
	struct list_head	head;
};
typedef struct wait_queue_head wait_queue_head_t;

struct wait_queue_entry {
	unsigned int		flags;       /* WQ_FLAG_EXCLUSIVE 等 */
	void			*private;    /* 指向 task_struct */
	wait_queue_func_t	func;        /* ← 回调函数指针 */
	struct list_head	entry;
};
```

```c
/* include/linux/swait.h —— 简单等待队列 */
struct swait_queue_head {
	raw_spinlock_t		lock;
	struct list_head	task_list;
};

struct swait_queue {
	struct task_struct	*task;       /* 直接是 task_struct * */
	struct list_head	task_list;
};
```

差异：

| | `wait_queue_head_t` | `swait_queue_head` |
|--|---------------------|---------------------|
| 锁 | `spinlock_t`（RT 上会睡眠） | **`raw_spinlock_t`** |
| 节点 | 有 `func` 回调 + `flags` | **只有 `task` + `task_list`** |
| exclusive 唤醒 | 支持 `WQ_FLAG_EXCLUSIVE` | **不支持**（一律 FIFO 逐个唤醒） |
| autoremove | 支持（`autoremove_wake_function`） | **不支持**（要手动 `__finish_swait`） |

**completion 用不上 `func` 回调、用不上 exclusive、用不上 autoremove** ——
它只需要"按 FIFO 唤醒任务"这一件事。所以用 swait 更省：
每个节点省掉一个函数指针 + flags，且不引入 `spinlock_t` 在 RT 上的复杂性。

### `swake_up_all_locked()` 的注释也点明了 completion 是个特例

```c
/*
 * Wake up all waiters. This is an interface which is solely exposed for
 * completions and not for general usage.
 *
 * It is intentionally different from swake_up_all() to allow usage from
 * hard interrupt context and interrupt disabled regions.
 */
void swake_up_all_locked(struct swait_queue_head *q)
```

两个信息：
1. `swake_up_all_locked()` 是**只为 completion 开的口子**，
   注释明说 "*not for general usage*"；
2. 它之所以叫 `_locked`，是因为调用方（completion）**已经拿着锁了**，
   这样就能在**硬中断上下文 / 关中断区域**里使用。

---

## 10. 官方对 completion vs semaphore 的定性

`kernel/sched/completion.c` 开头那段注释，是整个 API 的设计说明书，值得逐字读完：

```c
/*
 * Generic wait-for-completion handler;
 *
 * It differs from semaphores in that their default case is the opposite,
 * wait_for_completion default blocks whereas semaphore default non-block. The
 * interface also makes it easy to 'complete' multiple waiting threads,
 * something which isn't entirely natural for semaphores.
 *
 * But more importantly, the primitive documents the usage. Semaphores would
 * typically be used for exclusion which gives rise to priority inversion.
 * Waiting for completion is a typically sync point, but not an exclusion point.
 */
```

拆成三层：

| 层 | 内容 | 推论 |
|----|------|------|
| ① 默认行为相反 | `wait_for_completion()` **默认阻塞**；`down()` **默认不阻塞**（有令牌就走） | 这是两个原语的语义定位差异 |
| ② 一对多更自然 | `complete_all()` 一次唤醒所有 | semaphore 要唤醒多个得循环 `up()` |
| ③ ⭐ **语义即文档** | *"the primitive documents the usage"* | 看到 `struct completion` 就知道这是**同步点**；看到 `struct semaphore` 会以为是**互斥点** |

第三层是最深刻的：

> Semaphores would typically be used for exclusion which gives rise to
> **priority inversion**. Waiting for completion is a typically **sync point**,
> but **not an exclusion point**.

**为什么提到优先级反转？** 因为如果用一个"计数=1 的 semaphore"来当完成通知用，
那么它就变成了一个事实上的互斥锁 —— 而互斥锁会引发优先级反转问题
（高优先级任务等低优先级任务持有的锁）。
而 completion 的等待者**不持有任何东西**：
`wait_for_completion()` 醒来后没有"要释放"的义务，不存在"持锁者"这个概念。

**用一个 completion 表达"等某事做完"，读代码的人立刻明白这是同步；
用一个 semaphore 表达同样的事，读代码的人会去找"它在保护什么资源"。**

这就是为什么内核社区在"等待事件"场景一律推荐 completion。

### 三者定位总表

| | semaphore | mutex | **completion** |
|--|-----------|-------|----------------|
| 语义 | 资源计数 / 互斥 | **互斥**（有 owner） | **同步点**（一次性事件） |
| 初值 | N（可 >1） | 1 | **0**（"还没完成"） |
| 默认行为 | 有令牌就走 | 有就拿 | **默认阻塞** |
| 需要"释放"吗 | ✅ `up()` | ✅ `unlock()`（**必须同一任务**） | ❌ 完成方 `complete()`，等待方无需释放 |
| 优先级反转 | 会（当互斥用时） | 会（非 RT）/ 不会（RT） | **不会**（无持有者） |
| 一对多唤醒 | 循环 `up()` | 不支持 | ✅ `complete_all()` |
| 中断上下文 | ✅ `up()` | ❌ | ✅ `complete()` |

---

## HFT / 嵌入式关联

### completion 的延迟画像：和 semaphore 同量级

`wait_for_completion()` 的睡眠路径和 `down()` 是同一套
（`schedule_timeout` → 睡 → 醒 → 再查），所以延迟画像也一致：

```
wait_for_completion() 争用（即 complete() 还没来）时：
  ① 看 done == 0
  ② __prepare_to_swait 入队
  ③ __set_current_state + raw_spin_unlock_irq
  ④ schedule_timeout / schedule  → 上下文切换出去
  ⑤ ... 等 complete() ...
  ⑥ swake_up_locked → try_to_wake_up → 进 runqueue
  ⑦ ... 等调度器选中自己 ...
  ⑧ 上下文切换回来
  ⑨ raw_spin_lock_irq + 检查 done
  ⑩ done-- 消费，返回

量级：数 µs，方差大（取决于调度器）
```

**结论和 10.4 一致：热路径禁用。**

| 场景 | 用 |
|------|-----|
| 热路径（订单、行情处理） | ❌ completion。用 `atomic_t` 标志 + 重试，或 SPSC 无锁环 |
| **初始化 / probe / 配置加载** | ✅ completion 是**首选**（语义最清楚） |
| 模块卸载等引用归零 | ✅ |
| 等待硬件响应（进程上下文） | ✅ 但**必须带超时** |

### ⭐ `wait_for_completion()` 的 D 状态陷阱（嵌入式尤其严重）

和 10.4 §4 的 `down()` 是**完全同一个问题**：

```c
void __sched wait_for_completion(struct completion *x)
{
	wait_for_common(x, MAX_SCHEDULE_TIMEOUT, TASK_UNINTERRUPTIBLE);
}
```

`TASK_UNINTERRUPTIBLE` + `MAX_SCHEDULE_TIMEOUT` = **信号叫不醒、没有上界**。
如果硬件挂了、`complete()` 永远不来：

```
任务卡在 D 状态
→ kill -9 无效
→ ps 显示 "D"
→ 嵌入式设备上只能等看门狗重启
```

**规则（和 10.4 一致）**：驱动里**永远**用带超时或可中断的变体：

```c
/* ❌ 无上界 */
wait_for_completion(&dev->ready);

/* ✅ 有上界 */
if (!wait_for_completion_timeout(&dev->ready, 5 * HZ)) {
	dev_err(dev, "device not ready after 5s\n");
	return -ETIMEDOUT;
}

/* ✅ 可被 SIGKILL 兜底 */
if (wait_for_completion_killable(&dev->ready))
	return -ERESTARTSYS;
```

⚠️ 注意 `wait_for_completion_timeout()` 返回 **`unsigned long`**，
超时返回 **0**，成功返回 **≥1 的剩余 jiffies**。判超时只能写
`if (!ret)` 或 `if (ret == 0)`，**不能写 `if (ret < 0)`**（unsigned 永不为负）。

### ⭐ 栈上 completion 的生命周期 —— 最隐蔽的 use-after-free

`DECLARE_COMPLETION_ONSTACK()` 让你可以把 completion 放在栈上：

```c
#define COMPLETION_INITIALIZER_ONSTACK(work) \
	(*({ init_completion(&work); &work; }))

#define DECLARE_COMPLETION_ONSTACK(work) \
	struct completion work = COMPLETION_INITIALIZER_ONSTACK(work)
```

这很方便，但引入了 §7 分析的那个竞态：
**`wait_for_completion()` 返回时，对面的 `complete()` 可能还没执行完。**

典型错误（在驱动里非常常见）：

```c
int my_drv_probe(struct platform_device *pdev)
{
	DECLARE_COMPLETION_ONSTACK(done);        /* ← 栈上！ */

	start_async_init(dev, &done);            /* 交给工作线程/ISR 去 complete */
	wait_for_completion_timeout(&done, HZ);

	return 0;                                /* ⚠️ 函数返回，done 的栈内存失效 */
}                                            /*    而那个线程可能还在 complete(&done) */
```

**这个 bug 有多难查**：
- 大多数时候异步初始化很快，`complete()` 早就跑完了 → 没问题；
- 偶尔时序凑巧，`wait_for_completion_timeout()` 因**超时**返回
  （此时 `complete()` 还没被调用！）→ 函数返回 → 栈被复用 →
  后来 `complete()` 终于被调用 → 写到了别人的栈帧里 → **随机崩溃**。

**正确做法**（三选一）：

| 方法 | 说明 |
|------|------|
| **① 把 completion 放到设备结构体里** | 生命周期和绑定对象一致，最安全 ✅ |
| **② 用 `completion_done()` 等它走完** | 正是 §7 那个"拿锁即证明"的技巧 |
| **③ 超时后取消异步工作并等待** | 比如 `cancel_work_sync()` 之后再返回 |

```c
/* ✅ 推荐：completion 跟着设备走 */
struct my_dev {
	struct completion init_done;      /* 不是栈上 */
	...
};

static int my_drv_probe(struct platform_device *pdev)
{
	struct my_dev *dev = devm_kzalloc(...);
	init_completion(&dev->init_done);
	...
	/* 即使超时返回，dev 还在，complete(&dev->init_done) 也安全 */
}
```

### 用户态对照

| 内核 | 用户态 |
|------|--------|
| `struct completion` | `std::promise<T>` / `std::future<T>` |
| `complete_all()` | `std::latch::count_down()`（C++20） |
| `wait_for_completion()` | `std::future::get()` / `std::latch::wait()` |
| `try_wait_for_completion()` | `std::future::wait_for(0s)` |

热路径的替代：**`std::atomic<uint64_t>` 序号 + 忙等**，
或者 SPSC 无锁环形队列（生产者写完推进 `tail`，消费者自旋看 `tail`）。

---

## 实践模板

```c
#include <linux/completion.h>
#include <linux/errno.h>

/* ---------- 模板一：驱动 probe 等异步初始化（✅ 正确版） ---------- */

struct my_dev {
	struct completion init_done;     /* ✅ 放结构体里，不放栈上 */
	struct work_struct init_work;
	void __iomem *base;
};

static void init_worker(struct work_struct *work)
{
	struct my_dev *dev = container_of(work, struct my_dev, init_work);

	/* ... 做耗时的硬件初始化（可以睡眠）... */

	complete(&dev->init_done);       /* 通知等待方 */
}

static int my_drv_probe(struct platform_device *pdev)
{
	struct my_dev *dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	init_completion(&dev->init_done);
	INIT_WORK(&dev->init_work, init_worker);

	schedule_work(&dev->init_work);

	/* ✅ 带超时，且 interruptible 以便被信号打断 */
	if (wait_for_completion_interruptible_timeout(&dev->init_done, 5 * HZ) <= 0) {
		cancel_work_sync(&dev->init_work);   /* ✅ 超时后要取消异步工作 */
		dev_err(&pdev->dev, "init timeout\n");
		return -ETIMEDOUT;
	}

	dev_info(&pdev->dev, "init done\n");
	return 0;
}


/* ---------- 模板二：ISR 里 complete（完成方可以在中断上下文） ---------- */

static irqreturn_t my_hw_isr(int irq, void *data)
{
	struct my_dev *dev = data;

	/* ... 清中断 ... */

	complete(&dev->init_done);       /* ✅ complete() 可以在中断上下文调 */
	return IRQ_HANDLED;
}


/* ---------- 模板三：模块卸载等引用归零 ---------- */

static DECLARE_COMPLETION(module_gone);      /* 静态/全局，生命周期安全 */

static void last_user_gone(struct kref *kref)
{
	complete(&module_gone);
}

static void __exit my_mod_exit(void)
{
	/* 触发最后一个引用释放 */
	kref_put(&g_kref, last_user_gone);

	/* ✅ 等它真的走完，带超时 */
	if (!wait_for_completion_timeout(&module_gone, 10 * HZ))
		pr_warn("timeout waiting for last user\n");

	/* ... 其他清理 ... */
}


/* ---------- 模板四：不睡地试探（可在中断上下文） ---------- */

static irqreturn_t poll_isr(int irq, void *data)
{
	struct my_dev *dev = data;

	/* ✅ try_wait_for_completion 不睡，可以在 ISR 里调 */
	if (try_wait_for_completion(&dev->init_done)) {
		/* 已经完成，消费掉一个令牌 */
		handle_ready(dev);
	}
	return IRQ_HANDLED;
}
```

---

## 易错点核对表

| # | 易错点 | 正确做法 |
|---|--------|---------|
| 1 | `wait_for_completion_timeout()` 返回值赋给 `long` 后判 `< 0` | ❌ 它是 `unsigned long`，永不为负。判 `== 0` 表示超时 |
| 2 | 以为成功时返回 1 | ❌ 返回的是**剩余 jiffies**（可能很大）。只能判 `> 0` |
| 3 | ISR 里调 `wait_for_completion()` | ❌ 会睡。ISR 里只能用 `complete()` / `try_wait_for_completion()` / `completion_done()` |
| 4 | 用 `wait_for_completion()`（无超时） | ⚠️ 卡住就是 D 状态进程，`kill -9` 无效。驱动里用 `_timeout` / `_killable` 变体 |
| 5 | 把 `struct completion` 放栈上交给异步代码 | ❌ 栈帧销毁后 `complete()` 会写到别人的栈。放结构体或全局 |
| 6 | `complete_all()` 后直接重用 | ❌ `done = UINT_MAX` 是永久标记，必须 `reinit_completion()` |
| 7 | `reinit_completion()` 前不等等待者走完 | ❌ 等待者会误判成超时（§8 的竞态） |
| 8 | 用 `completion_done()` 判断"还有没有等待者" | ❌ 它只表示"完成了"；`complete_all()` 后恒返回 true |
| 9 | 以为 `done` 是布尔量 | ❌ 是**计数器**，`complete()` 可以在 `wait` 之前调 |
| 10 | 手搓"waitqueue + bool"代替 completion | ⚠️ 要自己保证"先查后睡"顺序，丢了唤醒就是 lost wakeup |
| 11 | 用 semaphore 当完成通知 | ⚠️ 语义误导（读代码的人会去找"它保护什么资源"），且引入优先级反转风险 |
| 12 | 忘了 `complete()` 是 FIFO 一对一 | 唤醒 N 个等待者要调 N 次 `complete()`，或用 `complete_all()` |

---

## 常见陷阱

1. 混淆 completion 和 semaphore——completion 是一次性通知，semaphore 可重复
2. 在 completion 的 `wait_for_completion()` 中以为会自旋——它会睡眠（进程上下文）
3. 多次 `complete()` 一个 completion——`complete()` 通常只调一次，`complete_all()` 标记永久完成
4. **（v6.6 补充）** `wait_for_completion_timeout()` 返回值当布尔用还判负数
5. **（v6.6 补充）** `complete_all()` 后不 `reinit_completion()` 就重用
6. **（v6.6 补充）** 把 `struct completion` 放栈上，超时返回后栈失效而异步侧还在 `complete()`
7. **（v6.6 补充）** 用 `completion_done()` 判断"是否还有等待者"（`complete_all()` 后恒 true）
8. **（v6.6 补充）** 照着 v5.6 之前的代码用 `wait_queue_head_t` 直接操作 `completion.wait`

---

## 自测题

<details>
<summary>自测题（点击展开）</summary>

**Q1.** completion 的典型使用场景？

<details><summary>答案</summary>

驱动初始化等硬件就绪：`init_completion(&done)` → 启动硬件 → `wait_for_completion_timeout(&done, timeout)` → 中断处理函数中 `complete(&done)`。线程池任务完成通知：主线程 `wait_for_completion()` 等所有 worker `complete()`。模块卸载等引用归零。vs semaphore：completion 语义更清晰（一次性事件），semaphore 适合计数资源。

<details><summary>按 v6.6 修订/补充</summary>

**"vs semaphore"这条要补充官方的定性。** `kernel/sched/completion.c` 开头
那段注释是设计说明书，最后一句最关键：

```c
/*
 * But more importantly, the primitive documents the usage. Semaphores would
 * typically be used for exclusion which gives rise to priority inversion.
 * Waiting for completion is a typically sync point, but not an exclusion point.
 */
```

两点值得展开：

**① "the primitive documents the usage"（原语本身就是文档）。**
看到 `struct completion` 就知道这是**同步点**；看到 `struct semaphore`
会去找"它在保护什么资源"。用 semaphore 当完成通知，读代码的人会误判。

**② 为什么提到优先级反转？**
用一个"计数=1 的 semaphore"做完成通知，它就变成了事实上的互斥锁 ——
而互斥锁会引发优先级反转。completion 的等待者**不持有任何东西**：
`wait_for_completion()` 醒来后没有"要释放"的义务，**不存在"持锁者"**。

**另外补一个使用上的坑（§8/HFT 关联章节）**：
第 ① 条模板里 `wait_for_completion_timeout()` 是对的，
但如果图省事写成 **`wait_for_completion()`**（无超时）就会踩
和 10.4 §4 `down()` **完全相同的坑**：睡在 `TASK_UNINTERRUPTIBLE` +
`MAX_SCHEDULE_TIMEOUT` → 卡住就是 `D` 状态进程，`kill -9` 无效。
**驱动里永远用带超时或可中断的变体。**

**还有一个很容易犯的错：把 completion 放栈上**（§HFT 关联）：
```c
int my_drv_probe(...) {
	DECLARE_COMPLETION_ONSTACK(done);      /* ⚠️ 栈上 */
	start_async_init(dev, &done);
	wait_for_completion_timeout(&done, HZ);
	return 0;                              /* 栈帧销毁，而异步侧可能还在 complete() */
}
```
多数时候没事（异步很快），偶尔超时返回时异步侧还没跑完 →
后来的 `complete()` 写到别人的栈帧 → **随机崩溃**。
**completion 应该放在设备结构体里**，而不是栈上。

</details>
</details>

**Q2.** `complete()` 和 `complete_all()` 的区别？

<details><summary>答案</summary>

`complete()`：唤醒一个等待者，completion 的 done+1。如果有多个等待者，需要多次 `complete()`。`complete_all()`：唤醒所有等待者，并将 completion 标记为永久完成（后续 `wait_for_completion()` 立即返回）。`complete_all()` 后不能重用该 completion（除非 reinit）。典型：驱动 probe 成功后 `complete_all()`，所有等待者放行。

<details><summary>按 v6.6 修订/补充</summary>

**"done+1"这句要精确化** —— 源码有个 `UINT_MAX` 的保护：

```c
static void complete_with_flags(struct completion *x, int wake_flags)
{
	unsigned long flags;
	raw_spin_lock_irqsave(&x->wait.lock, flags);

	if (x->done != UINT_MAX)          /* ← 保护：已经是永久完成就不再加 */
		x->done++;
	swake_up_locked(&x->wait, wake_flags);
	raw_spin_unlock_irqrestore(&x->wait.lock, flags);
}

void complete_all(struct completion *x)
{
	unsigned long flags;
	lockdep_assert_RT_in_threaded_ctx();

	raw_spin_lock_irqsave(&x->wait.lock, flags);
	x->done = UINT_MAX;               /* ← 不是 done++，设成哨兵值 */
	swake_up_all_locked(&x->wait);
	raw_spin_unlock_irqrestore(&x->wait.lock, flags);
}
```

**`UINT_MAX` 是"永久完成"哨兵值，代码里三处为它特判**：

| 位置 | 代码 | 作用 |
|------|------|------|
| `complete_with_flags()` | `if (x->done != UINT_MAX) x->done++;` | **防溢出**（否则绕回 0） |
| `do_wait_for_common()` | `if (x->done != UINT_MAX) x->done--;` | **永不被消费**，所有等待者都能通过 |
| `try_wait_for_completion()` | `else if (x->done != UINT_MAX) x->done--;` | 同上 |

**为什么用 `UINT_MAX` 而不是单独一个 bool？**
因为这样不需要额外字段，`struct completion` 能保持**两个字段、24 字节**。
代价是两种语义挤在一个字段里，读代码时要记住这个约定。

**另外补两条原文没说的**：

① **`complete()` 是 FIFO 一对一的**：`swake_up_locked()` 用
`list_first_entry` 取队首，入队侧是 `list_add_tail` —— 标准的 FIFO。
唤醒 N 个等待者要么调 N 次 `complete()`，要么用 `complete_all()`。

② ⭐ **`reinit_completion()` 有竞态，不是"调一下就行"**（§8）：
```c
static inline void reinit_completion(struct completion *x)
{
	x->done = 0;          /* 就一行 */
}
```
`complete_all` 的 kernel-doc 明确警告：
> *"The code must make sure that **all waiters have woken and finished**
> before reinitializing @x."*

因为等待者被唤醒后还没执行到 `__finish_swait()` 出队，
此时 `done` 被清成 0，他们醒来后会**误判成"超时"**。

实践中：**能不用 `complete_all()` 就不用** ——
它引入的"永久状态 + 需要 reinit + reinit 有竞态"这一串麻烦，
往往比它解决的问题更麻烦。

</details>
</details>

**Q3.** HFT 中 completion 的用户态对应物？

<details><summary>答案</summary>

① `std::promise<T>` + `std::future<T>`：一次性设置值 + 等待。② `std::condition_variable`：更灵活，可重复使用。③ `std::latch`（C++20）：一次性，多线程等同一事件。④ `std::barrier`（C++20）：多线程同步点。HFT 热路径不用这些（有 syscall 开销），用无锁标志位（`std::atomic<bool>` + spin）。

<details><summary>按 v6.6 修订/补充</summary>

这四条都对，补**可量化的延迟画像**和**一条内核侧的映射修正**。

**内核 completion 的延迟画像（和 semaphore 同量级）**：
`wait_for_completion()` 的睡眠路径和 `down()` 是同一套
（`schedule_timeout` → 睡 → 醒 → 再查），
所以是完整的**两次上下文切换 + 调度器往返**，量级 **数 µs 且方差大**。
这与 HFT 的 P99 尾延迟预算（通常 < 10 µs）同一量级。

| 场景 | 用 |
|------|-----|
| 热路径（订单、行情处理） | ❌ 用 `atomic_t` 序号 + 重试，或 SPSC 无锁环 |
| **初始化 / probe / 配置加载** | ✅ completion 是**首选**（语义最清楚） |
| 模块卸载等引用归零 | ✅ |
| 等待硬件响应（进程上下文） | ✅ 但**必须带超时** |

**一条映射修正**：`complete_all()` 对应的其实**不是**
`std::condition_variable::notify_all()`，而是 **`std::latch::count_down()`**。
区别在"是否可重用"：
- `std::condition_variable` 可以反复 notify（对应多次 `complete()`）；
- `std::latch` 是一次性的、count 减到 0 后永久放行（对应 `complete_all()`
  的 `done = UINT_MAX` 永不被消费）。

把 `complete_all()` 想成 `notify_all()` 会导致误以为"可以反复用" ——
这正是 §8 那个 reinit 竞态的根源。

**最后一条实践提醒**：用户态 `std::future::get()` 只能阻塞等待，
没有超时版本；要超时得用 `std::future::wait_for(duration)`。
这和内核的 `wait_for_completion()`（无超时）vs
`wait_for_completion_timeout()`（有超时）是同一个取舍 ——
**默认 API 往往是"无上界"的那个，写生产代码时要主动选带超时的变体。**

</details>
</details>

**Q4.** 为什么 `complete()` 可以在 `wait_for_completion()` 之前调用而不丢事件？

<details><summary>答案</summary>

因为 `done` 是**计数器**（`unsigned int`），不是布尔标志。

```c
	if (x->done != UINT_MAX)
		x->done++;                    /* 计数 +1 */
	swake_up_locked(&x->wait, wake_flags);
```

等待侧第一步就查这个计数：

```c
	if (!x->done) {                     /* ① 已有令牌 → 整个等待块跳过 */
		DECLARE_SWAITQUEUE(wait);
		do {
			...
		} while (!x->done && timeout);
		...
	}
	if (x->done != UINT_MAX)
		x->done--;                      /* ④ 消费一个 */
	return timeout ?: 1;
```

时间线（工作线程跑得特别快的情况）：
```
  主线程：init_completion(&done)           → done = 0
  主线程：kthread_run(worker)
           worker 立刻跑完 → complete()   → done = 1
  主线程：wait_for_completion(&done)
          ① 看到 done != 0 → 跳过等待
          ④ done-- → done = 0
          ✅ 立即返回，事件没丢
```

**这解决的是经典的 lost wakeup 问题。** 手搓"waitqueue + bool"时：

```c
	prepare_to_wait(&wq, &wait, TASK_UNINTERRUPTIBLE);
	if (!hw_ready)          /* ← 检查晚了 */
		schedule();
```
在 `prepare_to_wait()` 和 `if (!hw_ready)` 之间如果发生了
`hw_ready = true; wake_up(&wq);`，这次唤醒就**丢了** ——
因为那时你还没进等待队列。

**completion 怎么保证不会**：②入队 和 ③睡 这两个动作**都在
`x->wait.lock` 保护下**完成（`__wait_for_common()` 调用前就拿好锁了）。
ISR 的 `complete()` 必须拿到同一把锁才能 `done++`，所以它要么
- 发生在入队之前 → 那 `done` 已经 > 0，你在 ① 就返回了；
- 发生在睡眠之后 → 那 `swake_up_locked()` 会真的唤醒你。

**不存在"已入队但还没睡"这个中间态被 ISR 观察到的可能。**

（顺带一条内核等待队列的通用纪律，见 §4：
**`__set_current_state(state)` 必须在放锁之前**。顺序反了，
`complete()` 的唤醒会打在一个 `TASK_RUNNING` 的任务上而失效，然后你睡死。）

</details>

**Q5.** `completion_done()` 里为什么"拿了锁立刻放、什么都不做"？

<details><summary>答案</summary>

```c
bool completion_done(struct completion *x)
{
	unsigned long flags;

	if (!READ_ONCE(x->done))
		return false;

	/*
	 * If ->done, we need to wait for complete() to release ->wait.lock
	 * otherwise we can end up freeing the completion before complete()
	 * is done referencing it.
	 */
	raw_spin_lock_irqsave(&x->wait.lock, flags);
	raw_spin_unlock_irqrestore(&x->wait.lock, flags);
	return true;
}
```

**这两行不是冗余，是防止 use-after-free。**

推演那个竞态（completion 在**栈上**时最典型）：

```
等待方                                     完成方（另一 CPU 或 ISR）
─────────────────────────────────         ────────────────────────────
wait_for_completion(&done) 返回
（被唤醒，但 complete() 可能还没走完）
函数返回 → 栈帧销毁
                                          complete(): done++
                                                      swake_up_locked()
                                                      ⚠️ 还在读写 &x->wait.lock
                                                         → 写到别人的栈帧里！
```

**解法**：既然 `complete()` **全程持有 `wait.lock`**，那么
"我成功地拿到又放掉这把锁"这件事本身就证明了
**"此刻没有 `complete()` 正在执行"**。

**这是一个很漂亮的技巧**：用"抢到锁"作为"对方已完成"的证明，
而不是引入额外的引用计数或 RCU。

⚠️ **但要注意这个函数名字有误导性** —— 它不是"查询有没有等待者"，
而是"**这个 completion 是否已完成，且可以安全销毁**"。
`complete_all` 的 kernel-doc 还专门补了一刀：

> *"Also note that the function `completion_done()` **can not be used
> to know if there are still waiters** after `complete_all()` has been called."*

即 `complete_all()` 之后 `done = UINT_MAX` 永不为 0，
`completion_done()` **恒返回 true** —— 它无法告诉你"等待者们都醒了吗"。
要安全地 `reinit_completion()` 得用别的方法（§8）。

</details>

**Q6.** `reinit_completion()` 有什么竞态？为什么"能不用 `complete_all()` 就不用"？

<details><summary>答案</summary>

```c
static inline void reinit_completion(struct completion *x)
{
	x->done = 0;        /* 就一行 */
}
```

**为什么需要它**：`complete_all()` 把 `done` 设成 `UINT_MAX`，
而 `UINT_MAX` **永不被消费**（`if (x->done != UINT_MAX) x->done--`），
所以 `complete_all()` 之后这个 completion 就"永久完成"了，
不 reinit 就没法再用。

**竞态在哪**（`complete_all` 的 kernel-doc 明确警告）：

> *"The code must make sure that **all waiters have woken and finished**
> before reinitializing @x."*

```
wait_list = [A, B, C]

complete_all(&x)  → done = UINT_MAX，唤醒 A、B、C
                     （都进 runqueue 了，但都还没跑到）

⚠️ 此刻 A/B/C 的 do_wait_for_common() 还没执行到 __finish_swait() 出队，
   他们的 task_list 节点还挂在 x->wait.task_list 上

reinit_completion(&x) → done = 0

A 醒来 → __finish_swait（出队）
       → if (!x->done) return timeout     ← ❌ A 误判成"超时/失败"！
```

**正确做法**（三选一）：

| 方法 | 说明 |
|------|------|
| 引用计数 + 另一个 completion | 等待者走完后 `complete(&second_done)`，主方等 N 次 |
| `kthread_stop()` 等自带机制 | 内核 kthread 停止路径内部已处理这种同步 |
| **避免 `complete_all()`** | 只需唤醒一个时用 `complete()`，就不需要 reinit |

**实践中建议：能不用 `complete_all()` 就不用。**
它引入的"永久状态 + 需要 reinit + reinit 有竞态"这一串麻烦，
往往比它解决的问题更麻烦。多数"唤醒所有等待者"的需求，
用 `complete()` 循环调用、或者改用一个 `atomic_t` 标志 + `wake_up_all()`
反而更清楚。

</details>

**Q7.** v6.6 的 completion 用的是 `swait` 还是 `wait_queue`？什么时候换的，为什么？

<details><summary>答案</summary>

**用的是 `swait`（simple wait queue），转换发生在 v5.7。**

```c
/* v6.6 include/linux/completion.h */
struct completion {
	unsigned int done;
	struct swait_queue_head wait;      /* ← swait，不是 wait_queue_head_t */
};

/* include/linux/swait.h */
struct swait_queue_head {
	raw_spinlock_t		lock;
	struct list_head	task_list;
};
```

**版本断崖实测**（抓多版本 `include/linux/completion.h` 统计关键词）：

| 版本 | `swait` 次数 | `wait_queue_head` 次数 |
|------|-------------|----------------------|
| v4.9 | 0 | 1 |
| v5.5 | 0 | 1 |
| v5.6 | **0** | **1** |
| **v5.7** | **3** | **2** ← 断点 |
| v6.0 | 3 | 2 |
| **v6.6** | **3** | **2** |

**为什么换成 swait** —— 对比两者：

| | `wait_queue_head_t` | `swait_queue_head` |
|--|---------------------|---------------------|
| 锁 | `spinlock_t`（RT 上会睡眠） | **`raw_spinlock_t`** |
| 节点 | 有 `func` 回调 + `flags` | **只有 `task` + `task_list`** |
| exclusive 唤醒 | 支持 `WQ_FLAG_EXCLUSIVE` | **不支持**（一律 FIFO 逐个） |
| autoremove | 支持 | **不支持**（手动 `__finish_swait`） |

**completion 用不上 `func` 回调、用不上 exclusive、用不上 autoremove** ——
它只需要"按 FIFO 唤醒任务"这一件事。所以用 swait 更省：
每个节点省掉一个函数指针 + flags，且不引入 `spinlock_t` 在 RT 上的复杂性。

**一个旁证**（`kernel/sched/swait.c` 的注释）：
```c
/*
 * Wake up all waiters. This is an interface which is solely exposed for
 * completions and not for general usage.
 *
 * It is intentionally different from swake_up_all() to allow usage from
 * hard interrupt context and interrupt disabled regions.
 */
void swake_up_all_locked(struct swait_queue_head *q)
```
`swake_up_all_locked()` 是**只为 completion 开的口子**（"not for general usage"），
`_locked` 后缀是因为调用方已经拿着锁，这样就能在**硬中断上下文 / 关中断区域**使用。

**实践意义**：照着 v5.6 之前的代码直接操作 `completion.wait` 的成员
（比如当它是 `wait_queue_head_t` 去调 `wake_up_all`）会编译不过或语义错。

</details>

**Q8.** `wait_for_completion_timeout()` 的返回值该怎么判断？

<details><summary>答案</summary>

**它是 `unsigned long`，超时返回 `0`，成功返回"剩余 jiffies"（≥1）。**

```c
unsigned long __sched
wait_for_completion_timeout(struct completion *x, unsigned long timeout)
{
	return wait_for_common(x, timeout, TASK_UNINTERRUPTIBLE);
}
```

底层那句决定了返回值语义：

```c
	return timeout ?: 1;      /* GCC 语法：timeout ? timeout : 1 */
```

意思是：**还剩时间就返回剩余 jiffies，已经归零就返回 1**。

| 返回类型 | API | 成功 | 超时 | 被打断 |
|---------|-----|------|------|--------|
| `unsigned long` | `wait_for_completion_timeout()` | **剩余 jiffies（≥1）** | **`0`** | — |
| `long` | `wait_for_completion_interruptible_timeout()` | 剩余 jiffies（≥1） | `0` | **`-ERESTARTSYS`（负数）** |

**三个易错点**：

**① 别对 `unsigned long` 版本判负数。**
```c
/* ❌ unsigned 永不为负，这个分支永远不进 */
unsigned long ret = wait_for_completion_timeout(&x, HZ);
if (ret < 0)  { ... }

/* ✅ */
if (ret == 0) { /* 超时 */ }
else          { /* 完成，ret 是剩余 jiffies */ }
```

**② interruptible_timeout 版本要分三态：**
```c
long ret = wait_for_completion_interruptible_timeout(&x, HZ);
if (ret > 0)       { /* 完成 */ }
else if (ret == 0) { /* 超时 */ }
else               { /* 被打断，ret == -ERESTARTSYS */ }
```

**③ 成功时返回的不是 1，是剩余 jiffies。**
`return timeout ?: 1` 意味着成功时返回的是**剩余时间**（可能很大），
只有剩余时间刚好归零时才返回 1。所以判断只能写 `> 0`，**不能写 `== 1`**。

**顺带提醒**：`wait_for_completion()`（无超时版本）睡在
`TASK_UNINTERRUPTIBLE` + `MAX_SCHEDULE_TIMEOUT` 上 ——
**信号叫不醒、没有上界**。卡住就是 `D` 状态进程，`kill -9` 无效
（和 10.4 §4 的 `down()` 完全同一个问题）。
驱动里应该用 `_timeout` 或 `_killable` 变体。

</details>

**Q9.** 官方为什么说"等待完成是同步点，不是互斥点"？这有什么实际影响？

<details><summary>答案</summary>

出自 `kernel/sched/completion.c` 开头的设计说明注释，最后一句最关键：

```c
/*
 * But more importantly, the primitive documents the usage. Semaphores would
 * typically be used for exclusion which gives rise to priority inversion.
 * Waiting for completion is a typically sync point, but not an exclusion point.
 */
```

**"the primitive documents the usage"（原语本身就是文档）** ——
这是最重要的实践意义：

- 看到 `struct completion` → 立刻知道这是**同步点**；
- 看到 `struct semaphore` → 会去找"**它在保护什么资源**"。

用 semaphore 当完成通知，读代码的人会误判，进而可能做出错误修改
（比如以为要对称释放，或者以为可以重复获取）。

**为什么提到优先级反转？**

用一个"计数=1 的 semaphore"做完成通知，它就变成了**事实上的互斥锁** ——
而互斥锁会引发优先级反转：高优先级任务等低优先级任务持有的锁，
而低优先级任务可能被中优先级任务抢占，导致高优先级任务无限期等待。

**completion 的等待者不持有任何东西**：
`wait_for_completion()` 醒来后没有"要释放"的义务 ——
等待方**不会**调 `up()` / `unlock()`。**不存在"持锁者"这个概念**，
所以也就没有优先级反转的载体。

**三者定位总表**：

| | semaphore | mutex | **completion** |
|--|-----------|-------|----------------|
| 语义 | 资源计数 / 互斥 | **互斥**（有 owner） | **同步点**（一次性事件） |
| 初值 | N（可 >1） | 1 | **0**（"还没完成"） |
| 默认行为 | 有令牌就走 | 有就拿 | **默认阻塞** |
| 需要"释放"吗 | ✅ `up()` | ✅ `unlock()`（**同一任务**） | ❌ 等待方无需释放 |
| 优先级反转 | 会（当互斥用时） | 会（非 RT）/ 不会（RT） | **不会**（无持有者） |
| 一对多唤醒 | 循环 `up()` | 不支持 | ✅ `complete_all()` |
| 中断上下文 | ✅ `up()` | ❌ | ✅ `complete()` |

**实际影响一句话**：
"等某件事做完" → 用 completion，别用 semaphore（语义清楚 + 无优先级反转）；
"保护一段临界区" → 用 mutex，别用 completion（completion 没有 owner，
任何人都能 `complete()`，做不了互斥）。

</details>

</details>

---

→ [4.4 休眠唤醒](../../chapter-04-process-scheduling/notes/section-4.4-休眠与唤醒.md) · [10.5 mutex](./section-10.5-互斥体.md)

---
