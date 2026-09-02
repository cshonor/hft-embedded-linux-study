## ⑦ 大内核锁 · The Big Kernel Lock (BKL)

历史包袱：曾经存在一把 **覆盖几乎整个内核** 的巨锁 — 进内核就可能拿着它，粗暴保证「内核非抢占式互斥」。

| 属性 | 说明 |
|------|------|
| 粒度 | **极粗** — 扩展性差 |
| 命运 | **逐步撕掉**；现代内核 **禁止新代码使用** |
| 学习意义 | 理解「为何要细粒度锁 / RCU / per-CPU」 |

#### 为何淘汰

| 问题 | 后果 |
|------|------|
| 多核无法并行执行大量内核路径 | SMP 扩展性差 |
| 持锁时间长 | 延迟、抖动大 |
| 隐式全局序列化 | 难推理、难优化 |

```
旧世界:  用户态 ──syscall──► [ BKL ] 几乎全家桶串行
新世界:  细粒度 spinlock / mutex / RCU / per-CPU
```

#### 你需要记住的唯一行动项

| 规则 | |
|------|--|
| **新代码禁止 BKL** | 用 Ch 10 其它机制 |
| 读旧补丁/旧驱动见 `lock_kernel` | 当作考古，迁移掉 |

**HFT / 驱动：** 不要怀念「一把大锁省事」— 大锁 = 尾延迟与多核浪费。正确粗细：数据怎么共享，锁就围着数据走。

→ [Ch 9 争用与可扩展性](../../chapter-09-kernel-sync-intro/notes/section-9.6-争用和可扩展性.md) · [10.11 选型](./section-10.11-选型速查Ch-9--Ch-10.md)

### 常见陷阱

1. 以为 BKL 还在现代内核——2.6.37 完全移除，不存在了
2. 把 BKL 当通用锁——BKL 是特殊锁（可睡眠、递归、全局唯一），不推荐用于新代码
3. 以为 BKL 和 mutex 一样——BKL 可递归加锁、自动释放于 schedule()，mutex 不能

---

> **本篇分工**：上面速查表**原样保留**。本篇往下**不复述**"大锁不好"这种常识，
> 只做八件事，**全部拿历史源码实证**（BKL 在 v6.6 里已经一个字节都不剩，
> 唯一可靠的证据来源就是抓历史版本的 tag，这也是本篇和前几篇最大的不同）：
>
> ① 拆开 BKL 的**本体**：它就是一把 `raw_spinlock_t`，全局变量 `kernel_flag`；
> ② ⭐ 讲清 BKL 的**四个"超能力"**——递归、持锁可睡、`schedule()` 时自动放锁、
> 不受 lockdep 管辖——**每一条都和 Ch10 前面所有原语的语义相反**；
> ③ 逐行读 `lib/kernel_lock.c`（v2.6.38，最后一版完整实现），
> 包括 `CONFIG_PREEMPT` 下那段"**抢不到就开抢占轮询**"的奇葩逻辑；
> ④ 讲清 `current->lock_depth` 这个 `task_struct` 字段怎么实现递归；
> ⑤ 讲清 `release_kernel_lock()` / `reacquire_kernel_lock()` 这对
> **调度器侧的透明放锁**接口（这是 BKL 能"可睡眠"的真正机制）；
> ⑥ ⭐ **版本断崖：BKL 删于 v2.6.39，不是 v2.6.37**
> （原速查表与自测题里的"2.6.37"是错的，本篇给出三重证据链）；
> ⑦ `CONFIG_BKL` 与 `CONFIG_LOCK_KERNEL` 两个 Kconfig 的分工——
> 以及为什么在 **UP + 非抢占**内核上 `lock_kernel()` 会**编译成空**；
> ⑧ 给出**迁移对照表**：当年靠 BKL 的各种用法，今天分别该用什么。
>
> 所有代码与常量均核对自 jsdelivr 抓取的历史 tag 源码，行号可查。
> **本篇是 Ch10 唯一一篇"纯考古"的笔记**——但考古的价值在于：
> BKL 是被"可睡眠的锁"这四个字杀死的最著名案例，
> 而 2011 年之后 PREEMPT_RT 又把同一场仗重新打了一遍。

---

## 1. BKL 的本体：一把 `raw_spinlock_t`，仅此而已

先破除一个流传很广的误解：**BKL 不是什么特殊的数据结构，也不是信号量**。
它的全部实体就是 `lib/kernel_lock.c`（v2.6.38）：

```c
/*
 * The 'big kernel lock'
 *
 * This spinlock is taken and released recursively by lock_kernel()
 * and unlock_kernel().  It is transparently dropped and reacquired
 * over schedule().  It is used to protect legacy code that hasn't
 * been migrated to a proper locking design yet.
 *
 * Don't use in new code.
 */
static  __cacheline_aligned_in_smp DEFINE_RAW_SPINLOCK(kernel_flag);
```

（`lib/kernel_lock.c:v2.6.38:26`）

三个信息点：

| 观察 | 含义 |
|------|------|
| `raw_spinlock_t` | **不是** `spinlock_t`。这是 10.4 §1 那条规律的**第五个实例**：凡是要自己编排"放锁→睡→拿锁"序列的原语，内部锁一律 raw（关抢占/不睡）。BKL 也要跨 `schedule()` 释放，所以必须是 raw |
| `static` + 全局唯一 | 全内核**只有一把**。这是"BKL"里 "Big" 的字面意思，也是它一切问题的根源 |
| `__cacheline_aligned_in_smp` | 独占一个 cacheline，避免和别的全局变量伪共享。**这恰恰是承认了它是全局争用热点**——给一把"应该没人用"的锁做 cacheline 对齐，本身就是一种自嘲 |

注释里那句 **"It is transparently dropped and reacquired over schedule()"**
是理解 BKL 全部怪异语义的钥匙，见 §4。

---

## 2. ⭐ BKL 的四个"超能力"——每一条都和 Ch10 其它原语相反

把 BKL 和本篇笔记里学过的原语放一起看，会发现它**没有一条**符合正常锁的定义：

| 性质 | spinlock | mutex | semaphore | **BKL** |
|------|----------|-------|-----------|---------|
| **可递归？** | ❌ 自死锁 | ❌ 自死锁 | ❌ 自死锁 | ✅ **`lock_depth` 计数** |
| **持锁时可睡眠？** | ❌ | ✅ | ✅ | ✅ **（隐式，且是设计的一部分）** |
| **`schedule()` 时自动放锁？** | ❌（禁止调度） | ❌ | ❌ | ✅ **`release_kernel_lock()`** |
| **有 owner 概念？** | ❌ | ✅ | ❌ | ⚠️ 有（`current`），但**可跨任务转移** |
| **受 lockdep 管辖？** | ✅ | ✅ | ✅ | ❌ **明确排除** |
| **实例数量** | 任意 | 任意 | 任意 | **1** |

### 为什么这四条组合起来是灾难

单独看，"可递归"挺方便，"可睡眠"挺友好。合在一起就变成了：

```
进程 A:  lock_kernel()              /* depth 0→1，真加锁 */
         ... 进入某个旧驱动的 ioctl ...
         copy_from_user()           /* 缺页，睡眠 */
                 ↓
         schedule()                 /* 内核透明地 release_kernel_lock() */
                 ↓                  /* 此刻 A 手里【没有】BKL！ */
进程 B:  lock_kernel()              /* ✅ 拿到，高兴地进来了 */
         ... 进入【另一个】旧驱动的 ioctl ...
                 ↓
         它以为自己独占内核，实际上 A 还停在半路
```

这就是 BKL 最反直觉的地方：**"持有 BKL"根本不构成互斥保证**。
它保证的只是"不会有两个进程同时在**非睡眠状态**下执行 BKL 保护的代码"。
只要 A 睡了一觉，互斥就破了，而且这个破坏是**静默的**——
代码里完全看不出来。

> **对照现代内核**：这正是 PREEMPT_RT 把 `spinlock_t` 变成可睡眠的 `rt_mutex`
> 之后，社区立刻要求"**spinlock 保护的临界区里绝对不能睡眠**"的原因。
> 2011 年杀掉 BKL 的教训，2020 年代又完整地重演了一次。
> 见 10.2 §RT 一节与 10.5 §RT 一节。

---

## 3. 递归怎么实现：`current->lock_depth`

`lock_kernel()` / `unlock_kernel()` 只是包装（`include/linux/smp_lock.h` v2.6.38）：

```c
#define lock_kernel() do {					\
	_lock_kernel(__func__, __FILE__, __LINE__);		\
} while (0)

#define unlock_kernel()	do {					\
	_unlock_kernel(__func__, __FILE__, __LINE__);		\
} while (0)
```

真正的实现（v2.6.38 `lib/kernel_lock.c:119`）：

```c
void __lockfunc _lock_kernel(const char *func, const char *file, int line)
{
	int depth = current->lock_depth + 1;

	trace_lock_kernel(func, file, line);

	if (likely(!depth)) {
		might_sleep();
		__lock_kernel();
	}
	current->lock_depth = depth;
}

void __lockfunc _unlock_kernel(const char *func, const char *file, int line)
{
	BUG_ON(current->lock_depth < 0);
	if (likely(--current->lock_depth < 0))
		__unlock_kernel();

	trace_unlock_kernel(func, file, line);
}
```

### 状态机

`lock_depth` 初值 **-1**（未持锁），语义是"**嵌套层数减一**"：

| 调用 | `lock_depth` 变化 | 是否真的动 `kernel_flag` |
|------|------------------|------------------------|
| 第 1 次 `lock_kernel()` | -1 → 0 | ✅ `__lock_kernel()` 拿 spinlock |
| 第 2 次 `lock_kernel()` | 0 → 1 | ❌ 只加计数 |
| 第 3 次 `lock_kernel()` | 1 → 2 | ❌ 只加计数 |
| 第 1 次 `unlock_kernel()` | 2 → 1 | ❌ 只减计数 |
| 第 2 次 `unlock_kernel()` | 1 → 0 | ❌ 只减计数 |
| 第 3 次 `unlock_kernel()` | 0 → **-1** | ✅ `__unlock_kernel()` 放 spinlock |

判定条件 `if (likely(!depth))`：depth 为 0 才加锁。
判定条件 `if (likely(--current->lock_depth < 0))`：减到 -1 才放锁。

### 三个值得注意的细节

1. **`might_sleep()` 出现在加锁路径上**（`__lock_kernel()` 之前）。
   对一个"锁"来说这是极其罕见的信号——它在说"**拿到这把锁之后你可以睡**"。
   现代任何一把 `spinlock_t` 的加锁路径上都挂的是 `might_sleep()` 的
   **反面**（`CONFIG_DEBUG_ATOMIC_SLEEP` 下的 `___might_sleep()` 断言）。

2. **`BUG_ON(current->lock_depth < 0)`** —— 多放一次锁直接 panic。
   这是递归锁唯一的"保护"：**放多了会死，抢多了（别人）不会死**。

3. **`lock_depth` 是 per-task 的**，存在 `task_struct` 里：

   ```
   include/linux/sched.h:1200 (v2.6.38)
   	int lock_depth;		/* BKL lock depth */
   ```

   ⭐ 这意味着 **BKL 的"持有者"是记录在进程上的，不是记录在锁上的**。
   `kernel_flag` 这个 spinlock 本身**完全不知道谁持有它**。
   ——这就是 §4 里调度器能"偷偷把锁放掉"的前提。

---

## 4. ⭐ 调度器侧的透明放锁：`release/reacquire_kernel_lock()`

BKL "可睡眠"的真正机制不在 `lock_kernel()` 里，而在**调度器**里。
`include/linux/smp_lock.h`（v2.6.38）导出了两个给调度器专用的接口：

```c
#define release_kernel_lock(tsk) do { 		\
	if (unlikely((tsk)->lock_depth >= 0))	\
		__release_kernel_lock();	\
} while (0)

static inline int reacquire_kernel_lock(struct task_struct *task)
{
	if (unlikely(task->lock_depth >= 0))
		return __reacquire_kernel_lock();
	return 0;
}
```

实现（`lib/kernel_lock.c:42`）：

```c
int __lockfunc __reacquire_kernel_lock(void)
{
	while (!do_raw_spin_trylock(&kernel_flag)) {
		if (need_resched())
			return -EAGAIN;
		cpu_relax();
	}
	preempt_disable();
	return 0;
}

void __lockfunc __release_kernel_lock(void)
{
	do_raw_spin_unlock(&kernel_flag);
	preempt_enable_no_resched();
}
```

### 时序图

```
进程 A（持有 BKL，depth=0）
   │
   ├─ 调用 copy_from_user() → 缺页 → 准备睡眠
   │
   ▼
schedule()
   │
   ├─ release_kernel_lock(prev)      /* 见 lock_depth>=0 → 真的放掉 kernel_flag */
   │     └─ A->lock_depth 仍然是 0！ ← ⭐ 关键：计数不动，只是锁暂时不在手里
   │
   ├─ ... 切换到别的进程 B ...
   │     └─ B 这时可以 lock_kernel() 成功（见 §2 的灾难场景）
   │
   ├─ ... 切回 A ...
   │
   └─ reacquire_kernel_lock(prev)    /* 重新 trylock，失败返回 -EAGAIN → 再调度一次 */
```

⭐ **核心洞察：`lock_depth` 这个"持有凭证"和 `kernel_flag` 这个"实际锁"是会分离的。**
平时二者同步；调度时二者分离。所以：

> **`current->lock_depth >= 0` 只表示"我这个任务处在 BKL 保护区间里"，
> 完全不表示"我现在物理上握着那把 spinlock"。**

这条语义在今天的 v6.6 里**已经找不到任何对应物**——
现代内核里"持锁"永远是"物理上握着"。这也是为什么把 BKL 时代的
驱动代码直译成现代代码会出微妙的 bug。

---

## 5. `CONFIG_PREEMPT` 下那段"抢不到就开抢占轮询"的逻辑

`__lock_kernel()` 有两套实现（`lib/kernel_lock.c:64`）：

```c
#ifdef CONFIG_PREEMPT
static inline void __lock_kernel(void)
{
	preempt_disable();
	if (unlikely(!do_raw_spin_trylock(&kernel_flag))) {
		/*
		 * If preemption was disabled even before this
		 * was called, there's nothing we can be polite
		 * about - just spin.
		 */
		if (preempt_count() > 1) {
			do_raw_spin_lock(&kernel_flag);
			return;
		}

		/*
		 * Otherwise, let's wait for the kernel lock
		 * with preemption enabled..
		 */
		do {
			preempt_enable();
			while (raw_spin_is_locked(&kernel_flag))
				cpu_relax();
			preempt_disable();
		} while (!do_raw_spin_trylock(&kernel_flag));
	}
}

#else
static inline void __lock_kernel(void)
{
	do_raw_spin_lock(&kernel_flag);
}
#endif
```

### 两条分支的判据：`preempt_count() > 1`

| 情况 | 行为 | 为什么 |
|------|------|--------|
| `preempt_count() == 1`（只有自己刚加的那层） | **开抢占 + 轮询**（polite） | 说明调用者本来就可抢占，那么"等待 BKL"期间也应该允许被抢占——否则持锁者可能正等着在这个 CPU 上跑完 |
| `preempt_count() > 1`（调用者已经关了抢占/在原子上下文） | **死等**（`do_raw_spin_lock`） | 开抢占也没用（抢不掉），反而可能死锁。注释原话："there's nothing we can be polite about" |

**⭐ 这是一个"礼貌自旋"（polite spinning）的早期原型**，和今天
`mutex` 的乐观自旋（10.5 §OSQ）、qspinlock 的 pending 位是同一个思想族：
**等锁的时候别把 CPU 焊死，让持锁者有机会跑完。**
但它比现代实现粗糙得多——轮询循环里没有退避、没有 MCS 队列、没有公平性，
纯粹是 `while (locked) cpu_relax();`，在 8 核以上会引发严重的 cacheline 乒乓。

### 放锁侧刻意绕开 lockdep

```c
static inline void __unlock_kernel(void)
{
	/*
	 * the BKL is not covered by lockdep, so we open-code the
	 * unlocking sequence (and thus avoid the dep-chain ops):
	 */
	do_raw_spin_unlock(&kernel_flag);
	preempt_enable();
}
```

**为什么故意躲开 lockdep？** 因为它一旦被 lockdep 管辖，
递归加锁（`lock_kernel()` 套 `lock_kernel()`）会被立刻判定为
"**possible recursive locking detected**" 而刷屏告警。
BKL 的设计就建立在递归之上，只能让 lockdep 闭嘴。

> ⭐ **这条对今天的读者最有价值**：
> **"一把需要关掉死锁检测器才能用的锁，本身就是设计失败的证据。**
> 现代 `spinlock_t` / `mutex` 全部被 lockdep 完整覆盖，
> 递归加锁会立刻被抓住——这不是 lockdep 多事，是它替你发现了 bug。

---

## 6. 两个 Kconfig：`BKL` vs `LOCK_KERNEL`，以及在 UP 上编译成空

`init/Kconfig`（v2.6.38）：

```
config LOCK_KERNEL
	bool
	depends on (SMP || PREEMPT) && BKL
	default y
```

（`init/Kconfig:72`，v2.6.38）

配合 `lib/Makefile`：

```
obj-$(CONFIG_LOCK_KERNEL) += kernel_lock.o
```

（`lib/Makefile:46`，v2.6.37 与 v2.6.38 完全相同）

于是有**三层**编译行为（`include/linux/smp_lock.h` v2.6.38 的 `#ifdef` 结构）：

| 配置组合 | `CONFIG_LOCK_KERNEL` | `lock_kernel()` 展开成 |
|---------|---------------------|----------------------|
| `CONFIG_BKL=y` + (`SMP` 或 `PREEMPT`) | y | **真锁**（`_lock_kernel()`） |
| `CONFIG_BKL=y` + UP + 无抢占 | n（依赖不满足） | **空**（`do { } while(0)`） |
| `CONFIG_BKL=n` | n | **空**，且 `smp_lock.h` 里用 `#ifdef CONFIG_BKL /* provoke build bug if not set */` **故意制造编译错误** |

⭐ **最后一行最值得玩味**：

```c
#ifdef CONFIG_BKL /* provoke build bug if not set */
#define lock_kernel()
#define unlock_kernel()
#define cycle_kernel_lock()			do { } while(0)
#endif /* CONFIG_BKL */
```

如果 `CONFIG_BKL` 没开，头文件**故意不定义** `lock_kernel`。
这样任何还在调用它的（树外）代码会直接 **编译失败**，而不是静默变成空操作。
——这是当年逼迫树外驱动迁移的"硬手段"。

> **给今天的启示**：这就是内核处理"废弃 API"的标准做法之一。
> 同一手法在 v6.6 还能看到，比如 `down()` 被 kernel-doc 标记 deprecated
> （10.4 §deprecated）、`kmap_atomic()` 被移除（05 模块 Ch12.9）——
> **先警告、再制造编译错误、最后删文件**，三步走。

### 还有个空转 API：`cycle_kernel_lock()`

```c
/*
 * Various legacy drivers don't really need the BKL in a specific
 * function, but they *do* need to know that the BKL became available.
 * This function just avoids wrapping a bunch of lock/unlock pairs
 * around code which doesn't really need it.
 */
static inline void cycle_kernel_lock(void)
{
	lock_kernel();
	unlock_kernel();
}
```

作用：**拿一下立刻放**，纯粹为了"确保 BKL 此刻是可用的"（即没人被卡住）。
今天看来荒谬，但在当年它是给那些"open() 时要确认没有别的 BKL 用户卡死"的
旧驱动用的。这个 API 在 v6.6 世界里**没有任何对应物**——
现代内核里不存在"确认某把锁可用"这种操作。

---

## 7. ⭐ 版本断崖：BKL 删于 **v2.6.39**，不是 v2.6.37

> ⚠️ **原速查表与本页自测题 Q1 里写的"2.6.37 完全移除"是错误的**，
> 下面给出三重证据链。这是 Ch10 第 14 条"凭记忆必错"的事实。

### 证据链（全部实测，抓历史 tag 做字节数对比）

**证据 ①：`include/linux/smp_lock.h` 的存在性**

| tag | 字节数 | 判定 |
|-----|--------|------|
| v2.6.37 | 1637 | ✅ 存在 |
| v2.6.38 | 1637 | ✅ 存在（与 v2.6.37 **完全相同**，无改动） |
| **v2.6.39** | **77** | ❌ **404 残片**（`Couldn't find the requested file ...`） |
| v3.0 | 77 | ❌ 404 残片 |
| v6.6 | 77 | ❌ 404 残片 |

> ⚠️ 抓历史文件时**必须 `wc -c` 验证**。jsdelivr 对不存在的文件返回
> exit 0 + 一段 ~77 字节的错误文本当作文件内容。
> **<100 字节即 404 残片**，这是 05 模块一路用下来的标准判别法。

**证据 ②：`lib/kernel_lock.c` 的实现本体**

| tag | 字节数 | 判定 |
|-----|--------|------|
| v2.6.37 | 3272 | ✅ 完整实现 |
| v2.6.38 | 3272 | ✅ 完整实现 |
| **v2.6.39** | **70** | ❌ 404 残片 |
| v3.0 | 70 | ❌ 404 残片 |

**证据 ③：`task_struct.lock_depth` 字段**

```
include/linux/sched.h  (v2.6.38)  :1200:  int lock_depth;   /* BKL lock depth */
include/linux/sched.h  (v3.0)     :  (grep 结果为 0 次)
```

三个证据互相独立、结论一致：**v2.6.38 是最后一个带 BKL 的版本，
v2.6.39（2011 年 5 月）把它从内核树里彻底删掉了。**

### 为什么"2.6.37"这个错误说法流传这么广

合理推测（不装作确证）：v2.6.37 前后正是 **BKL 迁移收官**的密集期——
大量驱动在这个窗口被逐个改掉，`CONFIG_LOCK_KERNEL` 的默认状态和
"还有多少 BKL 用户"在那个版本发生了显著变化。
"最后一批用户被清掉"和"基础设施被删掉"是**两个不同的版本**，
很容易记混。**记基础设施删除点：v2.6.39。**

---

## 8. 迁移对照表：当年靠 BKL 做的事，今天该用什么

BKL 不是"被一个东西取代"，而是**按用途拆成了七八种机制**。
这也是它花了那么多年才删完的原因。

| 当年用 BKL 保护的东西 | 现代（v6.6）该用什么 | 为什么 |
|----------------------|---------------------|--------|
| 字符设备的 `open()` / `ioctl()` 串行化 | `struct mutex`，**放在设备结构体里**（per-device） | 粒度从"全内核"降到"单个设备" |
| 文件系统的 `super_block` 操作 | `inode->i_rwsem` / `sb->s_umount`（`struct rw_semaphore`） | 读多写少，用 rwsem |
| "这个模块还在被用吗" | `refcount_t` + `try_module_get()` | 根本不需要锁，是引用计数问题 |
| 简单的开/关标志 | `atomic_t` / `WRITE_ONCE` + 屏障 | 单个字长，无锁即可 |
| 驱动里的"全局配置" | 通常应该**改成 per-device**；真要全局就用 `static DEFINE_MUTEX()` | 先问"为什么要全局" |
| 遍历链表时防并发修改 | **RCU** | 读者完全无锁（Ch10 没讲 RCU，见 Ch17 / 06 模块） |
| "统计谁在用" | per-CPU 计数 + `percpu_counter` | 消除共享（05 模块 Ch12.10） |
| 冷路径上偶尔要全局互斥 | `static DEFINE_MUTEX()` | 冷路径不关心性能，mutex 可调试性最好（10.5 §13） |

⭐ **一句话总结迁移原则**：
**"先消除共享，再选锁"**——BKL 的思路是"反正都会冲突，那就一把锁搞定"；
现代思路是"**这块数据到底被谁共享？能不能不共享？**"
per-CPU、每设备、每文件、RCU 全都是在**减少共享**，而不是**优化锁**。

---

## HFT / 嵌入式关联

### BKL 的延迟画像：一个不可推理的尾延迟分布

对低延迟系统，BKL 的问题**不是"平均慢"，而是"完全无法推理"**：

| 延迟来源 | 是否可预测 | 说明 |
|---------|-----------|------|
| 拿不到锁要等多久 | ❌ | 持锁者可能睡在**任何一个**旧驱动的**任何一行**上 |
| 持锁者会不会睡 | ❌ | `might_sleep()` 是显式邀请；代码里看不出来 |
| 睡多久 | ❌ | 可能是磁盘 I/O（毫秒级）、可能是用户态缺页 |
| 会不会被抢占 | ❌ | `CONFIG_PREEMPT` 下持锁者随时被抢走，且**抢走后锁不在它手里**（§4） |
| 争用的是谁 | ❌ | 全内核任何路径，跨子系统 |

对比一下现代原语的延迟上界：

| 原语 | 延迟上界 | 可推理性 |
|------|---------|---------|
| `spinlock_t`（非 RT） | 临界区长度 × CPU 数 | ✅ 有界（临界区禁止睡眠 + 禁止抢占） |
| `mutex` | 一次调度往返（数 µs ~ 数十 µs） | ✅ 有界（10.5 §OSQ） |
| `completion` | 等待事件的时长 | ✅ 有界（你自己定义事件） |
| **BKL** | **无上界** | ❌ |

**⭐ 对 HFT 的通用教训（这才是本篇真正的价值）**：
BKL 死于"**锁的语义无法局部推理**"。
一个工程师要评估自己代码的延迟，却必须读完内核里**所有** BKL 用户——
这在工程上是不可能的，于是只能整体删掉。

> 同样的判据今天可以直接套用：
> **如果评估你代码的最坏延迟需要读别人的代码，那你的同步设计就完了。**

### 嵌入式：为什么 BKL 在单核上"看起来没问题"，移植到多核就炸

`CONFIG_LOCK_KERNEL` 的依赖关系 `depends on (SMP || PREEMPT) && BKL`
带来一个隐蔽的移植陷阱：

```
UP + 无抢占（大量老嵌入式配置）：
    CONFIG_LOCK_KERNEL = n
    → lock_kernel() 编译成空
    → 所有这些代码路径实际上【根本没加锁】

SMP 或多核（升级后）：
    CONFIG_LOCK_KERNEL = y
    → lock_kernel() 变成真锁
    → 突然开始串行化，性能雪崩；
       而那些"靠 BKL 顺带保护"的隐式不变量，有些会因为 §2 的睡眠语义【依然没保护】
```

**双向踩坑**：既可能"原来没锁现在有锁"导致性能崩，
也可能"以为有锁其实没有"导致数据竞争。
这类 bug 只在多核配置下出现，且极难复现。

> 今天的等价陷阱：**在 `CONFIG_PREEMPT_NONE` 上开发、在
> `CONFIG_PREEMPT_RT` 上部署**。锁的语义在两个配置下不同
> （`spinlock_t` 在 RT 上变成可睡眠的 `rt_mutex`），
> 而编译期不会报错。

### 观测手段（考古向）

当年 BKL 有专门的 tracepoint（`lib/kernel_lock.c` 里
`#define CREATE_TRACE_POINTS` + `#include <trace/events/bkl.h>`）：

| tracepoint | 字段 | 用途 |
|-----------|------|------|
| `lock_kernel` | `func`, `file`, `line` | **谁在哪里拿了 BKL**（定位热点） |
| `unlock_kernel` | `func`, `file`, `line` | 配对分析持锁时长 |

注意这对 tracepoint **记录了调用点的文件名和行号**——
这是"给迁移工作用的定位工具"，不是给性能分析用的。
一个需要专门 tracepoint 来找出"到底谁在用"的锁，
本身就说明它的使用范围已经失控了。

---

## 实践模板：见到旧代码里的 BKL 怎么迁

```c
/* ===== BKL 时代（v2.6.38 及以前）===== */
static int foo_ioctl(struct inode *inode, struct file *filp,
		     unsigned int cmd, unsigned long arg)
{
	lock_kernel();                 /* 全局串行化 */
	switch (cmd) {
	case FOO_SET:   foo->val = arg; break;
	case FOO_GET:   ret = foo->val; break;
	}
	unlock_kernel();
	return ret;
}
```

```c
/* ===== v6.6 的正确写法：锁跟着数据走 ===== */
struct foo_dev {
	int val;
	struct mutex lock;             /* ⭐ per-device */
};

static long foo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct foo_dev *foo = filp->private_data;
	int ret = 0;

	if (mutex_lock_interruptible(&foo->lock))   /* 10.5：驱动用可中断变体 */
		return -ERESTARTSYS;
	switch (cmd) {
	case FOO_SET:   foo->val = arg; break;
	case FOO_GET:   ret = foo->val; break;
	}
	mutex_unlock(&foo->lock);
	return ret;
}
```

**迁移时逐条自检**：

| # | 检查项 | 为什么 |
|---|--------|--------|
| 1 | BKL 在这里到底保护**哪个对象**？ | 多数时候原作者自己都说不清，要读调用路径 |
| 2 | 这个对象是全局的还是 per-device / per-file？ | 决定锁放在哪（**通常该 per-device**） |
| 3 | 临界区里有没有会睡眠的调用（`copy_*_user`、分配、I/O）？ | 有 → mutex/spinlock 二选一的分水岭 |
| 4 | 有没有递归依赖（`a()` 调 `b()`，两个都 `lock_kernel()`）？ | 有 → 必须重构，现代锁不可递归（10.5 明确禁止） |
| 5 | 迁移后有没有跨子系统的隐式假设？ | BKL 曾经顺带保护过的东西会暴露出来 |

---

## 易错点核对表

| # | 易错点 | 正确做法 |
|---|--------|---------|
| 1 | 说"BKL 在 2.6.37 被移除" | ❌ **v2.6.39**（§7 三重证据链） |
| 2 | 以为 BKL 是个信号量 | ❌ 它是 `raw_spinlock_t`，只有"可睡眠"是通过调度器放锁伪造出来的 |
| 3 | 以为持有 BKL 就有互斥 | ⚠️ 持锁者一睡，互斥就破了（§2 时序图） |
| 4 | 以为 `lock_depth >= 0` 表示握着 spinlock | ❌ 只表示"在 BKL 区间里"，调度期间二者分离（§4） |
| 5 | 以为 BKL 受 lockdep 保护 | ❌ 源码注释明说 "not covered by lockdep"，放锁是 open-code 的 |
| 6 | 以为 BKL 在 UP 上也串行化 | ❌ UP + 无抢占时 `CONFIG_LOCK_KERNEL=n`，**编译成空** |
| 7 | 以为 `cycle_kernel_lock()` 有用 | ❌ 它是"确认锁可用"的空转，现代内核无对应物 |
| 8 | 迁移时直接把 `lock_kernel()` 换成一把全局 mutex | ⚠️ 只是把"全局锁"换了个名字，粒度问题没解决 |
| 9 | 以为递归加锁能被现代锁支持 | ❌ `mutex` / `spinlock` 都不允许自递归，必须重构调用关系 |
| 10 | 以为 BKL 的"可睡眠"是优点 | ❌ 它正是不可推理尾延迟的根源 |

---

## 常见陷阱

1. 混淆 completion 和 semaphore——completion 是一次性通知，semaphore 可重复
2. 在 completion 的 `wait_for_completion()` 中以为会自旋——它会睡眠（进程上下文）
3. **（v6.6 补充 / 历史订正）** 说 BKL 在 v2.6.37 被删除——**v2.6.39**
4. **（v6.6 补充）** 以为 BKL 是信号量或 mutex——它是 `raw_spinlock_t` + 调度器放锁的复合体
5. **（v6.6 补充）** 以为"可递归 + 可睡眠"是便利特性——它使延迟上界消失
6. **（v6.6 补充）** 在 UP 无抢占配置上验证 BKL 相关逻辑——那里它编译成空
7. **（v6.6 补充）** 迁移时只做 `lock_kernel()` → 全局 mutex 的机械替换

---

## 自测题

<details>
<summary>自测题（点击展开）</summary>

**Q1.** BKL（Big Kernel Lock）是什么？为什么被移除？

<details><summary>答案</summary>

BKL 是 Linux 早期从 SMP 过渡时用的全局锁。特点：① 全局唯一（一把锁保护所有）。② 可睡眠（schedule 时自动释放，返回后重新获取）。③ 可递归（同一进程可多次 lock）。移除原因：① 全局锁是性能瓶颈（多核扩展性差）。② 可睡眠+自动释放导致语义复杂。③ 阻碍 PREEMPT_RT。2.6.37 完全移除，所有 BKL 用户改为 mutex/spinlock。

<details><summary>按 v6.6 修订/补充</summary>

**⚠️ 最后一句的版本号错了：不是 2.6.37，是 v2.6.39。**
三重证据（§7）：

| 证据 | v2.6.38 | v2.6.39 |
|------|---------|---------|
| `include/linux/smp_lock.h` | 1637 B 存在 | **77 B = 404 残片** |
| `lib/kernel_lock.c` | 3272 B 存在 | **70 B = 404 残片** |
| `task_struct.lock_depth` | `sched.h:1200` 有 | v3.0 起 grep 为 0 次 |

其余部分（①②③ 三条特点与三条移除原因）**全部成立**，而且
源码里能逐条找到对应实现：

- ① 全局唯一 → `static DEFINE_RAW_SPINLOCK(kernel_flag)`（`lib/kernel_lock.c:26`）
- ② 可睡眠 → `might_sleep()` 出现在 `_lock_kernel()` 里（`:127`），
  配合调度器的 `release_kernel_lock()` / `reacquire_kernel_lock()`
- ③ 可递归 → `current->lock_depth`，`:121` 的 `int depth = current->lock_depth + 1`

**再补三点原答案没提到的**：

1. **它是 `raw_spinlock_t`，不是信号量**。这是 10.4 §1 那条规律的第五个实例
   （要自己编排"放锁→睡→拿锁"的原语，内部锁一律 raw）。
2. **它明确不受 lockdep 管辖**——`__unlock_kernel()` 的注释写着
   "the BKL is not covered by lockdep, so we open-code the unlocking sequence"。
   因为它建立在递归之上，一上 lockdep 就会刷"recursive locking"告警。
3. **"可睡眠"的代价是互斥保证消失**：持锁者睡着时锁被调度器放掉了，
   别的 CPU 可以进来（§2 时序图）。所以 BKL 保护的代码在跨睡眠边界时
   **根本不互斥**——这是它最反直觉、也最致命的性质。

</details>
</details>

**Q2.** BKL 移除后，原来用 BKL 的代码改用了什么？

<details><summary>答案</summary>

每个子系统逐个迁移：① ioctl → per-file mutex。② 文件系统 → per-superblock lock。③ 驱动 → per-device mutex。迁移过程持续多个版本（2.6.26-2.6.37），通过 `lock_kernel()`/`unlock_kernel()` 标记 BKL 用户，逐个替换。迁移后 SMP 扩展性显著提升。

<details><summary>按 v6.6 修订/补充</summary>

**版本号区间要修正**：迁移的收官期确实在 v2.6.2x–v2.6.3x，
但**基础设施删除点是 v2.6.39**（2011 年 5 月），
所以"迁移过程"应该写成 **v2.6.26 – v2.6.39**。

**替代方案那三条要扩展**——BKL 不是被一个东西取代的，
而是**按用途拆成了七八种机制**（§8 完整表）。除了 mutex，还有：

| 用途 | 现代方案 |
|------|---------|
| "模块还在被用吗" | `refcount_t` + `try_module_get()`（**不是锁问题**） |
| 简单的开/关标志 | `atomic_t` / `WRITE_ONCE` + 屏障（**不需要锁**） |
| 遍历链表防并发修改 | **RCU**（读者无锁） |
| 全局统计 | per-CPU + `percpu_counter`（**消除共享**） |

⭐ **最关键的一条原则**：迁移的正确思路不是"换一把更好的锁"，
而是**先问"这块数据为什么要共享"**。
BKL 时代的默认答案是"反正都会冲突，一把锁搞定"；
现代答案是 per-device / per-file / per-CPU / RCU——
**全都是在减少共享，而不是优化锁**。

**另外补一个机制细节**：迁移能被推进，靠的是
`CONFIG_BKL` 这个开关的**硬手段**——关掉它之后
`smp_lock.h` **故意不定义** `lock_kernel()`
（注释写着 "provoke build bug if not set"），
让还在用的树外代码**编译失败**而不是静默变空操作。

</details>
</details>

**Q3.** BKL 的历史教训对 HFT 设计有什么启示？

<details><summary>答案</summary>

① 避免全局锁——用 per-thread/per-CPU 数据消除共享。② 可睡眠锁不是万能的——BKL 可睡眠但导致语义混乱。③ 锁的可扩展性比锁的正确性更难——BKL 是正确的但不可扩展。④ 逐步替换优于大重写——BKL 花了 5 年逐个迁移。HFT 设计：热路径无锁，冷路径细粒度锁。

<details><summary>按 v6.6 修订/补充</summary>

四条都成立，补一条**最有工程价值**的判据：

> ⭐ **BKL 真正死于"锁的语义无法局部推理"。**
> 一个工程师要评估自己代码的最坏延迟，却必须读完内核里**所有** BKL 用户——
> 因为持锁者可能睡在**任何**一个旧驱动的**任何**一行上（§HFT 关联的表）。
> 这在工程上做不到，于是只能整体删掉。

**这条判据今天可以直接套用**：
> **如果评估你代码的最坏延迟需要读别人的代码，那同步设计就失败了。**

具体到 HFT 系统的四条落地检查：

| 检查 | 方法 |
|------|------|
| 我的锁保护的范围能不能一屏看完？ | 临界区超过一屏 → 拆分 |
| 临界区里有没有任何可能睡眠的调用？ | 有 → 不该用 spinlock（10.2 §睡眠禁令） |
| 这把锁被几个模块共享？ | >1 个 → 问"能不能 per-实例" |
| 持锁最坏多久能不能**从我自己这段代码**算出来？ | 算不出 → 设计有问题 |

**第 ② 条要再强调一层**：BKL 的"可睡眠"和现代 mutex 的"可睡眠"
是**完全不同的两件事**：

| | BKL | `mutex`（v6.6） |
|--|-----|----------------|
| 睡眠方式 | **隐式**（`schedule()` 时自动放锁，代码里看不出来） | **显式**（`mutex_lock()` 就是睡眠点） |
| 睡醒后 | 锁不在手里，要 `reacquire` | 确定持有锁 |
| 互斥是否保持 | ❌ 睡眠期间互斥消失 | ✅ 全程保持 |
| lockdep 覆盖 | ❌ | ✅（9 条强制语义，10.5 §13） |

**区别是"隐式 vs 显式"**。mutex 也睡，但睡得明明白白、
睡醒后确定性持有锁、且被 lockdep 完整监控。
所以结论不是"可睡眠的锁不好"，而是"**隐式睡眠的锁是灾难**"。

</details>
</details>

**Q4.** （v6.6 新增）`current->lock_depth >= 0` 能不能推出"当前进程握着 `kernel_flag` 这把 spinlock"？

<details><summary>答案</summary>

**不能。** 这是 BKL 最反直觉的语义（§4）。

`lock_depth >= 0` 只表示"**这个任务处在 BKL 保护区间里**"，
与"物理上握着 spinlock"是**两件会分离的事**：

```
lock_kernel()          → lock_depth = 0，同时真拿了 kernel_flag
     ↓
copy_from_user() 缺页  → 睡眠
     ↓
schedule()             → release_kernel_lock():
                         do_raw_spin_unlock(&kernel_flag)   ← 锁没了
                         但 lock_depth 仍然是 0！           ← 凭证还在
     ↓
（此刻别的 CPU 可以 lock_kernel() 成功）
     ↓
切回本进程             → reacquire_kernel_lock():
                         do_raw_spin_trylock(&kernel_flag) ← 重新拿
```

⭐ **核心：`lock_depth`（持有凭证，存在 `task_struct` 里）与
`kernel_flag`（实际锁，全局变量）在调度期间是分离的。**

推论：**`release_kernel_lock()` 完全不修改 `lock_depth`**——
它只 `do_raw_spin_unlock()` + `preempt_enable_no_resched()`，
计数留给 `reacquire_kernel_lock()` 之后照旧使用。

这也解释了为什么 BKL 不可能被 lockdep 管辖：
**lockdep 的模型是"锁对象上有 owner"，而 BKL 的 owner 记录在进程上、
还会被调度器偷偷清掉**，这套模型 lockdep 表达不了。

</details>

**Q5.** （v6.6 新增）为什么 `__lock_kernel()` 在 `CONFIG_PREEMPT` 下要判断 `preempt_count() > 1`？

<details><summary>答案</summary>

因为它要在"**礼貌自旋**"和"**死等**"之间选一条路（§5）：

```c
preempt_disable();
if (unlikely(!do_raw_spin_trylock(&kernel_flag))) {
	if (preempt_count() > 1) {
		do_raw_spin_lock(&kernel_flag);   /* 死等 */
		return;
	}
	do {
		preempt_enable();                 /* 开抢占轮询 */
		while (raw_spin_is_locked(&kernel_flag))
			cpu_relax();
		preempt_disable();
	} while (!do_raw_spin_trylock(&kernel_flag));
}
```

| `preempt_count()` | 含义 | 选择 | 理由 |
|------------------|------|------|------|
| `== 1` | 只有自己刚加的那层，调用者本来就可抢占 | **开抢占 + 轮询** | 持锁者可能正等着在这个 CPU 上被调度跑完；焊死 CPU 反而让它跑不完 |
| `> 1` | 调用者已经关抢占 / 在原子上下文 | **死等** | 开抢占也没用（反正是不可抢占上下文），源码注释："there's nothing we can be polite about" |

⭐ **这是"礼貌自旋"的早期原型**，和 v6.6 里
`mutex` 的 OSQ 乐观自旋（10.5 §midpath）、qspinlock 的 pending 位（10.2）
属于同一思想族：**等锁时别把 CPU 焊死，给持锁者跑完的机会。**

但它比现代实现粗糙得多：

| | BKL 的礼貌自旋 | v6.6 mutex 的 OSQ |
|--|---------------|------------------|
| 排队 | ❌ 无，谁抢到算谁 | ✅ MCS 队列，FIFO |
| 退避 | ❌ 纯 `cpu_relax()` 忙轮询 | 有，且会退出自旋去睡眠 |
| cacheline 乒乓 | 严重（多核全盯同一个 `kernel_flag`） | 每个 CPU 转自己的 `osq_node` |
| 公平性 | ❌ 无 | ✅ |

所以"思想对了、实现错了"——正确的做法要等到 v4.x 的
qspinlock / OSQ 才真正落地。

</details>

</details>

---

→ [10.6 完成变量](./section-10.6-完成变量.md) · [10.8 顺序锁](./section-10.8-顺序锁.md) · [Ch 9 争用与可扩展性](../../chapter-09-kernel-sync-intro/notes/section-9.6-争用和可扩展性.md)

---
