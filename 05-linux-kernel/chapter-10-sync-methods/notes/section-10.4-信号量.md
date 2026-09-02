## ④ 信号量 · Semaphores

**睡眠锁**：拿不到时任务 **睡眠等待**，不空转 CPU。可实现 **互斥（计数=1）** 或 **资源池（计数>1）**。

| 属性 | 说明 |
|------|------|
| 争用 | **睡眠**（可被调度走） |
| 上下文 | **仅进程上下文** — ISR/softirq **禁止** `down` 睡眠 |
| 持有时间 | 可较长（相对 spinlock） |

#### 操作直觉

| 操作 | 含义 |
|------|------|
| **`down` / `down_interruptible`** | P 操作 — 计数减；不够则睡 |
| **`up`** | V 操作 — 计数加；唤醒等待者 |
| **`down_trylock`** | 不睡，失败立即返回 |

```
计数初始 = 3（三个缓冲槽）
任务 A down → 2
任务 B down → 1
任务 C down → 0
任务 D down → 睡眠……
A 用完 up → 1，唤醒 D     ← ⚠️ 见 §8：真实实现里这里 count **不加**
```

#### 与 mutex 的关系

| | semaphore | mutex（10.5） |
|--|-----------|----------------|
| 计数 | 可 >1 | **二值**，专为互斥 |
| 历史 | 更老、更通用 | **新互斥代码首选** |
| 所有者 | 语义较弱 | 有明确所有者、严格规则 |

---

> **本篇分工**：上面的速查表**原样保留**，它是"30 秒回忆"用的。本篇往下**不复述**这些语义，
> 只做六件事，全部用 v6.6 源码实证：
>
> ① 拆开 `struct semaphore`（**24 字节、三个字段**），并回答一个书上不会讲的问题：
> **为什么它内部那把锁是 `raw_spinlock_t` 而不是 `spinlock_t`**（答案在 §1，可推广成一条规律）；
> ② ⭐ **订正一：`down()` 在 v6.6 已被 kernel-doc 显式标记废弃**，
> 理由是它睡在 `TASK_UNINTERRUPTIBLE` 上 → `D` 状态进程 → `kill -9` 都杀不掉；
> ③ ⭐ **订正二：`down_trylock()` 的返回值是反的**（0 = 成功），源码里有专门的警告注释；
> ④ ⭐ **订正三：`DEFINE_SEMAPHORE` 从 v6.4 起要两个参数**（附 v4.19~v6.6 的实测版本断崖）；
> ⑤ ⭐ **反直觉：`up()` 在有等待者时根本不 `count++`**，而是直接"传令牌"给队首 —— 为了保 FIFO；
> ⑥ 用 `Documentation/locking/mutex-design.rst` 的官方原话回答"为什么新代码一律用 mutex"，
> 并给出**真正的理由不是性能、而是可调试性**（mutex 有 9 条强制语义 + 5 项 debug 能力，
> semaphore 一条都没有）。额外指出：**mutex 有乐观自旋、semaphore 没有**，
> 所以信号量的获取延迟是**调度器级**的。
>
> 所有常量与代码均核对自缓存的 v6.6 源码，行号可查。

---

## 1. `struct semaphore` 在 v6.6 里只有 **24 字节、三个字段**

`include/linux/semaphore.h` 全文才 51 行，结构体本体就更短了：

```c
/* include/linux/semaphore.h —— v6.6 原文 */
/* Please don't access any members of this structure directly */
struct semaphore {
	raw_spinlock_t		lock;
	unsigned int		count;
	struct list_head	wait_list;
};
```

逐个字段：

| 字段 | 大小（x86-64） | 作用 |
|------|---------------|------|
| `raw_spinlock_t lock` | 4 B | 保护另外两个字段的**内部锁**（不是给用户用的） |
| `unsigned int count` | 4 B | 还能被多少任务获取。为 0 时**wait_list 上可能有人** |
| `struct list_head wait_list` | 16 B | 等待者 FIFO 队列，串的是 `struct semaphore_waiter` |

合计 4 + 4 + 16 = **24 字节**。

### 官方给的大小对照（含 mutex / rwsem）

`Documentation/locking/mutex-design.rst` 的 "Disadvantages" 一节原话：

> Unlike its original design and purpose, 'struct mutex' is among the largest
> locks in the kernel. E.g: on x86-64 it is 32 bytes, where 'struct semaphore'
> is 24 bytes and rw_semaphore is 40 bytes. Larger structure sizes mean more CPU
> cache and memory footprint.

| 睡眠锁 | x86-64 大小 | 说明 |
|--------|------------|------|
| `struct semaphore` | **24 B** | 最小 |
| `struct mutex` | **32 B** | 多了 `osq`（乐观自旋队列）+ debug 字段 |
| `struct rw_semaphore` | **40 B** | 最大 |

有意思的是官方文档把 mutex 更大这件事写进了 **"Disadvantages"（缺点）** 一节 —— 也就是说，
社区承认 mutex 的代价是**更大的 cache/内存占位**，但仍然推荐它。这本身就是个很强的信号：
**推荐 mutex 的理由不在省内存，而在别处**（见 §9）。

---

### ⭐ 为什么内部锁是 `raw_spinlock_t` 而不是 `spinlock_t`

这是本篇最值得记住的一条。书上不会讲，但一旦想通了可以推广成规律。

回忆 10.2 的结论：在 `CONFIG_PREEMPT_RT` 下，`spinlock_t` 会被替换成 `rt_mutex_base`，
**它会睡眠**；而 `raw_spinlock_t` 在任何配置下都是真自旋。

现在看信号量的慢路径在干什么（`kernel/locking/semaphore.c`）：

```c
static inline int __sched ___down_common(struct semaphore *sem, long state,
								long timeout)
{
	struct semaphore_waiter waiter;

	list_add_tail(&waiter.list, &sem->wait_list);
	waiter.task = current;
	waiter.up = false;

	for (;;) {
		if (signal_pending_state(state, current))
			goto interrupted;
		if (unlikely(timeout <= 0))
			goto timed_out;
		__set_current_state(state);
		raw_spin_unlock_irq(&sem->lock);     /* ① 放锁 */
		timeout = schedule_timeout(timeout); /* ② 睡眠 */
		raw_spin_lock_irq(&sem->lock);       /* ③ 醒来后重新拿锁 */
		if (waiter.up)
			return 0;
	}
	/* ... */
}
```

注意 ①②③ 这三步 —— 这是一个**手工编排的"放锁 → 睡 → 拿锁"序列**：

- 它在**持有锁的状态下**调用 `schedule_timeout()` 之前，先自己把锁放了；
- 睡醒之后，它又自己把锁拿回来；
- 整个过程**没有任何内核通用的锁框架在帮它兜底**，全是手写。

**如果 `sem->lock` 是一把会睡眠的锁（`spinlock_t` on RT），这段代码的正确性就没法保证了**：
`raw_spin_lock_irq()` 在拿不到锁时是自旋等待，而自旋等待在"刚被唤醒、马上要重新检查
`waiter.up`"这条路径上是必需的 —— 因为它同时还要关中断，而 RT 的睡眠锁不允许在关中断
的临界区里睡眠。

所以结论很简单：

> **所有"自己实现睡眠逻辑"的内核原语，内部锁一律用 `raw_spinlock_t`。**

这条规律可以直接推广，去看一眼就明白：

| 原语 | 内部锁类型 | 是否自己编排睡眠 |
|------|-----------|----------------|
| `struct semaphore` | `raw_spinlock_t lock` | ✅ 是（`___down_common`） |
| `struct mutex` | `raw_spinlock_t wait_lock` | ✅ 是（`__mutex_lock_common`） |
| `struct rw_semaphore` | `raw_spinlock_t wait_lock` | ✅ 是（`rwsem_down_write_slowpath`） |
| `struct completion` | `raw_spinlock_t wait.lock`（swait） | ✅ 是（`wait_for_completion*`） |

（completion 的字段在 v6.6 里是 `struct swait_queue_head wait`，其内部是 `raw_spinlock_t lock`，
见 10.6。）

**反过来**：`spinlock_t` / `rwlock_t` 这种**不睡眠**的原语，本身就用不着这层考虑 ——
它们要么直接是 `raw_spinlock_t`（非 RT），要么在 RT 上被整体替换成可睡眠的 `rt_mutex_base`。

---

## 2. 为什么内部锁用 **irqsave** 变体（源码注释逐字）

`down()` 一上来就是 `raw_spin_lock_irqsave()`，而不是 `raw_spin_lock()`。
`kernel/locking/semaphore.c` 开头有一段注释专门解释这件事，值得逐字读完：

```c
/*
 * Some notes on the implementation:
 *
 * The spinlock controls access to the other members of the semaphore.
 * down_trylock() and up() can be called from interrupt context, so we
 * have to disable interrupts when taking the lock.  It turns out various
 * parts of the kernel expect to be able to use down() on a semaphore in
 * interrupt context when they know it will succeed, so we have to use
 * irqsave variants for down(), down_interruptible() and down_killable()
 * too.
 *
 * The ->count variable represents how many more tasks can acquire this
 * semaphore.  If it's zero, there may be tasks waiting on the wait_list.
 */
```

拆成三层：

| 层 | 内容 | 推论 |
|----|------|------|
| 第一层（正常） | `down_trylock()` 和 `up()` **可以**在中断上下文调 | 所以必须关中断再拿内部锁，否则 ISR 打断后会死锁 |
| 第二层（奇怪） | "内核里有代码指望**在知道自己会成功时**能在中断里调 `down()`" | 所以 `down()` 系列**也**得用 irqsave |
| 第三层（语义） | `count == 0` 不代表没人等，只代表**可能有**人在 `wait_list` 上 | 见 §8 |

⚠️ **第二层是纯历史包袱，不要模仿。** 它的意思是"有些老代码在中断里调 `down()`，
赌 `count > 0` 一定能成功" —— 这个赌注极其脆弱：一旦 `count` 恰好是 0，
ISR 里就会调用 `schedule_timeout()`，直接 `BUG: scheduling while atomic`。

**正确写法**：中断上下文里**只**用 `down_trylock()`。这一点在 `down_trylock` 的
kernel-doc 里也被再次确认：

```c
/**
 * down_trylock - try to acquire the semaphore, without waiting
 * @sem: the semaphore to be acquired
 *
 * Try to acquire the semaphore atomically.  Returns 0 if the semaphore has
 * been acquired successfully or 1 if it cannot be acquired.
 *
 * NOTE: This return value is inverted from both spin_trylock and
 * mutex_trylock!  Be careful about this when converting code.
 *
 * Unlike mutex_trylock, this function can be used from interrupt context,
 * and the semaphore can be released by any task or interrupt.
 */
```

---

## 3. 六个 API 的完整对照

`include/linux/semaphore.h` 只导出了 6 个函数。注意 `down_interruptible` /
`down_killable` / `down_trylock` / `down_timeout` 都带 `__must_check`：

```c
extern void down(struct semaphore *sem);
extern int __must_check down_interruptible(struct semaphore *sem);
extern int __must_check down_killable(struct semaphore *sem);
extern int __must_check down_trylock(struct semaphore *sem);
extern int __must_check down_timeout(struct semaphore *sem, long jiffies);
extern void up(struct semaphore *sem);
```

| API | 睡眠状态 | 成功返回 | 失败返回 | 可中断上下文 | 备注 |
|-----|---------|---------|---------|------------|------|
| `down()` | `TASK_UNINTERRUPTIBLE` | （void） | — | ❌ | ⚠️ **v6.6 已废弃**，见 §4 |
| `down_interruptible()` | `TASK_INTERRUPTIBLE` | `0` | `-EINTR` | ❌ | **最常用** |
| `down_killable()` | `TASK_KILLABLE` | `0` | `-EINTR` | ❌ | 只被**致命信号**打断 |
| `down_timeout()` | `TASK_UNINTERRUPTIBLE` | `0` | `-ETIME` | ❌ | 有超时，但仍不可被信号叫醒 |
| `down_trylock()` | **不睡** | **`0`** | **`1`** | ✅ | ⚠️ **返回值是反的**，见 §5 |
| `up()` | **不睡** | （void） | — | ✅ | 无归属，谁都能调 |

这四行是它们的全部实现差异 —— 只有 `state` 和 `timeout` 两个参数不同：

```c
static noinline void __sched __down(struct semaphore *sem)
{
	__down_common(sem, TASK_UNINTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
}

static noinline int __sched __down_interruptible(struct semaphore *sem)
{
	return __down_common(sem, TASK_INTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
}

static noinline int __sched __down_killable(struct semaphore *sem)
{
	return __down_common(sem, TASK_KILLABLE, MAX_SCHEDULE_TIMEOUT);
}

static noinline int __sched __down_timeout(struct semaphore *sem, long timeout)
{
	return __down_common(sem, TASK_UNINTERRUPTIBLE, timeout);
}
```

源码注释解释了这个写法（`___down_common` 上方）：

```c
/*
 * Because this function is inlined, the 'state' parameter will be
 * constant, and thus optimised away by the compiler.  Likewise the
 * 'timeout' parameter for the cases without timeouts.
 */
```

**这是内核里一个很典型的优化手法**：用 `static inline` + 常量参数，
让编译器为每个调用点**生成一份专属的、常量已折叠的副本**。四个 `noinline` 包装函数
各占一份代码（所以是 `noinline`，避免代码膨胀失控），但每份内部的 `state` / `timeout`
判断都被优化掉了。

三个睡眠状态的区别（`include/linux/sched.h`）：

| 状态 | 组成 | 谁能叫醒 |
|------|------|---------|
| `TASK_INTERRUPTIBLE` | — | **任意信号** + 显式唤醒 |
| `TASK_KILLABLE` | `TASK_WAKEKILL \| TASK_UNINTERRUPTIBLE` | **只有致命信号**（SIGKILL 等）+ 显式唤醒 |
| `TASK_UNINTERRUPTIBLE` | — | **只有显式唤醒**。信号来了也只是记下 pending，不唤醒 |

`TASK_KILLABLE` 是个很好的折中：既能被 `kill -9` 兜底（不会留下杀不死的 `D` 状态进程），
又不会被随便一个 `SIGCHLD`/`SIGWINCH` 之类的非致命信号打断业务逻辑。
**驱动里写"等待硬件就绪"时，`down_killable()` 通常比 `down_interruptible()` 更省心** ——
不用处理"被无关信号打断后返回 `-EINTR`"的分支。

---

## 4. ⭐ 订正一：`down()` 在 v6.6 已被**显式标记废弃**

这是本次核对里最该记住的一条。翻开 `kernel/locking/semaphore.c` 的 `down()` kernel-doc：

```c
/**
 * down - acquire the semaphore
 * @sem: the semaphore to be acquired
 *
 * Acquires the semaphore.  If no more tasks are allowed to acquire the
 * semaphore, calling this function will put the task to sleep until the
 * semaphore is released.
 *
 * Use of this function is deprecated, please use down_interruptible() or
 * down_killable() instead.
 */
void __sched down(struct semaphore *sem)
{
	unsigned long flags;

	might_sleep();
	raw_spin_lock_irqsave(&sem->lock, flags);
	if (likely(sem->count > 0))
		sem->count--;
	else
		__down(sem);
	raw_spin_unlock_irqrestore(&sem->lock, flags);
}
EXPORT_SYMBOL(down);
```

最后那句 **"Use of this function is deprecated"** 是 v6.6 官方文档里的原话。

### 为什么废弃

`__down()` 睡在 `TASK_UNINTERRUPTIBLE` 上，后果链条：

```
down() 拿不到 → __down(sem, TASK_UNINTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT)
             → 进程进入 D 状态（uninterruptible sleep）
             → 信号来了也只记 pending，不唤醒
             → kill -9 无效（SIGKILL 也只是个信号）
             → ps 里看到一个杀不死的 D 状态进程
             → 只能重启
```

注意 `MAX_SCHEDULE_TIMEOUT`（= `LONG_MAX`）意味着**没有超时**。所以严格来说：
**一个卡在 `down()` 里的进程，唯一的出路就是有人调 `up()`**。
如果那个 `up()` 永远不会来（比如硬件挂了、对端驱动有 bug），这个进程就永久卡死。

在生产环境 / 嵌入式设备上的后果尤其严重：`D` 状态进程**不响应任何信号**，
`kill -9` 打上去毫无反应，`ps aux` 里 `STAT` 列显示 `D`，连 `SIGKILL` 都救不回来。
运维只能重启设备。这就是为什么内核社区把 `down()` 标成 deprecated —— 它把
"能不能被杀"这个决定权从运维手里拿走了。

**替换规则**：

| 原写法 | 改成 | 理由 |
|--------|------|------|
| `down(&sem)` | `down_interruptible(&sem)` | 需要处理 `-EINTR`（最常见） |
| `down(&sem)` | `down_killable(&sem)` | 只让 `SIGKILL` 打断，分支更少 |
| `down(&sem)` | `down_timeout(&sem, HZ)` | 有明确上界；但仍不可被信号叫醒，小心 |

⚠️ **注意改写的代价**：`down()` 返回 `void`，改成 `down_interruptible()` 后**必须**
检查返回值。这是一个真实的语义变化，不是简单替换函数名：

```c
/* ❌ 错误：直接换名字，丢了返回值检查 —— 被打断后仍会进临界区 */
- down(&dev->sem);
+ down_interruptible(&dev->sem);          /* 编译过，逻辑错 */

/* ✅ 正确 */
if (down_interruptible(&dev->sem))
	return -ERESTARTSYS;                 /* 或 -EINTR */
```

`__must_check` 就是 GCC 的 `warn_unused_result` 属性，编译器会在你丢弃返回值时报警 ——
但**只有当你完全不接收返回值时**才报。`(void)down_interruptible(...)` 或者
"接收了但不判断"它管不着。

---

## 5. ⭐ 订正二：`down_trylock()` 的返回值是**反的**

先完整看一遍源码：

```c
int __sched down_trylock(struct semaphore *sem)
{
	unsigned long flags;
	int count;

	raw_spin_lock_irqsave(&sem->lock, flags);
	count = sem->count - 1;
	if (likely(count >= 0))
		sem->count = count;
	raw_spin_unlock_irqrestore(&sem->lock, flags);

	return (count < 0);
}
EXPORT_SYMBOL(down_trylock);
```

`return (count < 0);` —— 拿不到（count 变成负数）时返回 **1**，拿到时返回 **0**。

kernel-doc 里那句警告值得抄在显示器上：

> **NOTE: This return value is inverted from both spin_trylock and
> mutex_trylock!  Be careful about this when converting code.**

### 三家 trylock 对照

| API | 成功返回 | 失败返回 | 判据写法 |
|-----|---------|---------|---------|
| `spin_trylock()` | **1**（真） | `0` | `if (spin_trylock(&l))` |
| `mutex_trylock()` | **1**（真） | `0` | `if (mutex_trylock(&m))` |
| `down_trylock()` | **`0`** | **`1`** | `if (down_trylock(&s) == 0)` 或 `if (!down_trylock(&s))` |

也就是说：**`spin_trylock` / `mutex_trylock` 遵循 C 的"真值 = 成功"惯例，
`down_trylock` 遵循 errno 风格（0 = 成功）**。混用必然出错，而且是那种
"测试环境没争用所以一直走成功分支、上了生产偶发争用才炸"的隐蔽 bug。

⚠️ **最危险的写法**：

```c
if (down_trylock(&sem)) {      /* ← 语义完全反了！ */
	/* 作者以为：拿到了信号量，进临界区 */
	do_something();            /* 实际：这是"没拿到"的分支 */
	up(&sem);                  /* 凭空多释放一个令牌 */
}
```

这个 bug 的恐怖之处在于：`up()` 会把 `count` 凭空加 1，等于**伪造了一个令牌**。
后续会有多余的任务同时进入临界区，表现为偶发的数据损坏，极难复现。

### 一个隐蔽但关键的类型细节

```c
struct semaphore {
	raw_spinlock_t		lock;
	unsigned int		count;      /* ← unsigned */
	...
};

int __sched down_trylock(struct semaphore *sem)
{
	int count;                              /* ← int（有符号）*/
	count = sem->count - 1;                 /* unsigned - int → 提升为 unsigned? */
	...
}
```

这里 `sem->count` 是 `unsigned int`，而局部变量 `count` 声明成 **`int`**。
这个类型差异是**刻意的**：

- 如果 `count` 也声明成 `unsigned int`，那么 `sem->count == 0` 时
  `count = 0 - 1` 会绕回 `0xFFFFFFFF`，`count >= 0` **恒真** —— 逻辑彻底错误；
- 声明成 `int` 后，`sem->count - 1` 的结果（`unsigned int`）在赋给 `int` 时
  做实现定义的转换，在 x86-64（二进制补码）上得到 `-1`，`count >= 0` 为假 → 正确。

同时 `if (likely(count >= 0)) sem->count = count;` 这一句把 `int` 写回
`unsigned int` 字段也是安全的 —— 因为进这个分支时 `count >= 0`，值不会是负数。

**这类"字段是无符号、局部变量是有符号"的写法在内核里是有意的设计**，
目的就是为了能判断"减完之后是不是变负了"。看到不要"顺手改成一致的"。

---

## 6. ⭐ 订正三：`DEFINE_SEMAPHORE` 从 **v6.4** 起要两个参数

这条会让照着老代码/老书写的人直接编译不过。

### v6.6 的签名（`include/linux/semaphore.h`）

```c
#define __SEMAPHORE_INITIALIZER(name, n)				\
{									\
	.lock		= __RAW_SPIN_LOCK_UNLOCKED((name).lock),	\
	.count		= n,						\
	.wait_list	= LIST_HEAD_INIT((name).wait_list),		\
}

/*
 * Unlike mutexes, binary semaphores do not have an owner, so up() can
 * be called in a different thread from the one which called down().
 * It is also safe to call down_trylock() and up() from interrupt
 * context.
 */
#define DEFINE_SEMAPHORE(_name, _n)	\
	struct semaphore _name = __SEMAPHORE_INITIALIZER(_name, _n)

static inline void sema_init(struct semaphore *sem, int val)
{
	static struct lock_class_key __key;
	*sem = (struct semaphore) __SEMAPHORE_INITIALIZER(*sem, val);
	lockdep_init_map(&sem->lock.dep_map, "semaphore->lock", &__key, 0);
}
```

注意：

- `DEFINE_SEMAPHORE(_name, _n)` 现在是**两个参数**：名字 + 初始计数；
- `sema_init(&sem, val)` 的签名**一直没变**，还是两个参数。

### 版本断崖实测

逐个版本抓 `include/linux/semaphore.h`，对 `DEFINE_SEMAPHORE` 的签名做 diff：

| 版本 | `DEFINE_SEMAPHORE` 签名 | 参数个数 |
|------|------------------------|---------|
| v4.19 | `DEFINE_SEMAPHORE(name)` | 1 |
| v5.0 | `DEFINE_SEMAPHORE(name)` | 1 |
| v5.5 | `DEFINE_SEMAPHORE(name)` | 1 |
| v6.0 | `DEFINE_SEMAPHORE(name)` | 1 |
| v6.1 | `DEFINE_SEMAPHORE(name)` | 1 |
| v6.2 | `DEFINE_SEMAPHORE(name)` | 1 |
| v6.3 | `DEFINE_SEMAPHORE(name)` | 1 |
| **v6.4** | **`DEFINE_SEMAPHORE(_name, _n)`** | **2** ← 断点 |
| v6.5 | `DEFINE_SEMAPHORE(_name, _n)` | 2 |
| **v6.6** | **`DEFINE_SEMAPHORE(_name, _n)`** | 2 |

**断点在 v6.4。**

### 迁移写法

```c
/* ❌ v6.3 及以前 */
static DEFINE_SEMAPHORE(my_sem);        /* 隐含 count = 1 */

/* ✅ v6.4 起：必须显式给计数 */
static DEFINE_SEMAPHORE(my_sem, 1);     /* 二值信号量（互斥）*/
static DEFINE_SEMAPHORE(my_pool, 8);    /* 资源池：8 个槽 */

/* 动态初始化：签名没变，两个参数 */
struct semaphore sem;
sema_init(&sem, 1);
```

⚠️ **语义陷阱**：老写法 `DEFINE_SEMAPHORE(name)` 隐含的是 **`count = 1`**（二值）。
迁移时如果顺手写成 `DEFINE_SEMAPHORE(name, 0)`，信号量初始就是 0，
**第一个 `down()` 就会直接睡死**。

**顺带记住 `sema_init` 里的一个小细节**：

```c
static inline void sema_init(struct semaphore *sem, int val)
{
	static struct lock_class_key __key;      /* ← static！ */
	...
	lockdep_init_map(&sem->lock.dep_map, "semaphore->lock", &__key, 0);
}
```

`__key` 是 `static` 的，意味着**所有通过 `sema_init()` 初始化的信号量共用同一个
lockdep class key**。lockdep 因此会把它们当成"同一类锁"来检测死锁 —— 这是有意的
（否则每个实例一个 key，key 的数量会爆炸）。代价是 lockdep **无法区分**
"先拿 A 再拿 B" 和 "先拿 B 再拿 A" 这两个不同的 `sema_init` 出来的信号量。
需要区分时用 `mutex_init` 的 `nested` 变体或 `lockdep_set_class()`。

---

## 7. 慢路径：`___down_common` 是一个**循环**，不是单次等待

很多人（包括很多教材）把"睡眠等待"想成"睡一次，醒来就好了"。
看 v6.6 的实现会发现不是 —— 它是一个 **`for (;;)`**：

```c
struct semaphore_waiter {
	struct list_head list;
	struct task_struct *task;
	bool up;
};

/*
 * Because this function is inlined, the 'state' parameter will be
 * constant, and thus optimised away by the compiler.  Likewise the
 * 'timeout' parameter for the cases without timeouts.
 */
static inline int __sched ___down_common(struct semaphore *sem, long state,
								long timeout)
{
	struct semaphore_waiter waiter;

	list_add_tail(&waiter.list, &sem->wait_list);
	waiter.task = current;
	waiter.up = false;

	for (;;) {
		if (signal_pending_state(state, current))
			goto interrupted;
		if (unlikely(timeout <= 0))
			goto timed_out;
		__set_current_state(state);
		raw_spin_unlock_irq(&sem->lock);
		timeout = schedule_timeout(timeout);
		raw_spin_lock_irq(&sem->lock);
		if (waiter.up)
			return 0;
	}

 timed_out:
	list_del(&waiter.list);
	return -ETIME;

 interrupted:
	list_del(&waiter.list);
	return -EINTR;
}
```

### 为什么必须是循环

因为**醒来的原因有四种，只有一种是"真的拿到了"**：

| 醒来原因 | `waiter.up` | `schedule_timeout` 返回 | 后续动作 |
|---------|------------|------------------------|---------|
| ① 被 `up()` 传了令牌 | **`true`** | 剩余 jiffies | `return 0` ✅ |
| ② 收到信号 | `false` | 剩余 jiffies | 循环回去 → `signal_pending_state` 命中 → `-EINTR` |
| ③ 超时 | `false` | `0` | 循环回去 → `timeout <= 0` 命中 → `-ETIME` |
| ④ 假唤醒（spurious wakeup） | `false` | 剩余 jiffies | 循环回去 → 重新睡 |

注意 ② 和 ③ 都**不是**在 `schedule_timeout()` 返回后立即判断的，而是**回到循环顶部**
再判断。这个顺序很重要：

- 收到信号时，`schedule_timeout()` 会返回一个正的剩余时间（因为还没到点），
  但循环顶部 `signal_pending_state(state, current)` 会先命中 → `-EINTR`；
- 超时情况下，`schedule_timeout()` 返回 `0`，但代码并不在这里判 `0`，
  而是让循环顶部的 `timeout <= 0` 命中 → `-ETIME`。

**这是个很干净的写法**：把"我为什么醒了"的所有判断集中在循环顶部，
而不是散落在 `schedule_timeout()` 返回后的各处。

### 成功的唯一判据是 `waiter.up`，不是 `count`

注意循环里**完全没有检查 `sem->count`**。它只看：

```c
if (waiter.up)
	return 0;
```

`waiter.up` 由 `up()` 那一侧在 `raw_spin_lock_irqsave` 保护下置 `true`（见 §8）。
这就是"传令牌"（token passing）模型：**`up()` 不把令牌放回池子里，而是直接递给队首**。

### `struct semaphore_waiter` 是**栈上变量**

```c
static inline int __sched ___down_common(...)
{
	struct semaphore_waiter waiter;      /* ← 局部变量！在栈上 */
	list_add_tail(&waiter.list, &sem->wait_list);
	...
}
```

这看起来很危险 —— 把一个栈变量的 `list_head` 挂进了全局链表里，
函数一返回这个地址就失效了。

**但它是对的**，原因就在 `schedule_timeout()` 那一行：

```
raw_spin_unlock_irq(&sem->lock);
timeout = schedule_timeout(timeout);   /* ← 在这里切走，又在这里切回 */
raw_spin_lock_irq(&sem->lock);
```

`raw_spin_unlock_irq()` **开了中断**（恢复的是进入 `down()` 时的 flags），
所以 `schedule_timeout()` 可以真的把当前任务切走；但 `___down_common`
这个函数**没有返回**，它的栈帧还完整地挂在那里。等被唤醒重新调度回来时，
执行流从这个栈帧的下一行继续 —— `waiter` 的地址依然是有效的。

**这是内核里"用栈变量当等待节点"的通用手法**（mutex、rwsem、completion 都这么干），
它的前提是：**睡眠点一定在把节点摘链之前**。三条退出路径都保证了这点：

- `return 0`（成功）：节点已经被 `up()` 侧的 `__up()` 用 `list_del` 摘掉了；
- `-ETIME`：`list_del(&waiter.list)` 后再 return；
- `-EINTR`：`list_del(&waiter.list)` 后再 return。

⚠️ **反面案例**：如果某个睡眠锁的实现在"摘链"和"函数返回"之间又被调度走，
或者节点所在的栈帧被提前释放，链表里就会留下一个悬垂指针 →
下一次 `list_first_entry()` 拿到的是垃圾 → 内核 oops。这也是为什么
**中断上下文绝对不能调 `down()`** —— ISR 的栈帧生命周期和进程完全不同。

### 外层还包了一层 tracepoint

```c
static inline int __sched __down_common(struct semaphore *sem, long state,
					long timeout)
{
	int ret;

	trace_contention_begin(sem, 0);
	ret = ___down_common(sem, state, timeout);
	trace_contention_end(sem, ret);

	return ret;
}
```

`trace_contention_begin` / `trace_contention_end` 出自 `include/trace/events/lock.h`，
可以用 `perf` 或 `tracefs` 直接观测**每个信号量的争用时长**：

```bash
# 观测所有锁争用事件（含 semaphore）
perf lock record -- ./workload
perf lock report

# 或者直接抓 tracepoint
echo 1 > /sys/kernel/tracing/events/lock/contention_begin/enable
echo 1 > /sys/kernel/tracing/events/lock/contention_end/enable
cat /sys/kernel/tracing/trace_pipe
```

**这是排查"信号量争用导致尾延迟"的直接工具**。注意 tracepoint 的开销本身不低，
生产环境上只能在采样窗口内开。

---

## 8. ⭐ `up()` 在有等待者时**根本不 `count++`**，而是直接"传令牌"

先看源码：

```c
/**
 * up - release the semaphore
 * @sem: the semaphore to release
 *
 * Release the semaphore.  Unlike mutexes, up() may be called from any
 * context and even by tasks which have never called down().
 */
void __sched up(struct semaphore *sem)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&sem->lock, flags);
	if (likely(list_empty(&sem->wait_list)))
		sem->count++;        /* 没人等 → 令牌放回池子 */
	else
		__up(sem);           /* 有人等 → 直接递过去，count 不动 */
	raw_spin_unlock_irqrestore(&sem->lock, flags);
}
EXPORT_SYMBOL(up);

static noinline void __sched __up(struct semaphore *sem)
{
	struct semaphore_waiter *waiter = list_first_entry(&sem->wait_list,
						struct semaphore_waiter, list);
	list_del(&waiter->list);
	waiter.up = true;
	wake_up_process(waiter->task);
}
```

**关键观察**：走 `__up()` 分支时，`sem->count` **完全没有被修改**，全程保持 0。

### 为什么不"既 count++ 又唤醒"

直觉上会觉得应该两步都做 —— 把令牌放回池子（`count++`），再把队首唤醒。
推演一下就会发现这会**破坏 FIFO**：

```
初始：count = 0，wait_list = [A, B]（A 先来的）

任务 C 出现在 CPU 上，正准备调 down()

持有者调 up()：
  ┌─ 朴素做法（错误）────────────────────────────
  │  count++          → count = 1
  │  wake_up(A)       → A 进入 runqueue，但还没跑到
  │  raw_spin_unlock_irqrestore()
  │
  │  ⚠️ 此刻 count == 1，而 A 还在 runqueue 上没被调度到
  │  → C 调 down()，看到 count > 0，直接 count-- 拿走！
  │  → C 插到了 A 前面（A 是先到的等待者）
  │  → A 被唤醒后发现令牌没了，只好再次 list_add_tail 到队尾
  │  → A 排到了 B 后面，彻底乱序
  └───────────────────────────────────────────────

  ┌─ v6.6 做法（正确）────────────────────────────
  │  __up()：list_del(A), A->up = true, wake_up(A)
  │  count 保持 0
  │  raw_spin_unlock_irqrestore()
  │
  │  → C 调 down()，看到 count == 0 → 老实去队尾排队
  │  → A 被调度到时，循环里看到 waiter.up == true → return 0 ✅
  │  → FIFO 成立
  └───────────────────────────────────────────────
```

**根本原因**：`wake_up_process()` 只是把任务放进 runqueue，
**它不会立刻被调度到**。在"A 被唤醒"和"A 真正开始跑"之间存在一个窗口，
朴素做法在这个窗口里让 `count` 短暂为正，任何新来的任务都能合法地抢走它。

v6.6 的解法是**关掉这个窗口**：既然 `count` 一直是 0，新来的任务就无机可乘。
令牌不是"放回池子"，而是**直接绑定到 A 的 `waiter.up` 标志上** —— 别人拿不走。

### 这个设计和 mutex / rwsem 是同构的

| 原语 | 释放方行为 | 目的 |
|------|-----------|------|
| semaphore | `wait_list` 非空时不 `count++`，直接 `waiter.up = true` | 保 FIFO，防插队 |
| mutex | 直接把 `owner` 字段移交给等待者（handoff） | 同上 |
| rwsem | 直接把读者计数 / 写者标志移交 | 同上 |

**这是一条通用的内核设计原则**：睡眠锁的释放路径，在有等待者时
**不做"归还 + 唤醒"两步，而是做"直接移交"一步**。因为两步之间必有窗口。

### 回到 §1 那句注释

现在可以理解 `kernel/locking/semaphore.c` 开头那段注释的后半句了：

> The ->count variable represents how many more tasks can acquire this
> semaphore.  **If it's zero, there may be tasks waiting on the wait_list.**

注意 "**may be**"。`count == 0` 有两种完全不同的含义：

| `count == 0` 时 | `wait_list` | 含义 |
|----------------|------------|------|
| 情况一 | **空** | 令牌被某个任务持有（或池子空了），**没人排队** |
| 情况二 | **非空** | 有人在排队，`up()` 会走传令牌路径 |

所以**绝对不能**用 `count == 0` 来推断"有没有人在等"。要看 `wait_list`。
（当然，用户代码本来就不该直接读 `sem->count` —— 结构体上方写着
"Please don't access any members of this structure directly"。）

---

## 9. 为什么内核新代码一律用 mutex（**理由不是性能**）

官方 `Documentation/locking/mutex-design.rst` 的 "When to use mutexes" 一节，整节就一句话：

> Unless the strict semantics of mutexes are unsuitable and/or the critical
> region prevents the lock from being shared, **always prefer them to any other
> locking primitive**.

翻译：**除非 mutex 的严格语义不适合，或者临界区本身导致锁无法共享，
否则永远优先用 mutex，而不是任何其他锁原语。**

注意它的措辞是 "**any other locking primitive**" —— 包括 semaphore，
也包括 spinlock、rwlock、rwsem。

### 理由一：mutex 有 9 条**强制**语义，semaphore 一条都没有

`mutex-design.rst` 的 "Semantics" 一节：

> The mutex subsystem checks and enforces the following rules:
>
> - Only one task can hold the mutex at a time.
> - Only the owner can unlock the mutex.
> - Multiple unlocks are not permitted.
> - Recursive locking/unlocking is not permitted.
> - A mutex must only be initialized via the API (see below).
> - A task may not exit with a mutex held.
> - Memory areas where held locks reside must not be freed.
> - Held mutexes must not be reinitialized.
> - Mutexes may not be used in hardware or software interrupt
>   contexts such as tasklets and timers.

逐条对照 semaphore：

| # | mutex 规则 | mutex 检查？ | semaphore 检查？ |
|---|-----------|-------------|-----------------|
| 1 | 同一时刻只有一人持有 | ✅ | ❌（count 可以 >1，本来就允许） |
| 2 | **只有持有者能解锁** | ✅ | ❌ **任何人都能 `up()`** |
| 3 | 禁止重复解锁 | ✅ | ❌ **多调一次 `up()` 会凭空造令牌** |
| 4 | 禁止递归加锁 | ✅ | ❌ 递归 `down()` 会自死锁，无人检测 |
| 5 | 只能用 API 初始化 | ✅ | ⚠️ 只有 `sema_init` / `DEFINE_SEMAPHORE` |
| 6 | 任务不能持锁退出 | ✅ | ❌ |
| 7 | 持锁期间内存不能释放 | ✅ | ❌ |
| 8 | 持有的锁不能重新初始化 | ✅ | ❌ |
| 9 | 不能用于软硬件中断上下文 | ✅ | ⚠️ 部分（`down_trylock`/`up` 可以） |

**semaphore 在 1~4、6~8 这七条上完全没有检查。** 其中最致命的是 **#2 和 #3**：

```c
/* semaphore：这些错误编译器和内核都不会告诉你 */
up(&sem);
up(&sem);              /* ❌ 重复 up：count 凭空 +1，后续会有两个任务同时进临界区 */

/* 线程 A down，线程 B up —— 合法但通常意味着设计错了 */
```

对比 mutex：

```c
mutex_unlock(&m);
mutex_unlock(&m);      /* ✅ DEBUG_MUTEXES 下立刻 debug_bug 打印并可能 panic */
```

### 理由二：`CONFIG_DEBUG_MUTEXES` 的 5 项 debug 能力，semaphore 全没有

`mutex-design.rst`：

> These semantics are fully enforced when CONFIG DEBUG_MUTEXES is enabled.
> In addition, the mutex debugging code also implements a number of other
> features that make lock debugging easier and faster:
>
> - Uses symbolic names of mutexes, whenever they are printed
>   in debug output.
> - Point-of-acquire tracking, symbolic lookup of function names,
>   list of all locks held in the system, printout of them.
> - Owner tracking.
> - Detects self-recursing locks and prints out all relevant info.
> - Detects multi-task circular deadlocks and prints out all affected
>   locks and tasks (and only those tasks).

| debug 能力 | mutex | semaphore |
|-----------|-------|-----------|
| 打印锁的**符号名** | ✅ | ❌（只能看到地址） |
| **获取点追踪**（在哪一行拿的锁）+ 系统内所有持锁列表 | ✅ | ❌ |
| **owner 追踪** | ✅ | ❌ 本来就没有 owner 概念 |
| **自递归检测** | ✅ | ❌ |
| **多任务环形死锁检测**（只打印相关锁和任务） | ✅ | ❌ |

**这才是"用 mutex"的真正理由。** 不是为了性能，是为了**出了问题能查**。
一个只有地址没有符号名、不知道谁持有、检测不出环形死锁的锁，在生产环境上出问题时
基本等于无法诊断。

对比一下两份 oops 输出的信息量差异：

```
/* mutex + DEBUG_MUTEXES 检测到自递归 */
BUG: mutex trylock failure ...
 showing all locks held in the system:
 #0: ffff888012345678 (&dev->lock){+.+.}-{3:3}, at: my_func+0x42/0x100 [mydrv]
 ...
 
/* semaphore 递归 down()：什么都不会发生，进程静静卡死在 D 状态，
   ps 只显示 "D"，没有任何提示。 */
```

### 理由三（性能）：mutex 有**乐观自旋**，semaphore 没有

这一条才是性能层面的差异，而且方向可能和直觉相反 —— **mutex 更快**。

`struct mutex` 里有一个 semaphore 完全没有的字段：

```c
/* include/linux/mutex.h —— v6.6 */
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

`mutex-design.rst` 描述 mutex 的三条获取路径：

> When acquiring a mutex, there are three possible paths that can be
> taken, depending on the state of the lock:
>
> (i) **fastpath**: tries to atomically acquire the lock by cmpxchg()ing
>     the owner with the current task. This only works in the uncontended
>     case (cmpxchg() checks against 0UL, so all 3 state bits above have
>     to be 0). If the lock is contended it goes to the next possible path.
>
> (ii) **midpath**: aka **optimistic spinning**, tries to spin for acquisition
>      while the lock owner is running and there are no other tasks ready
>      to run that have higher priority (need_resched). The rationale is
>      that if the lock owner is running, it is likely to release the lock
>      soon. The mutex spinners are queued up using MCS lock so that only
>      one spinner can compete for the mutex.
>      ...
>
> (iii) **slowpath**: last resort, if the lock is still unable to be acquired,
>      the task is added to the wait-queue and sleeps until woken up by the
>      unlock path. Under normal circumstances it blocks as TASK_UNINTERRUPTIBLE.
>
> While formally kernel mutexes are sleepable locks, it is path (ii) that
> makes them more practically a **hybrid type**. By simply not interrupting a
> task and busy-waiting for a few cycles instead of immediately sleeping,
> the performance of this lock has been seen to **significantly improve**
> a number of workloads. Note that this technique is also used for rw-semaphores.

**三条路径对比**：

| | mutex | semaphore |
|--|-------|-----------|
| 快路径 | `cmpxchg` owner（无争用时） | `count--`（无争用时） |
| **中路径（乐观自旋）** | ✅ **有**：持有者在跑时就自旋等一会儿，用 MCS 排队 | ❌ **没有** |
| 慢路径 | 入队 + 睡眠 | 入队 + 睡眠 |

semaphore 只有两条：**要么 `count > 0` 直接拿到，要么 `count == 0` 直接
`__down(sem)` → `schedule_timeout()` 睡下去**。中间没有"自旋等一下"这个档位。

**后果**：

```
信号量争用的代价 = 完整的两次上下文切换 + 调度器往返
                ≈ 数 µs 量级，且方差极大（取决于调度器、CFS、迁移）
                
mutex 争用的代价（持有者正在跑时）= 几个 ~ 几十个 cycle 的自旋
                                ≈ 亚 µs 量级
```

所以对于"临界区很短、持有者大概率马上就放"的场景，**mutex 的中路径会让它比
semaphore 快一个数量级**。这也是官方文档说 mutex "practically a hybrid type"
（实际上是混合型）的原因。

### 什么时候**必须**用 semaphore

虽然有上面这些，semaphore 不是没用武之地。官方的豁免条款是
"**除非 mutex 的严格语义不适合**"。具体是三种情况：

| 场景 | 为什么 mutex 不行 |
|------|------------------|
| ① **计数 > 1**（资源池 / 限流） | mutex 是二值的，语义上就是互斥 |
| ② **无归属释放**（A 获取、B 释放） | 违反 mutex 规则 #2，DEBUG 下会报错 |
| ③ 需要在**中断上下文**释放 | mutex 规则 #9 明确禁止 mutex 用于中断上下文；`up()` 可以 |

场景 ③ 值得展开：`up()` 的 kernel-doc 明确说：

> Release the semaphore. Unlike mutexes, **up() may be called from any
> context and even by tasks which have never called down().**

这是个很实用的能力：**进程上下文 `down_interruptible()` 等硬件，ISR 里 `up()` 通知**。
这个模式用 mutex 是做不到的（ISR 里不能 `mutex_unlock`），用 completion（10.6）
更好，但 semaphore 也能work。

---

## 10. PREEMPT_RT 上的 semaphore —— **不变**

对照 10.2 和 10.3 的结论：

| 原语 | 非 RT | `CONFIG_PREEMPT_RT` | 是否变化 |
|------|-------|---------------------|---------|
| `spinlock_t` | 真自旋 | → `struct rt_mutex_base`，**可睡眠** | ✅ 变 |
| `rwlock_t` | 真自旋 | → `struct rwbase_rt`，**可睡眠** | ✅ 变 |
| `raw_spinlock_t` | 真自旋 | 真自旋 | ❌ 不变 |
| **`struct semaphore`** | 睡眠锁 | **睡眠锁** | ❌ **不变** |
| `struct completion` | 睡眠锁 | 睡眠锁 | ❌ 不变 |

原因很直白：**semaphore 本来就是睡眠锁**，RT 补丁要解决的问题是"把自旋锁变成
可睡眠的，以免自旋者在 RT 上造成不可预测的抢占延迟"。semaphore 从来不自旋，
所以根本没有要改的地方。

它内部的 `raw_spinlock_t` 也保持不变 —— 这正好印证了 §1 那条规律：
**正因为它是 `raw_spinlock_t`，RT 才敢不动它**。如果哪天有人"顺手"把它改成
`spinlock_t`，这个 semaphore 在 RT 内核上就会出问题。

---

## HFT / 嵌入式关联

### ⭐ 信号量获取延迟是**调度器级**的 —— 热路径禁用

这是本篇对 HFT 最重要的一条结论。把 §9 的性能分析再往具体数字上推一步：

```
信号量争用时的一次获取：
  ① down() 发现 count == 0
  ② list_add_tail 挂到 wait_list
  ③ __set_current_state() + raw_spin_unlock_irq()
  ④ schedule_timeout()   → 上下文切换出去（保存/恢复寄存器、切换地址空间、
                            可能触发负载均衡迁移到另一个 NUMA 节点）
  ⑤ ... 等待持有者 up() ...
  ⑥ wake_up_process()    → 放进 runqueue
  ⑦ ... 等待调度器选中自己（可能被更高优先级任务插队、可能被抢占）...
  ⑧ 上下文切换回来
  ⑨ raw_spin_lock_irq() + 检查 waiter.up
  ⑩ 返回

代价量级：数 µs ~ 数十 µs，且 P99/P99.9 方差极大
```

对比：

| 原语 | 无争用 | 轻争用 | HFT 热路径 |
|------|--------|--------|-----------|
| `atomic_t` / `READ_ONCE` | ~1–20 cycles | 重试 | ✅ 首选 |
| 无锁环形队列（SPSC） | ~10 cycles | 无 | ✅ 首选 |
| `spinlock_t` | ~20–40 cycles | 几十 ~ 几百 cycles（qspinlock MCS 排队） | ⚠️ 可用，需独占核 |
| **mutex（中路径乐观自旋）** | ~20 cycles | 自旋几百 cycles（持有者在跑时） | ⚠️ 控制面可以 |
| **semaphore** | ~20 cycles | **数 µs + 大方差**（直接睡） | ❌ **禁止** |

**结论**：信号量的延迟不是"锁本身的开销"，而是**两次调度器往返**。
在低延迟系统里，这个数字和 P99 尾延迟目标（通常要求 < 10 µs，甚至 < 1 µs）
是同一个量级 —— 也就是说，**一次信号量争用就可能直接吃掉整个尾延迟预算**。

**热路径替代方案**：

| 需求 | ❌ 不要用 | ✅ 用 |
|------|---------|-------|
| 单生产者单消费者传数据 | semaphore + 缓冲区 | **SPSC 无锁环形队列**（`READ_ONCE`/`WRITE_ONCE` + acquire/release） |
| 限制并发数 | counting semaphore | `atomic_t` 计数 + 重试 / 退避（或干脆不做限流，用分区） |
| 等硬件就绪（进程上下文） | `down_interruptible` | 可以接受（在控制面/初始化路径） |
| 统计计数器 | semaphore 保护 | `local_t` / per-CPU 计数 + 聚合读 |

### 独占物理核是唯一可靠答案

结合 10.2 的结论：在 VM 上 `CONFIG_PARAVIRT` 会把 `spinlock_t` 的慢路径
**默认劫持成裸 TAS**（`virt_spin_lock_key` 是 `DECLARE_STATIC_KEY_TRUE`），
争用退化成 O(N²) cacheline 乒乓，且持锁者被 hypervisor 抢占时无人能偷锁。

对 HFT 的推论是分层的：

| 层 | 建议 |
|----|------|
| **热路径（订单发送、行情处理）** | 无锁 + 独占物理核 + 关中断绑核（Linux 侧用 `isolcpus` + `nohz_full`） |
| **温路径（风控、仓位更新）** | `spinlock_t` 或 mutex（中路径自旋），仍绑核 |
| **控制面（初始化、配置、错误处理）** | semaphore / completion / mutex 都可以 |

### `D` 状态进程的运维噩梦（嵌入式尤其严重）

§4 讲过 `down()` 会睡在 `TASK_UNINTERRUPTIBLE` 上，产生杀不死的 `D` 状态进程。
在嵌入式设备上这个后果被放大：

- 没有运维人员能上去排查，只能靠看门狗重启；
- 看门狗如果自己也被这个 D 状态任务阻塞（比如共享同一个锁），整个设备假死；
- 现场复现困难 —— 可能几个月才出现一次。

**规则**：嵌入式驱动里**永远不写 `down()`**，用 `down_killable()` 或
`down_timeout()`，并且**永远给一个有限的超时**。

### 用 tracepoint 量化争用

§7 提到的 tracepoint 是排查尾延迟的直接工具：

```bash
# 抓信号量争用（含具体地址和耗时）
trace-cmd record -e lock:contention_begin -e lock:contention_end ./workload
trace-cmd report | grep -A2 contention
```

如果 `perf lock report` 里出现某个 semaphore 的 `con-bounces` 很高，
就说明它在被反复争用 —— 这往往意味着架构上该改成无锁了，而不是调参数。

---

## 实践模板

```c
#include <linux/semaphore.h>
#include <linux/errno.h>

/* ---------- 模板一：资源池（计数 > 1，semaphore 的正当用途） ---------- */

#define POOL_SLOTS	8

struct my_dev {
	struct semaphore slots;      /* 可用槽位数，初值 = POOL_SLOTS */
	void __iomem *base;
};

static int my_dev_probe(struct platform_device *pdev)
{
	struct my_dev *dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* ⚠️ v6.4 起必须两个参数 */
	sema_init(&dev->slots, POOL_SLOTS);

	platform_set_drvdata(pdev, dev);
	return 0;
}

static int submit_one(struct my_dev *dev)
{
	/* ✅ 用 killable：只让致命信号打断，不必处理无关信号的 -EINTR */
	if (down_killable(&dev->slots))
		return -EINTR;

	/* ... 占用一个槽位干活（可以睡眠、可以做 I/O）... */

	up(&dev->slots);             /* 可在中断上下文调用 */
	return 0;
}


/* ---------- 模板二：进程等硬件，ISR 通知（无归属释放） ---------- */

static DECLARE_WAIT_QUEUE_HEAD(hw_wq);
static DEFINE_SEMAPHORE(hw_done, 0);     /* 初值 0 = 一开始就是"未就绪" */

static int wait_for_hw(struct my_dev *dev, unsigned int timeout_ms)
{
	long ret;

	/* 有上界，且不可被信号打断（硬件等的就是硬件）*/
	ret = down_timeout(&dev->hw_done, msecs_to_jiffies(timeout_ms));
	if (ret == -ETIME) {
		dev_err(dev->dev, "hardware timeout after %u ms\n", timeout_ms);
		return -ETIMEDOUT;
	}
	return 0;                            /* ret == 0：拿到令牌 */
}

/* 中断上半部：只 up，不睡 */
static irqreturn_t my_hw_isr(int irq, void *data)
{
	struct my_dev *dev = data;

	/* ... 清中断 ... */
	up(&dev->hw_done);                   /* ✅ up() 可在中断上下文调用 */
	return IRQ_HANDLED;
}


/* ---------- 模板三：中断上下文只能用 trylock ---------- */

static irqreturn_t my_isr(int irq, void *data)
{
	struct my_dev *dev = data;

	/* ✅ down_trylock 不睡，返回值是反的：0 = 成功 */
	if (down_trylock(&dev->slots)) {
		/*
		 * 拿不到 —— 注意这里 **绝对不能** up()！
		 * 因为没拿到就不该释放，否则凭空造令牌。
		 */
		dev->slot_starved++;
		return IRQ_NONE;
	}

	/* ... 用槽位 ... */
	up(&dev->slots);
	return IRQ_HANDLED;
}
```

⚠️ **模板三里的注释是本篇最容易踩的坑**：`down_trylock()` 返回 `1` 表示**失败**，
此时**不能**调 `up()`。写成

```c
if (down_trylock(&sem)) {   /* 语义反了 */
	do_work();
	up(&sem);               /* 凭空造令牌 → 后续会有多个任务同时进临界区 */
}
```

就是 §5 讲的那个隐蔽 bug。

---

## 易错点核对表

| # | 易错点 | 正确做法 |
|---|--------|---------|
| 1 | 用 `down()` | ❌ v6.6 已废弃。用 `down_interruptible()` / `down_killable()` / `down_timeout()` |
| 2 | `if (down_trylock(&s)) { 进临界区 } ` | ❌ 反了。成功返回 **0**。写 `if (!down_trylock(&s))` |
| 3 | `DEFINE_SEMAPHORE(name);` | ❌ v6.4 起编译不过。写 `DEFINE_SEMAPHORE(name, 1)` |
| 4 | `DEFINE_SEMAPHORE(name, 0)` 迁移老代码 | ❌ 老写法隐含 `count=1`。迁移时显式写 `1` |
| 5 | 中断上下文调 `down()` | ❌ 会睡。只能用 `down_trylock()`；释放侧 `up()` 可以 |
| 6 | 用 `count == 0` 推断"有人在等" | ❌ 可能没人等，只是令牌被持有 |
| 7 | 以为 `up()` 总是 `count++` | ❌ 有等待者时 `count` 不变，直接传令牌 |
| 8 | 以为 semaphore 有 owner 概念 | ❌ 没有。任何人都能 `up()`，这是官方明说的 |
| 9 | 以为 semaphore 有 lockdep 死锁检测 | ❌ 只有 9 条 mutex 语义 + 5 项 DEBUG_MUTEXES 能力的一半都没有 |
| 10 | 以为 semaphore 快 | ❌ 没有乐观自旋，争用即睡眠，**调度器级延迟** |
| 11 | 热路径用 semaphore 做生产者-消费者 | ❌ 用 SPSC 无锁环形队列 |
| 12 | 把 `struct semaphore` 里的 `raw_spinlock_t` "顺手"改成 `spinlock_t` | ❌ RT 上会坏。规律见 §1 |

---

## 常见陷阱

1. 混淆 semaphore 和 mutex —— mutex 有归属（只有持有者能解锁），semaphore 无归属
2. 以为 counting semaphore 常用于内核 —— 内核中大多用 mutex，semaphore 只在特殊场景
   （计数 >1 / 无归属释放 / 中断侧释放）
3. 在中断上下文中调用 `down()` —— `down()` 会睡眠，中断只能用 `down_trylock()`
4. **（v6.6 补充）** 用 `down()` —— 已废弃，`TASK_UNINTERRUPTIBLE` 导致 `D` 状态进程
5. **（v6.6 补充）** 把 `down_trylock()` 的返回值当布尔"成功"用 —— 它是反的
6. **（v6.6 补充）** 复制 v6.3 及以前的 `DEFINE_SEMAPHORE(name)` —— v6.4 起要两个参数
7. **（v6.6 补充）** 以为 `up()` 会 `count++` 再唤醒 —— 实际是直接传令牌，`count` 不动

---

## 自测题

<details>
<summary>自测题（点击展开）</summary>

**Q1.** semaphore 和 mutex 的核心区别？

<details><summary>答案</summary>

mutex：① 有归属（owner 字段），只有持锁者能 unlock。② 支持优先级继承（rt_mutex）。③ 支持 lockdep。semaphore：① 无归属，任何人可以 up()。② 初始值可 >1（计数信号量）。③ 无优先级继承。内核新代码推荐 mutex，semaphore 只在需要计数语义或无归属场景使用。

<details><summary>按 v6.6 修订/补充</summary>

这个答案漏掉了**最关键的一条** —— 官方文档
`Documentation/locking/mutex-design.rst` 说"优先用 mutex"的**首要理由不是性能、
而是可调试性**：mutex 有 **9 条强制语义**（只有持有者能解锁 / 禁止重复解锁 /
禁止递归加锁 / 不能持锁退出 / 持锁内存不能释放 / 持有的锁不能重初始化 /
不能用于中断上下文……），semaphore **一条都不检查**；`CONFIG_DEBUG_MUTEXES`
还提供 **5 项 debug 能力**（符号名 / 获取点追踪 / owner 追踪 / 自递归检测 /
环形死锁检测），semaphore **全都没有**。

**另外一个方向相反的补充**：性能上 **mutex 反而更快** —— `struct mutex` 里有
`struct optimistic_spin_queue osq`（`CONFIG_MUTEX_SPIN_ON_OWNER`），持有者在跑时会
**乐观自旋**等一会儿（midpath），semaphore 没有这个档位，一争用就直接
`schedule_timeout()` 睡下去。所以 mutex 官方称自己是 "hybrid type"（混合型），
而信号量的争用延迟是**调度器级**的（数 µs，方差大）。

</details>
</details>

**Q2.** counting semaphore 在什么场景下有用？

<details><summary>答案</summary>

① 限制并发数：如限制同时打开的文件数（初始值=最大并发数）。② 生产者-消费者：semaphore 计数 = 队列中可用元素数，消费者 down() 取数据，生产者 up() 放数据。③ 资源池：初始值=池大小，获取资源 down()，释放 up()。但在内核中，这些场景更常用 kfifo + waitqueue 或 mempool。

<details><summary>按 v6.6 修订/补充</summary>

这三条里，只有 **① 限制并发数 / 资源池** 是 semaphore 在内核里的
正当用途。②③ 在**内核里**通常有更好的选择（kfifo + waitqueue、mempool），
但在**用户态**（HFT 的进程间通信）里 ②③ 依然是标准做法。

另外 §9 给出了 semaphore 的三个**不可替代场景**（mutex 语义上做不到的）：

| 场景 | 为什么 mutex 不行 |
|------|------------------|
| 计数 > 1 | mutex 是二值的 |
| 无归属释放（A 获取、B 释放） | 违反 mutex 规则 #2 |
| 需要在中断上下文释放 | mutex 规则 #9 明确禁止；`up()` 可以 |

第三条尤其实用：`up()` 的 kernel-doc 原文 —— *"Unlike mutexes, up() may be called
from any context and even by tasks which have never called down()"*。
这就是"进程上下文 `down_interruptible()` 等硬件、ISR 里 `up()` 通知"这个模式成立的原因。

</details>
</details>

**Q3.** HFT 中 semaphore 的用户态对应物？

<details><summary>答案</summary>

① `std::counting_semaphore<N>`（C++20）：计数信号量。② `sem_t`（POSIX）：进程间或线程间信号量。③ `std::binary_semaphore` = mutex 的近似。HFT 热路径避免 semaphore（有 futex 开销），用无锁队列代替生产者-消费者模式。非热路径可以用 semaphore 做资源限流。

<details><summary>按 v6.6 修订/补充</summary>

补一个可量化的判据。
内核态和用户态的结论是一致的 —— **信号量的争用代价不是锁本身的开销，
而是两次调度器往返**（切换出去 + 切换回来，中间还要等调度器选中自己）。
量级是 **数 µs 且方差极大**，和 HFT 的 P99 尾延迟预算（通常 < 10 µs）同一量级。

对比表（无争用 → 轻争用）：

| 原语 | 无争用 | 轻争用 | 热路径 |
|------|--------|--------|--------|
| `atomic_t` / `READ_ONCE` | ~1–20 cycles | 重试 | ✅ |
| SPSC 无锁环 | ~10 cycles | 无 | ✅ |
| `spinlock_t` | ~20–40 cycles | 几十~几百 cycles | ⚠️ 需独占核 |
| mutex（乐观自旋） | ~20 cycles | 自旋几百 cycles | ⚠️ 控制面 |
| **semaphore** | ~20 cycles | **数 µs + 大方差** | ❌ 禁止 |

所以第 ④ 点要更明确：**HFT 热路径不是"避免"semaphore，是"禁止"**。
用户态的 `sem_wait` 走 futex 快路径时确实不进内核（无争用时 ~20ns），
但**一旦争用就进内核 `futex_wait`** —— 和内核 semaphore 一样是调度器级延迟。
热路径的替代品是 **SPSC 无锁环形队列**（`memory_order_acquire/release` 的
`head`/`tail` 分离读写，无 RMW 竞争）。

</details>
</details>

**Q4.** 为什么 `struct semaphore` 内部的锁是 `raw_spinlock_t` 而不是 `spinlock_t`？

<details><summary>答案</summary>

因为信号量的慢路径 `___down_common()` 是**手工编排**的"放锁 → 睡 → 拿锁"序列：

```c
__set_current_state(state);
raw_spin_unlock_irq(&sem->lock);        /* ① 自己放锁 */
timeout = schedule_timeout(timeout);    /* ② 睡眠 */
raw_spin_lock_irq(&sem->lock);          /* ③ 醒来后自己拿回锁 */
if (waiter.up)
	return 0;
```

这段代码同时**关着中断**（用的是 `irq` 变体）又要保证"拿锁时不能睡眠"。
如果 `sem->lock` 是 `spinlock_t`，在 `CONFIG_PREEMPT_RT` 下它会被替换成
`rt_mutex_base` —— **会睡眠**。那么在关中断的临界区里睡眠是 RT 内核明确禁止的，
正确性无法保证。而 `raw_spinlock_t` 在任何配置下都真自旋，符合要求。

**可推广成一条规律（本节 §1）**：

> **所有"自己实现睡眠逻辑"的内核原语，内部锁一律用 `raw_spinlock_t`。**

验证：

| 原语 | 内部锁 | 自己编排睡眠 |
|------|--------|-------------|
| `struct semaphore` | `raw_spinlock_t lock` | ✅ `___down_common` |
| `struct mutex` | `raw_spinlock_t wait_lock` | ✅ `__mutex_lock_common` |
| `struct rw_semaphore` | `raw_spinlock_t wait_lock` | ✅ `rwsem_down_*_slowpath` |
| `struct completion` | `raw_spinlock_t`（swait 内） | ✅ `wait_for_completion*` |

反过来，这也解释了 §10 的结论：**PREEMPT_RT 完全不需要改 semaphore**
—— 它本来就是睡眠锁，内部锁又是 `raw_spinlock_t`，没有任何要改的地方。

</details>

**Q5.** 为什么 `up()` 在 `wait_list` 非空时**不**执行 `count++`？

<details><summary>答案</summary>

源码（`kernel/locking/semaphore.c`）：

```c
void __sched up(struct semaphore *sem)
{
	unsigned long flags;
	raw_spin_lock_irqsave(&sem->lock, flags);
	if (likely(list_empty(&sem->wait_list)))
		sem->count++;        /* 没人等 → 放回池子 */
	else
		__up(sem);           /* 有人等 → 直接传令牌，count 不动 */
	raw_spin_unlock_irqrestore(&sem->lock, flags);
}

static noinline void __sched __up(struct semaphore *sem)
{
	struct semaphore_waiter *waiter = list_first_entry(&sem->wait_list,
						struct semaphore_waiter, list);
	list_del(&waiter->list);
	waiter.up = true;        /* ← 令牌直接绑到队首的 waiter 上 */
	wake_up_process(waiter->task);
}
```

**原因：为了保 FIFO，防止插队。**

`wake_up_process()` 只是把任务放进 runqueue，**不会立刻调度到它**。
如果采用朴素的"count++ 再唤醒"两步：

```
count = 0，wait_list = [A, B]
持有者 up()：count++ → count = 1；wake_up(A)（A 还在 runqueue 上没跑到）
            unlock
⚠️ 此刻 count == 1 → 新来的 C 调 down()，看到 count > 0，直接拿走！
            → C 插到 A（先到的等待者）前面
            → A 醒来发现令牌没了，重新排到队尾（排到 B 后面）→ 乱序
```

v6.6 的做法把 `count` 一直保持 0，**关掉这个窗口**：新来的 C 看到 `count == 0`
只能老实去队尾排队。令牌不是"放回池子"，而是**绑定到 A 的 `waiter.up` 标志**上，
别人拿不走。A 醒来后循环里看到 `waiter.up == true` 就 `return 0`。

**这是个通用原则**（mutex / rwsem 同构）：睡眠锁的释放路径在有等待者时，
**不做"归还 + 唤醒"两步，而是做"直接移交"一步** —— 因为两步之间必有窗口。

</details>

**Q6.** `down_trylock()` 的返回值和其他 trylock 有什么不同？写错会怎样？

<details><summary>答案</summary>

**它是反的：0 = 成功，1 = 失败。**

```c
int __sched down_trylock(struct semaphore *sem)
{
	int count;
	raw_spin_lock_irqsave(&sem->lock, flags);
	count = sem->count - 1;
	if (likely(count >= 0))
		sem->count = count;
	raw_spin_unlock_irqrestore(&sem->lock, flags);
	return (count < 0);          /* 拿不到 → 返回 1 */
}
```

kernel-doc 的警告原文：

> **NOTE: This return value is inverted from both spin_trylock and
> mutex_trylock!  Be careful about this when converting code.**

| API | 成功 | 失败 |
|-----|------|------|
| `spin_trylock()` | 1 | 0 |
| `mutex_trylock()` | 1 | 0 |
| **`down_trylock()`** | **0** | **1** |

**写错的最坏后果**（这是它危险的地方）：

```c
if (down_trylock(&sem)) {     /* 作者以为"拿到了" */
	do_something();            /* 实际是"没拿到"的分支 */
	up(&sem);                  /* ❌ 凭空造了一个令牌 */
}
```

`up()` 会让 `count` 凭空 +1，等于**伪造了一个资源槽位**。后续会有多余的
任务同时进入临界区，表现为**偶发的数据损坏**。而且这个 bug 在测试环境
（无争用、永远走成功分支）里**完全不会触发**，上了生产偶发争用才炸 —— 极难复现。

**顺带一个隐蔽细节**：`sem->count` 是 `unsigned int`，但局部变量 `count`
声明成 `int`。这是**刻意**的 —— 若声明成 `unsigned`，`0 - 1` 会绕回
`0xFFFFFFFF`，`count >= 0` 恒真，逻辑彻底错误。看到不要"顺手改成一致的"。

</details>

**Q7.** `___down_common()` 里为什么是 `for (;;)` 而不是"睡一次就返回"？

<details><summary>答案</summary>

因为**醒来的原因有四种，只有一种是真的拿到了令牌**：

| 醒来原因 | `waiter.up` | 后续 |
|---------|------------|------|
| ① 被 `up()` 传令牌 | **`true`** | `return 0` ✅ |
| ② 收到信号 | `false` | 循环回顶部 → `signal_pending_state` 命中 → `-EINTR` |
| ③ 超时 | `false` | 循环回顶部 → `timeout <= 0` 命中 → `-ETIME` |
| ④ 假唤醒 | `false` | 循环回顶部 → 重新睡 |

```c
for (;;) {
	if (signal_pending_state(state, current))
		goto interrupted;
	if (unlikely(timeout <= 0))
		goto timed_out;
	__set_current_state(state);
	raw_spin_unlock_irq(&sem->lock);
	timeout = schedule_timeout(timeout);
	raw_spin_lock_irq(&sem->lock);
	if (waiter.up)
		return 0;            /* ← 成功的唯一判据 */
}
```

两个要点：

**① 判断集中在循环顶部，而不是散落在 `schedule_timeout()` 之后。**
收到信号时 `schedule_timeout()` 会返回正的剩余时间（没到点就醒了），
代码并不在这里判断，而是回到顶部让 `signal_pending_state()` 命中。
超时情况下 `schedule_timeout()` 返回 0，同样回到顶部让 `timeout <= 0` 命中。

**② 成功的判据是 `waiter.up`，全程没有检查 `sem->count`。**
这就是 §5 讲的"传令牌"模型 —— 令牌不在池子里，而是绑在 `waiter.up` 标志上。

**另一个值得注意的点**：`struct semaphore_waiter waiter` 是**栈上变量**，
却把它的 `list_head` 挂进了全局链表。这看起来危险，但**是对的**，
因为 `schedule_timeout()` 只是切走又切回**同一个栈帧**，`___down_common`
函数没有返回，`waiter` 的地址始终有效。三条退出路径都保证了"摘链先于返回"：
成功后节点已被 `__up()` 侧 `list_del`；`-ETIME` / `-EINTR` 两条路径都自己
`list_del(&waiter.list)`。

这也是内核等待队列的通用手法（mutex / rwsem / completion 都这么干），
前提是**睡眠点一定在摘链之前** —— 顺便解释了为什么 ISR 里绝对不能调 `down()`
（ISR 的栈帧生命周期和进程完全不同）。

</details>

**Q8.** 内核里 `down()` 已经被标记废弃了，为什么？改成什么？

<details><summary>答案</summary>

v6.6 的 kernel-doc 原文：

> **Use of this function is deprecated, please use down_interruptible() or
> down_killable() instead.**

**原因**：`down()` 睡在 `TASK_UNINTERRUPTIBLE` 上，且没有超时：

```c
static noinline void __sched __down(struct semaphore *sem)
{
	__down_common(sem, TASK_UNINTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
}
```

`TASK_UNINTERRUPTIBLE` 的语义是"**只有显式唤醒才能叫醒**" —— 信号来了也只是
记下 pending，不唤醒任务。`MAX_SCHEDULE_TIMEOUT`（= `LONG_MAX`）意味着没有上界。

后果链条：

```
down() 拿不到 → 睡 TASK_UNINTERRUPTIBLE → 进程进入 D 状态
             → 信号无效，kill -9 也只是个信号，同样无效
             → ps 里 STAT 列显示 "D"，杀不死
             → 如果那个 up() 永远不来（硬件挂了 / 驱动 bug）→ 永久卡死，只能重启
```

在生产/嵌入式设备上尤其严重：`D` 状态进程不响应任何信号，运维只能重启设备，
而且现场复现困难（可能几个月才一次）。

**改法对照**：

| 改成 | 返回值 | 特点 |
|------|--------|------|
| `down_interruptible()` | `0` / `-EINTR` | 任意信号都能打断，分支最多（最常用） |
| `down_killable()` | `0` / `-EINTR` | 只有**致命信号**（SIGKILL）能打断，分支更少 |
| `down_timeout(&s, HZ)` | `0` / `-ETIME` | 有上界，但仍不可被信号叫醒 |

⚠️ **改写不是简单换函数名**：`down()` 返回 `void`，换成 `down_interruptible()`
后**必须检查返回值**，否则被打断时会继续执行临界区代码：

```c
/* ❌ 编译过，逻辑错 */
down_interruptible(&dev->sem);

/* ✅ */
if (down_interruptible(&dev->sem))
	return -ERESTARTSYS;
```

`__must_check` 会在你**完全丢弃**返回值时报警，但"接收了却不判断"它管不着。

**嵌入式驱动的建议**：用 `down_killable()` 或 `down_timeout()`，
并且**永远给一个有限超时** —— 既能被 `kill -9` 兜底，又有上界。

</details>

**Q9.** `struct semaphore` 内部锁为什么用 `irqsave` 变体？（源码注释给了两层理由）

<details><summary>答案</summary>

`kernel/locking/semaphore.c` 开头注释原文：

```c
/*
 * The spinlock controls access to the other members of the semaphore.
 * down_trylock() and up() can be called from interrupt context, so we
 * have to disable interrupts when taking the lock.  It turns out various
 * parts of the kernel expect to be able to use down() on a semaphore in
 * interrupt context when they know it will succeed, so we have to use
 * irqsave variants for down(), down_interruptible() and down_killable()
 * too.
 */
```

**第一层（正常理由）**：`down_trylock()` 和 `up()` 可以在中断上下文调用。
如果在进程上下文拿内部锁时**不关中断**，ISR 一打断也去拿同一把锁 → 死锁。
所以必须 `irqsave`。

**第二层（历史包袱）**：*"内核里有代码指望在**知道自己会成功**时能在中断里调
`down()`"* —— 为了迁就这些老代码，`down()` / `down_interruptible()` /
`down_killable()` 也用了 irqsave 变体。

⚠️ **第二层千万不要模仿**。它的意思是有老代码在 ISR 里调 `down()`，
赌 `count > 0` 一定成功。这个赌注极脆弱：一旦 `count` 恰好为 0，
ISR 里就会调 `schedule_timeout()` → `BUG: scheduling while atomic`。

**正确写法**：中断上下文**只**用 `down_trylock()`（获取侧）和 `up()`（释放侧）。
`down_trylock` 的 kernel-doc 也确认了这点：

> Unlike mutex_trylock, this function **can be used from interrupt context**,
> and the semaphore can be released by any task or interrupt.

</details>

</details>

---

→ [10.5 mutex](./section-10.5-互斥体.md) · [4.4 休眠唤醒](../../chapter-04-process-scheduling/notes/section-4.4-休眠与唤醒.md)

> ↔ [ULK Ch5 §6 信号量与完成变量](../../../16-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-6-信号量与完成变量.md)
---
