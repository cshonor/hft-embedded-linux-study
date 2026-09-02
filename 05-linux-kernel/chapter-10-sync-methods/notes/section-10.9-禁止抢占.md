## ⑨ 禁止抢占 · Disabling Preemption

有时并不需要「锁住其他 CPU」，只需保证 **当前任务在本 CPU 上不被调度走** — 典型是保护 **per-CPU 数据**。

| API | 作用 |
|-----|------|
| **`preempt_disable()`** | 禁止内核抢占（可嵌套计数） |
| **`preempt_enable()`** | 恢复；可能立即调度 |
| **`get_cpu()` / `put_cpu()`** | 禁用抢占并返回 CPU 编号（防迁移） |

#### 为何 per-CPU 还要禁抢占

```
CPU0 上任务 A 操作 per_cpu(var, 0)
  │
  若被抢占迁移到 CPU1 再继续
  │
  ▼
  可能改到错误的 per-CPU 槽 / 假设被打破
```

| 手段 | 防什么 |
|------|--------|
| `preempt_disable` | 被调度到别的 CPU / 中间插入同 CPU 其他内核路径（视场景） |
| 再加 `local_irq_disable` | 连中断也不要打断（更重） |

#### 与锁的关系

| 场景 | 做法 |
|------|------|
| 数据真·每 CPU 一份、无跨 CPU 共享 | 常 **只需禁抢占**（或 `local_bh_disable`） |
| 跨 CPU 共享 | **自旋锁 / 原子** 等 |
| 既 per-CPU 又要防中断 | `local_irq_save` 或 `*_irq` 锁变体 |

**规则：** `preempt_disable` 区间必须 **短**；里面 **禁止睡眠**。

**HFT：** 用户态「绑核 + 不主动阻塞」近似减少迁移；内核驱动里统计计数用 `this_cpu_*` / per-CPU + 禁抢占是常规手法。乱禁抢占 = 调度延迟 ↑。

→ [Ch 4.5 抢占](../../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) · [12.10 per-CPU 分配](../../chapter-12-memory-management/notes/section-12.10-每个-CPU-的分配.md)

### 常见陷阱

1. 混淆 preempt_disable() 和 local_irq_disable()——前者只禁抢占，后者还禁中断
2. 以为 preempt_disable() 后不能被中断——可以被中断，但不能被调度
3. 在 preempt_disable() 区域做耗时操作——会延迟调度器，增加系统延迟

---

> **本篇分工**：上面速查表**原样保留**。本篇往下**不复述**"per-CPU 数据要禁抢占"这个常识，
> 只做十一件事，**全部用 v6.6 源码实证**（`include/linux/preempt.h`、
> `arch/x86/include/asm/preempt.h`、`arch/x86/include/asm/current.h`、
> `kernel/Kconfig.preempt`）：
>
> ① ⭐ 拆开 `preempt_count` 的**位布局**——一个 32 位字里塞了**四个计数器**
> （preempt / softirq / hardirq / NMI）+ 1 个标志位；
> ② ⭐ **版本断崖：v6.6 x86 上 `preempt_count` 住在 per-CPU 的 `pcpu_hot` 里，
> 不在 `thread_info`**（`pcpu_hot` 引入于 **v6.2**），而且它和 `current`
> 挤在**同一个 64 字节 cacheline** 里（`static_assert(sizeof == 64)`）；
> ③ `preempt_disable()` / `preempt_enable()` 的真实展开，以及 `barrier()` 为什么
> 一个在**自增之后**、一个在**自减之前**；
> ④ ⭐ **x86 专属的 `PREEMPT_NEED_RESCHED` 反相技巧**——把 NEED_RESCHED 塞进
> MSB 并**取反**，使得"减一并判断"能压成**一条 `decl` 指令**；
> ⑤ 上下文判断全家桶：`preemptible()` / `in_atomic()` / `in_task()` /
> `interrupt_context_level()`，以及 ⚠️ **`in_atomic()` 的官方禁用警告**
> 和三个已废弃宏；
> ⑥ ⭐ **四个抢占模型 + 三个自动符号**的关系表（`PREEMPT_NONE` / `VOLUNTARY` /
> `PREEMPT` / `PREEMPT_RT` × `PREEMPT_COUNT` / `PREEMPTION` / `PREEMPT_DYNAMIC`）；
> ⑦ ⚠️ **`CONFIG_PREEMPT_COUNT=n` 时 `preempt_disable()` 就是 `barrier()`**——
> 这对"禁抢占到底有没有用"是决定性的一条；
> ⑧ 嵌套/配对变体：`_notrace`、`_no_resched`，以及 ⭐ **模块里被 `#undef` 掉**
> 的那几个（注释："Modules have no business playing preemption tricks"）；
> ⑨ ⭐ v6.6 的 **cleanup 式写法** `guard(preempt)` / `guard(migrate)`；
> ⑩ ⭐ **`preempt_disable_nested()`** —— 它的 kernel-doc **点名了 seqcount 写侧**
> （与 10.8 §9 约束②直接呼应）；
> ⑪ `migrate_disable()`：那段 40 行注释讲清了"为什么不推荐"和"RT 为什么非要它"。
>
> 所有常量与代码均核对自缓存的 v6.6 源码，行号可查。

---

## 1. ⭐ `preempt_count` 是一个字里塞了四个计数器

`include/linux/preempt.h:14-53`（v6.6）的注释块就是一张图：

```
 bit  31        24        20        16         8         0
      ┌──────────┬─────────┬─────────┬──────────┬─────────┐
      │ N R S C  │  NMI    │ HARDIRQ │  SOFTIRQ │ PREEMPT │
      │ (1 bit)  │ (4 bit) │ (4 bit) │ (8 bit)  │ (8 bit) │
      └──────────┴─────────┴─────────┴──────────┴─────────┘
       ↑           位 20-23   位 16-19   位 8-15     位 0-7
       │
       PREEMPT_NEED_RESCHED 0x80000000
       （x86 专属，且是【反相】的，见 §4）
```

| 宏 | 值 | 位数 | 最大嵌套深度 |
|----|----|------|------------|
| `PREEMPT_MASK` | `0x000000ff` | 8 | 256 层 `preempt_disable()` |
| `SOFTIRQ_MASK` | `0x0000ff00` | 8 | 256 层 softirq |
| `HARDIRQ_MASK` | `0x000f0000` | 4 | 16 层硬中断 |
| `NMI_MASK` | `0x00f00000` | 4 | 16 层 NMI |
| `PREEMPT_NEED_RESCHED` | `0x80000000` | 1 | — |

源码注释对位宽的取舍讲得很具体（`:21-25`）：

> "The hardirq count could in theory be the same as the number of
> interrupts in the system, but we run all interrupt handlers with
> interrupts disabled, so we cannot have nesting interrupts. Though
> there are a few **palaeontologic drivers** which reenable interrupts in
> the handler, so we need more than one bit here."

（"化石级驱动"——内核注释少见地带了点情绪。**HARDIRQ 只给 4 位而不是 1 位，
就是为了容忍这些在中断处理里自己开中断的老驱动。**）

### 各层的"1 个单位"

```c
#define PREEMPT_OFFSET	(1UL << PREEMPT_SHIFT)      /* 0x00000100? 不 —— 是 1 */
#define SOFTIRQ_OFFSET	(1UL << SOFTIRQ_SHIFT)      /* 0x00000100 */
#define HARDIRQ_OFFSET	(1UL << HARDIRQ_SHIFT)      /* 0x00010000 */
#define NMI_OFFSET	(1UL << NMI_SHIFT)          /* 0x00100000 */
#define SOFTIRQ_DISABLE_OFFSET	(2 * SOFTIRQ_OFFSET) /* ⭐ = 0x200 */
```

⭐ **`SOFTIRQ_DISABLE_OFFSET` 是 `SOFTIRQ_OFFSET` 的 **2 倍** —— 这是
`local_bh_disable()` 用的偏移量，多出来的那一位用来区分
"**我在 softirq 里**"（1 个单位）和"**我只是禁了下半部**"（2 个单位）。
所以 `in_serving_softirq()` 才能写成 `softirq_count() & SOFTIRQ_OFFSET`。

### 由此派生的上下文判断

```c
#define nmi_count()		(preempt_count() & NMI_MASK)
#define hardirq_count()		(preempt_count() & HARDIRQ_MASK)
#define softirq_count()		(preempt_count() & SOFTIRQ_MASK)     /* 非 RT */

#define in_nmi()		(nmi_count())
#define in_hardirq()		(hardirq_count())
#define in_serving_softirq()	(softirq_count() & SOFTIRQ_OFFSET)
#define in_task()		(!(in_nmi() | in_hardirq() | in_serving_softirq()))
```

还有一个把四层压成 0~3 级的小工具（`:90`）：

```c
static __always_inline unsigned char interrupt_context_level(void)
{
	unsigned long pc = preempt_count();
	unsigned char level = 0;
	level += !!(pc & (NMI_MASK));
	level += !!(pc & (NMI_MASK | HARDIRQ_MASK));
	level += !!(pc & (NMI_MASK | HARDIRQ_MASK | SOFTIRQ_OFFSET));
	return level;
}
```

| 返回值 | 含义 |
|--------|------|
| 0 | 普通上下文（进程上下文，可睡眠） |
| 1 | softirq 上下文 |
| 2 | hardirq 上下文 |
| 3 | NMI 上下文 |

⚠️ **RT 上一个特例**（`:104`）：

```c
#ifdef CONFIG_PREEMPT_RT
# define softirq_count()	(current->softirq_disable_cnt & SOFTIRQ_MASK)
#else
# define softirq_count()	(preempt_count() & SOFTIRQ_MASK)
#endif
```

**RT 上 softirq 计数搬到了 `task_struct` 里**（因为 softirq 在 RT 上是可抢占的线程化执行，
不再是"在当前栈上嵌套"的模型）。所以**同一个 `softirq_count()` 在两个配置下读的是不同的地方**。

---

## 2. ⭐ 版本断崖：`preempt_count` 在 v6.2 搬进了 `pcpu_hot`

### v6.6 x86 的实际实现

```c
static __always_inline int preempt_count(void)          /* arch/x86/include/asm/preempt.h:25 */
{
	return raw_cpu_read_4(pcpu_hot.preempt_count) & ~PREEMPT_NEED_RESCHED;
}
```

`pcpu_hot` 是什么（`arch/x86/include/asm/current.h:14`，v6.6）：

```c
struct pcpu_hot {
	union {
		struct {
			struct task_struct	*current_task;
			int			preempt_count;
			int			cpu_number;
#ifdef CONFIG_CALL_DEPTH_TRACKING
			u64			call_depth;
#endif
			unsigned long		top_of_stack;
			void			*hardirq_stack_ptr;
			u16			softirq_pending;
#ifdef CONFIG_X86_64
			bool			hardirq_stack_inuse;
#else
			void			*softirq_stack_ptr;
#endif
		};
		u8	pad[64];
	};
};
static_assert(sizeof(struct pcpu_hot) == 64);            /* ⭐ 恰好一个 cacheline */
DECLARE_PER_CPU_ALIGNED(struct pcpu_hot, pcpu_hot);
```

⭐ **三个信息点**：

1. **`current`、`preempt_count`、`cpu_number`、`top_of_stack` 全部挤在
   同一个 64 字节 cacheline 里**（`static_assert(sizeof == 64)` + `PER_CPU_ALIGNED`）。
   这是刻意的：**内核最频繁访问的东西（`current`、`preempt_count()`、
   `smp_processor_id()`、取内核栈）全部命中同一个 cacheline，一次缓存未命中全部拿到。**
2. `preempt_count` 在里面的**偏移量并不固定**——它跟在 `current_task`（8 字节）后面，
   所以 x86-64 上是偏移 8。
3. **它现在是 `int`（有符号）**，不再是 `unsigned`。

### 版本断崖（抓多版本同名文件对比）

| 版本 | `arch/x86/include/asm/current.h` | `pcpu_hot` 出现次数 | 说明 |
|------|--------------------------------|-------------------|------|
| v5.15 | 443 B | 0 | 老实现 |
| v6.0 | 443 B | 0 | 老实现 |
| **v6.1** | 443 B | **0** | ⭐ 最后一个老版本 |
| **v6.2** | **916 B** | **4** | ⭐ **`pcpu_hot` 引入** |
| v6.3 | 916 B | 4 | |
| v6.6 | 916 B | 4 | 与 v6.2 一致 |

v6.1 的 `get_current()` 长这样：

```c
DECLARE_PER_CPU(struct task_struct *, current_task);

static __always_inline struct task_struct *get_current(void)
{
	return this_cpu_read_stable(current_task);
}
```

——只有一个独立的 per-CPU 指针。**v6.2 把它和 preempt_count 等合并成了一个 cacheline 结构体。**

> 📌 **对读者的意义**：中文资料和老书（包括 LKD3rd）讲的都是
> "`preempt_count` 在 `thread_info` 里"。**在 v6.6 x86 上这已经是错的。**
> 准确的说：`thread_info` 版本仍然存在于**通用实现**（`asm-generic/preempt.h`）：
> ```c
> return READ_ONCE(current_thread_info()->preempt_count);
> ```
> 所以**这个知识点是架构相关的**：x86 用 `pcpu_hot`，其他架构走通用 `thread_info`。

### 通用实现 vs x86 实现

| | 通用（`asm-generic/preempt.h`） | x86（`asm/preempt.h`） |
|--|------------------------------|----------------------|
| 存储位置 | `current_thread_info()->preempt_count` | per-CPU `pcpu_hot.preempt_count` |
| `PREEMPT_ENABLED` | **0** | `0 + PREEMPT_NEED_RESCHED`（即 `0x80000000`） |
| NEED_RESCHED 标志 | ❌ 没有（在 `thread_info` flags 里） | ✅ 塞进 MSB，**且反相** |
| `set_preempt_need_resched()` | **空函数** | `raw_cpu_and_4(..., ~PREEMPT_NEED_RESCHED)` |

---

## 3. `preempt_disable()` / `preempt_enable()` 的真实展开

```c
/* include/linux/preempt.h:202 */
#define preempt_disable() \
do { \
	preempt_count_inc(); \
	barrier(); \
} while (0)

/* :219，CONFIG_PREEMPTION 下 */
#define preempt_enable() \
do { \
	barrier(); \
	if (unlikely(preempt_count_dec_and_test())) \
		__preempt_schedule(); \
} while (0)

/* :240，!CONFIG_PREEMPTION 下 */
#define preempt_enable() \
do { \
	barrier(); \
	preempt_count_dec(); \
} while (0)
```

### ⭐ `barrier()` 的位置为什么是"一后一前"

| 调用 | `barrier()` 位置 | 作用 |
|------|-----------------|------|
| `preempt_disable()` | **自增之后** | 阻止编译器把临界区内的代码**上移**到自增之前（那样就不受保护了） |
| `preempt_enable()` | **自减之前** | 阻止编译器把临界区内的代码**下移**到自减之后 |

这是标准的"临界区不能被编译器挪出去"技巧，和 10.10 要讲的
`barrier()`（仅编译屏障，不生成任何指令）是同一个东西。

### `preempt_enable()` 有两个版本，取决于 `CONFIG_PREEMPTION`

| 配置 | `preempt_enable()` 会不会立即调度 |
|------|--------------------------------|
| `CONFIG_PREEMPTION=y`（`PREEMPT` / `PREEMPT_RT` 模型） | ✅ 会：`preempt_count_dec_and_test()` → `__preempt_schedule()` |
| `CONFIG_PREEMPTION=n`（`PREEMPT_NONE` / `VOLUNTARY`） | ❌ 不会：只 `preempt_count_dec()` |

⭐ **含义**：在 `PREEMPT_NONE` 内核上，`preempt_enable()` **只是一个
`barrier()` + 递减**，从不触发调度。所以在"服务器"配置的内核里，
`preempt_disable()` 区域的真实作用只剩**防止迁移和防止被显式抢占点调度**，
而不是"防止被随时抢占"。

---

## 4. ⭐ x86 的 `PREEMPT_NEED_RESCHED` 反相技巧

`arch/x86/include/asm/preempt.h:12-28`：

```c
/* We use the MSB mostly because its available */
#define PREEMPT_NEED_RESCHED	0x80000000

/*
 * We use the PREEMPT_NEED_RESCHED bit as an inverted NEED_RESCHED such
 * that a decrement hitting 0 means we can and should reschedule.
 */
#define PREEMPT_ENABLED	(0 + PREEMPT_NEED_RESCHED)

/*
 * We mask the PREEMPT_NEED_RESCHED bit so as not to confuse all current users
 * that think a non-zero value indicates we cannot preempt.
 */
static __always_inline int preempt_count(void)
{
	return raw_cpu_read_4(pcpu_hot.preempt_count) & ~PREEMPT_NEED_RESCHED;
}
```

### 三个设计点

**① 位是反相的**：

```c
static __always_inline void set_preempt_need_resched(void)
{
	raw_cpu_and_4(pcpu_hot.preempt_count, ~PREEMPT_NEED_RESCHED);   /* ⭐ 清位 = 需要调度 */
}

static __always_inline void clear_preempt_need_resched(void)
{
	raw_cpu_or_4(pcpu_hot.preempt_count, PREEMPT_NEED_RESCHED);     /* ⭐ 置位 = 不需要 */
}
```

反相之后，"**需要调度**"状态 = 位为 **0**。于是：

```
"preempt_count 的计数值" 与 "NEED_RESCHED 位" 可以被【一次减法】同时判定：

  decl pcpu_hot.preempt_count
      → 结果 == 0   ⟺  计数值归零（可抢占） 且 NEED_RESCHED 位为 0（需要调度）
```

**② 因此"减一并测试"能压成一条指令**（`asm/preempt.h:93`）：

```c
static __always_inline bool __preempt_count_dec_and_test(void)
{
	return GEN_UNARY_RMWcc("decl", pcpu_hot.preempt_count, e,
			       __percpu_arg([var]));
}
```

`GEN_UNARY_RMWcc` 生成 `decl` + 检查 ZF 标志——**一条指令完成"减一 + 置标志"**，
`preempt_enable()` 的快路径就是 `barrier(); decl; je`。

**③ `preempt_count()` 对外要**掩掉**这个位**：

```c
	return raw_cpu_read_4(pcpu_hot.preempt_count) & ~PREEMPT_NEED_RESCHED;
```

注释解释了原因：**"not to confuse all current users that think a non-zero
value indicates we cannot preempt"** —— 历史上所有把 `preempt_count()` 当
"计数值"用的代码（比如 `in_atomic()`）都假设"非 0 就不可抢占"，
如果不掩掉 MSB，这些代码在 x86 上会**永远看到非 0**。

### v6.6 的 `should_resched()`：一次等值比较判两件事

```c
static __always_inline bool should_resched(int preempt_offset)      /* :102 */
{
	return unlikely(raw_cpu_read_4(pcpu_hot.preempt_count) == preempt_offset);
}
```

⭐ 注意它是**等值比较**而不是位测试。`preempt_count_dec_and_test()` 展开成
（非 DEBUG/TRACE 配置下）：

```c
#define preempt_count_dec_and_test() __preempt_count_dec_and_test()
```

而 `CONFIG_DEBUG_PREEMPT` 或 `CONFIG_TRACE_PREEMPT_TOGGLE` 打开时（`:183`）：

```c
extern void preempt_count_add(int val);
extern void preempt_count_sub(int val);
#define preempt_count_dec_and_test() \
	({ preempt_count_sub(1); should_resched(0); })
```

——**变成函数调用 + 等值比较**，方便插桩（`CONFIG_DEBUG_PREEMPT` 会检查
"你是不是在 `preempt_disable()` 区域里调了会睡眠的函数"）。

> 📌 **排障提示**：如果你的内核开了 `CONFIG_DEBUG_PREEMPT`，
> `preempt_disable()` / `preempt_enable()` 的开销会明显变大（变成函数调用）。
> **量测 `preempt_disable()` 临界区的延迟时，先看 `.config`。**

---

## 5. 上下文判断全家桶 + ⚠️ `in_atomic()` 的官方禁用警告

| API | 定义 | 含义 |
|-----|------|------|
| `preemptible()` | `preempt_count() == 0 && !irqs_disabled()`（`:216`） | **能不能睡眠**的正规判据 |
| `in_atomic()` | `preempt_count() != 0`（`:175`） | ⚠️ 见下 |
| `in_atomic_preempt_off()` | `preempt_count() != PREEMPT_DISABLE_OFFSET`（`:181`） | 调度器内部用：判断"禁抢占之前是不是已经原子" |
| `in_task()` | `!(in_nmi() \| in_hardirq() \| in_serving_softirq())` | 在进程上下文 |
| `interrupt_context_level()` | 0~3 | §1 的表 |

### ⚠️ `in_atomic()` 的警告（源码原文，`:168`）

```c
/*
 * Are we running in atomic context?  WARNING: this macro cannot
 * always detect atomic context; in particular, it cannot know about
 * held spinlocks in non-preemptible kernels.  Thus it should not be
 * used in the general case to determine whether sleeping is possible.
 * Do not use in_atomic() in driver code.
 */
#define in_atomic()	(preempt_count() != 0)
```

三条要点：

1. **在不可抢占内核（`PREEMPT_COUNT=n`）上，spinlock 根本不改 `preempt_count`**
   → 持有 spinlock 时 `in_atomic()` 却返回 **false**（伪阴性）。
2. 官方结论："**it should not be used in the general case to determine
   whether sleeping is possible**"。
3. 最后一句是命令式的："**Do not use `in_atomic()` in driver code.**"

> **那该用什么？** 想判断"能不能睡眠" → 用 `might_sleep()` **断言**（让内核替你检查），
> 而不是自己 `if (in_atomic())` 分支。
> 想判断"在不在这个上下文" → 用 `in_task()` / `in_hardirq()` / `in_nmi()` / `in_serving_softirq()` 这些**特异性更强**的。

### 三个已废弃的宏（源码标注，`:124`）

```c
/*
 * The following macros are deprecated and should not be used in new code:
 * in_irq()       - Obsolete version of in_hardirq()
 * in_softirq()   - We have BH disabled, or are processing softirqs
 * in_interrupt() - We're in NMI,IRQ,SoftIRQ context or have BH disabled
 */
#define in_irq()		(hardirq_count())
#define in_softirq()		(softirq_count())
#define in_interrupt()		(irq_count())
```

| 废弃 | 替代 | 差别 |
|------|------|------|
| `in_irq()` | `in_hardirq()` | 完全等价，只是改名 |
| `in_softirq()` | `in_serving_softirq()` | ⭐ **语义不同**！`in_softirq()` 在"**只是 `local_bh_disable()` 了**"时也返回真；`in_serving_softirq()` 只在**真的在处理 softirq** 时返回真 |
| `in_interrupt()` | 组合 `in_nmi()\|in_hardirq()\|in_serving_softirq()` | 同上 |

⭐ **`in_softirq()` 与 `in_serving_softirq()` 的差别就是 §1 里
`SOFTIRQ_DISABLE_OFFSET = 2 * SOFTIRQ_OFFSET` 那多出来的一位**。
从"禁了 BH"里区分出"正在跑 softirq"，靠的就是它。

---

## 6. ⭐ 四个抢占模型 × 三个自动符号

`kernel/Kconfig.preempt`（v6.6）的 `choice` 里有 **4 个模型**：

| Kconfig | 提示串 | 说明 |
|---------|--------|------|
| `PREEMPT_NONE` | "No Forced Preemption (**Server**)" | 吞吐优先，**默认项**（`default PREEMPT_NONE`） |
| `PREEMPT_VOLUNTARY` | "Voluntary Kernel Preemption (Desktop)" | 加更多**显式抢占点** |
| `PREEMPT` | "Preemptible Kernel (Low-Latency Desktop)" | 内核全可抢占；`select PREEMPT_BUILD` |
| `PREEMPT_RT` | "Fully Preemptible Kernel (Real-Time)" | `depends on EXPERT && ARCH_SUPPORTS_RT` |

外加三个**自动 / 间接**符号：

| 符号 | 谁选中它 | 作用 |
|------|---------|------|
| `PREEMPT_COUNT` | 被 `PREEMPTION` 选中（`:94`） | ⭐ 决定 **`preempt_count` 是否真的存在** |
| `PREEMPTION` | 被 `PREEMPT_BUILD` 和 `PREEMPT_RT` 选中 | 决定 `preempt_enable()` 会不会真的去调度 |
| `PREEMPT_DYNAMIC` | 用户选；`depends on HAVE_PREEMPT_DYNAMIC && !PREEMPT_RT` | 启动时用内核参数覆盖抢占模型（static call 实现） |

关系图：

```
choice "Preemption Model"
  ├── PREEMPT_NONE        ── select PREEMPT_NONE_BUILD if !PREEMPT_DYNAMIC
  ├── PREEMPT_VOLUNTARY   ── select PREEMPT_VOLUNTARY_BUILD if !PREEMPT_DYNAMIC
  ├── PREEMPT             ── select PREEMPT_BUILD ──┐
  └── PREEMPT_RT          ── select PREEMPTION ─────┤
                                                     │
  PREEMPT_BUILD ── select PREEMPTION ────────────────┤
  PREEMPT_RT    ── select PREEMPTION ────────────────┤
                                                     ▼
                                            PREEMPTION ── select PREEMPT_COUNT
```

⭐ **关键分界是 `PREEMPT_COUNT`，不是那四个模型**：

| `PREEMPT_COUNT` | `preempt_disable()` 展开成 | 含义 |
|----------------|--------------------------|------|
| **y**（`PREEMPT` / `RT` / 或 `PREEMPT_DYNAMIC`） | `preempt_count_inc(); barrier();` | 真的有计数器 |
| **n**（`PREEMPT_NONE` / `VOLUNTARY`） | **`barrier()`** | ⭐ 什么都没做！ |

源码（`:267`）：

```c
#else /* !CONFIG_PREEMPT_COUNT */
/*
 * Even if we don't have any preemption, we need preempt disable/enable
 * to be barriers, so that we don't have things like get_user/put_user
 * that can cause faults and scheduling migrate into our preempt-protected
 * region.
 */
#define preempt_disable()			barrier()
#define sched_preempt_enable_no_resched()	barrier()
#define preempt_enable_no_resched()		barrier()
#define preempt_enable()			barrier()
#define preempt_check_resched()			do { } while(0)
#define preemptible()				0
#endif
```

⚠️ **两条反直觉的推论**：

1. **在 `PREEMPT_NONE` 内核上，`preempt_disable()` 唯一的作用是 `barrier()`**
   ——它既不阻止抢占（本来就不会被强制抢占），也不阻止迁移。
   注释里点明了它**为什么还要存在**：防止 `get_user()/put_user()`
   这种"会缺页、会调度"的东西被编译器挪进"临界区"。
2. **`preemptible()` 在 `PREEMPT_COUNT=n` 时恒为 0**（永远返回 false）。
   所以拿 `preemptible()` 做分支的代码在这种配置下会**走进另一条路**。

### `PREEMPT_DYNAMIC`：一个内核二进制，四种行为

```c
#ifdef CONFIG_PREEMPT_DYNAMIC
DECLARE_STATIC_CALL(preempt_schedule, preempt_schedule_dynamic_enabled);
#define __preempt_schedule() \
do { \
	__STATIC_CALL_MOD_ADDRESSABLE(preempt_schedule); \
	asm volatile ("call " STATIC_CALL_TRAMP_STR(preempt_schedule) : ASM_CALL_CONSTRAINT); \
} while (0)
#endif
```

`preempt_enable()` 里的调度调用走 **static call**（静态调用），
启动时可以通过内核命令行切换成 `none` / `voluntary` / `full`。
发行版用这个来"一个内核包覆盖服务器和桌面两种场景"。

| 启动参数 | 效果 |
|---------|------|
| `preempt=none` | 吞吐优先 |
| `preempt=voluntary` | 桌面 |
| `preempt=full` | 低延迟 |

⚠️ **但 `PREEMPT_DYNAMIC` 依赖 `!PREEMPT_RT`** —— RT 是编译期决定的，不能启动时切。

---

## 7. 嵌套、配对，以及模块里被 `#undef` 掉的那几个

### 嵌套是设计支持的

`PREEMPT_MASK` 有 8 位 → **最多嵌套 256 层**。所以 `preempt_disable()`
可以随便套，`preempt_enable()` 逐一减回去，只有减到 0 才调度。

> ⚠️ 但**必须严格配对**。多调一次 `preempt_enable()` 会让计数变成负数，
> `preempt_count() != 0` 就永远成立 → **该 CPU 上的任务永远不会被抢占**。
> 这类 bug 表现为"系统跑着跑着某个核卡死"，极难定位。
> 用 §9 的 `guard(preempt)` 可以彻底避免。

### 变体表

| API | 特点 |
|-----|------|
| `preempt_disable()` / `preempt_enable()` | 标准版 |
| `preempt_disable_notrace()` / `preempt_enable_notrace()` | 不产生 tracepoint（tracing 代码内部用，避免递归） |
| `preempt_enable_no_resched()` | ⭐ 减计数但**不检查 need_resched**，不立即调度 |
| `preempt_check_resched()` | 单独补一次检查（配合 `_no_resched` 用） |

`preempt_enable_no_resched()` 的用途：你已经确定后面马上会有一个
**自然抢占点**（比如紧接着就要 `spin_unlock()` 或 `local_bh_enable()`），
那就没必要在这里多调度一次——**减少一次无谓的上下文切换**。

### ⭐ 模块里这些 API 被**故意取消**

```c
#ifdef MODULE
/*
 * Modules have no business playing preemption tricks.
 */
#undef sched_preempt_enable_no_resched
#undef preempt_enable_no_resched
#undef preempt_enable_no_resched_notrace
#undef preempt_check_resched
#endif
```

（`include/linux/preempt.h:288`）

⭐ 在编译**模块**时，这四个宏被 `#undef` 掉 —— 模块代码里用它们会
**编译失败**（或者更糟：如果某个头恰好又定义了，行为就不一致）。
注释的措辞很硬："**Modules have no business playing preemption tricks.**"

> **为什么**：`preempt_enable_no_resched()` 的正确性依赖"调用者知道
> 后面马上会有抢占点"这个**全局假设**，而模块作者通常没有这个全局视野。
> 内核宁可在编译期禁掉，也不愿在运行时收 bug report。

---

## 8. ⭐ v6.6 的现代写法：`guard(preempt)` / `guard(migrate)`

`include/linux/preempt.h:467`：

```c
DEFINE_LOCK_GUARD_0(preempt, preempt_disable(), preempt_enable())
DEFINE_LOCK_GUARD_0(preempt_notrace, preempt_disable_notrace(), preempt_enable_notrace())
DEFINE_LOCK_GUARD_0(migrate, migrate_disable(), migrate_enable())
```

配合 `linux/cleanup.h` 的 `guard()` 宏（依赖 GCC/Clang 的
`__attribute__((cleanup(...)))`），可以写成：

```c
void foo(void)
{
	guard(preempt)();                  /* 等价于 preempt_disable() */

	this_cpu_inc(my_stat);
	/* ... 任意 return / 提前退出 / 异常路径 ... */

}                                      /* ⭐ 作用域结束自动 preempt_enable() */
```

| 传统写法 | `guard()` 写法 |
|---------|---------------|
| 每个 `return` 前都要记得 `preempt_enable()` | **编译器保证** |
| `goto out;` 路径容易漏 | 不会漏 |
| 配对错了 → 计数变负 → CPU 卡死 | 不可能 |

⭐ **v6.6 里 `DEFINE_LOCK_GUARD_0` 已经为 preempt / migrate / 各种 spinlock /
mutex 都定义了**，这是内核从 C 里"借用 RAII"的标准做法。
**新代码应该优先用 `guard()`。**

---

## 9. ⭐ `preempt_disable_nested()`：kernel-doc 直接点名了 seqcount

这是本篇和 10.8 最直接的交叉点。`include/linux/preempt.h:425`：

```c
/**
 * preempt_disable_nested - Disable preemption inside a normally preempt disabled section
 *
 * Use for code which requires preemption protection inside a critical
 * section which has preemption disabled implicitly on non-PREEMPT_RT
 * enabled kernels, by e.g.:
 *  - holding a spinlock/rwlock
 *  - soft interrupt context
 *  - regular interrupt handlers
 *
 * On PREEMPT_RT enabled kernels spinlock/rwlock held sections, soft
 * interrupt context and regular interrupt handlers are preemptible and
 * only prevent migration. preempt_disable_nested() ensures that preemption
 * is disabled for cases which require CPU local serialization even on
 * PREEMPT_RT. For non-PREEMPT_RT kernels this is a NOP.
 *
 * The use cases are code sequences which are not serialized by a
 * particular lock instance, e.g.:
 *  - seqcount write side critical sections where the seqcount is not
 *    associated to a particular lock and therefore the automatic
 *    protection mechanism does not work. This prevents a live lock
 *    against a preempting high priority reader.
 *  - RMW per CPU variable updates like vmstat.
 */
#define preempt_disable_nested()				\
do {								\
	if (IS_ENABLED(CONFIG_PREEMPT_RT))			\
		preempt_disable();				\
	else							\
		lockdep_assert_preemption_disabled();		\
} while (0)
```

### 三个要点

**① 它在非 RT 上是"断言"，在 RT 上是"真禁抢占"**：

| 配置 | 展开成 | 效果 |
|------|--------|------|
| 非 RT | `lockdep_assert_preemption_disabled()` | ⭐ **什么都不做**，只是让 lockdep 检查"你确实已经处于禁抢占状态" |
| RT | `preempt_disable()` | 真的加一层 |

**② kernel-doc 里点名的两个用例，正好是本篇的两个邻居**：

| 用例 | 对应 |
|------|------|
| "**seqcount write side critical sections where the seqcount is not associated to a particular lock** ... This prevents a **live lock against a preempting high priority reader**" | ⭐ **10.8 §9 约束②**：seqlock 写侧被抢占 → 读者自旋一个 tick → RT 读者活锁。<br>这里给出了官方推荐的解法 |
| "RMW per CPU variable updates like vmstat" | per-CPU 变量的"读-改-写"序列（见 §11） |

**③ 它解决的正是"RT 让隐式假设失效"这类问题**：
非 RT 上"持有 spinlock ⟹ 禁抢占"成立；RT 上 spinlock 变 rt_mutex，
**这条隐含前提没了**，所有依赖它的代码都会静默出错。
`preempt_disable_nested()` 就是给这类"我原本依赖隐式禁抢占"的代码一个
**在两个配置下都正确**的写法。

---

## 10. `migrate_disable()`：为什么不推荐，RT 为什么非要它

`include/linux/preempt.h:360` 有一段 40 行注释，标题就叫
**"Migrate-Disable and why it is undesired."**

### 为什么不推荐（注释摘要）

> "When a preempted task becomes eligible to run ... it might still have to
> wait for the preemptee's `migrate_disable()` section to complete. Thereby
> suffering a reduction in bandwidth...
> **IOW it trades latency / moves the interference term, but it stays in the
> system, and as long as it remains unbounded, the system is not fully
> deterministic.**"

翻译成对比表：

| | `preempt_disable()` | `migrate_disable()` |
|--|--------------------|--------------------|
| 高优先级任务唤醒延迟 | 要等低优先级任务跑完整个临界区 | ✅ **更短**（它可以先跑，只是不能迁移） |
| 低优先级任务可用带宽 | ✅ 可以随时迁到别的核 | ❌ 被钉住，可能挤占高优先级任务的带宽 |
| 系统确定性 | 干扰项是"抢占延迟" | 干扰项变成"迁移延迟"——**换了个地方，没消失** |

**结论**：`migrate_disable()` **只是把干扰项从一处搬到另一处**，
注释最后说："The end goal must be to get rid of `migrate_disable()`,
alternatively we need a schedulability theory that does not depend on
arbitrary migration."

### 那为什么还要有它

> "**PREEMPT_RT breaks a number of assumptions traditionally held.** By
> forcing a number of primitives into becoming preemptible, they would also
> allow migration. This turns out to break a bunch of **per-cpu usage**. To
> this end, all these primitives employ `migrate_disable()` to restore this
> implicit assumption."

⭐ **这就是 RT 上最隐蔽的一类 bug 来源**：per-CPU 数据的传统保护方式是
"`preempt_disable()`（或持有 spinlock）⟹ 我不会迁移到别的 CPU"。
RT 上这两者**都不再阻止迁移**，于是 per-CPU 假设破掉了。

| 传统假设 | 非 RT | RT |
|---------|-------|-----|
| 持有 `spinlock_t` ⟹ 不迁移 | ✅（禁抢占） | ❌（rt_mutex 可抢占） |
| `preempt_disable()` ⟹ 不迁移 | ✅ | ✅ |
| softirq 上下文 ⟹ 不迁移 | ✅ | ❌（线程化 softirq 可抢占） |

→ **RT 上要显式用 `migrate_disable()` 或 `local_lock_t` 来恢复"不迁移"保证。**

注释把 `migrate_disable()` 定性为：

> "This is a **'temporary' work-around at best**. The correct solution is
> getting rid of the above assumptions and reworking the code to employ
> explicit per-cpu locking or short preempt-disable regions."

⚠️ 而且实现有个硬约束（注释："the implementation is particularly tricky"）：
**`migrate_disable()` 和 `migrate_enable()` 都不允许阻塞**，
因为它要能在持 spinlock 的上下文里用。

---

## 11. per-CPU 数据到底该用什么保护

| 场景 | 用什么 | 理由 |
|------|--------|------|
| 只被**进程上下文**访问 | `preempt_disable()` / `guard(preempt)` | 挡住同 CPU 上的其它任务 |
| 还会被 **softirq / tasklet** 访问 | `local_bh_disable()` / `spin_lock_bh()` | 进程上下文 + 下半部互斥 |
| 还会被**硬中断**访问 | `local_irq_save()` / `spin_lock_irqsave()` | 最重，什么都挡住 |
| 只想**读改写**一个 per-CPU 标量 | ⭐ **`this_cpu_add()` / `this_cpu_inc()`** | 它们内部自带禁抢占，且编译成单条 `add` 指令 |
| 要在 RT 上也成立 | `local_lock_t` 或 `migrate_disable()` | 见 §10 |
| 只要"当前 CPU 编号" | `get_cpu()` / `put_cpu()` | = `preempt_disable()` + `smp_processor_id()` |

### ⭐ `this_cpu_*` 的正确性从哪来

```c
#define get_cpu()		({ preempt_disable(); __smp_processor_id(); })   /* linux/smp.h:274 */
#define put_cpu()		preempt_enable()
```

`get_cpu()` 的语义就是"**禁抢占 + 取 CPU 编号**"，保证你拿到的编号
在 `put_cpu()` 之前一直有效。

而 `this_cpu_add(pcp, val)` 这类操作在 x86 上编译成
**一条带 per-CPU 段前缀的 `add` 指令**，中间不会被打断——
**但前提是编译器不能把它拆开、且不能被抢占迁移**（否则"读"在 CPU0、
"写"跑到 CPU1 就错了）。所以它们内部会先 `preempt_disable()`。

> 📌 详细实现见 [12.10 每个 CPU 的分配](../../chapter-12-memory-management/notes/section-12.10-每个-CPU-的分配.md)
> （那里核对过 `raw/this_cpu` 三族语义）。

### ⚠️ `smp_processor_id()` 的三个稳定性条件

`linux/smp.h:243` 的 kernel-doc：

> "The CPU id is stable when:
>  - IRQs are disabled;
>  - preemption is disabled;
>  - the task is CPU affine."

并且：

```c
#ifdef CONFIG_DEBUG_PREEMPT
  extern unsigned int debug_smp_processor_id(void);
# define smp_processor_id() debug_smp_processor_id()
#else
# define smp_processor_id() __smp_processor_id()
#endif
```

⭐ **开 `CONFIG_DEBUG_PREEMPT` 时，`smp_processor_id()` 会变成一个
带检查的函数**——在不稳定上下文里用它就 `WARN`。
所以：**不确定稳不稳就用 `get_cpu()`**，别裸调 `smp_processor_id()`。

---

## 12. `preempt_notifier`：抢占不是"黑盒"

```c
struct preempt_ops {
	void (*sched_in)(struct preempt_notifier *notifier, int cpu);
	void (*sched_out)(struct preempt_notifier *notifier,
			  struct task_struct *next);
};

struct preempt_notifier {
	struct hlist_node link;
	struct preempt_ops *ops;
};
```

（`include/linux/preempt.h:326`）

**典型用户：KVM** —— 虚拟机运行期间，VMCS 里存的是 guest 的 CPU 状态；
一旦 VCPU 线程被抢占，必须在 `sched_out` 里把硬件状态存回 VMCS，
`sched_in` 时再恢复。

注释特意点明两个回调的**上下文不同**（`:321`）：

> "Please note that `sched_in` and `out` are called under **different
> contexts**. `sched_out` is called with **rq lock held and irq disabled**
> while `sched_in` is called **without rq lock and irq enabled**. This
> difference is intentional and depended upon by its users."

⚠️ **写 `sched_out` 回调时你在持 rq->lock + 关中断的上下文里**——
不能睡眠、不能拿大多数锁。**这是最容易写错的地方。**

---

## HFT / 嵌入式关联

### `preempt_disable()` 的延迟代价：它直接吃掉调度延迟预算

| 抢占模型 | `preempt_disable()` 区域的后果 |
|---------|------------------------------|
| `PREEMPT_NONE` | 本来就只在显式抢占点调度 → 影响小，**但 `preempt_enable()` 也不调度**（见 §6） |
| `PREEMPT_VOLUNTARY` | 同上 + 更多显式点 |
| `PREEMPT` | ⭐ **临界区内完全不可抢占** → 高优先级任务要等它结束 |
| `PREEMPT_RT` | 同上（RT 上 `preempt_disable()` 是少数几个真正的"不可抢占区"） |

⭐ **RT 上 `preempt_disable()` 区域是延迟的"最后堡垒"**——
RT 把 spinlock、中断处理、softirq 全都变成可抢占的了，
**唯一剩下的不可抢占区就是显式 `preempt_disable()`（和关中断）**。
所以 RT 内核调优的核心工作之一就是**找出并缩短这些区域**。

### 量化：怎么测 `preempt_disable()` 区域有多长

| 手段 | 做法 |
|------|------|
| **`preemptirq_delay` tracepoint** | 内核自带 `trace_preemptirq` 系列，专门记录关抢占/关中断时长 |
| **ftrace `preemptoff` tracer** | `echo preemptoff > /sys/kernel/tracing/current_tracer` —— **直接列出最长的关抢占区段 + 调用栈**。这是首选工具 |
| **`irqsoff` tracer** | 同理，测关中断时长 |
| **`CONFIG_DEBUG_PREEMPT`** | 检查"在禁抢占区里调了会睡眠的函数"（如 `kmalloc(GFP_KERNEL)`），是**写驱动时的必备开关** |
| **`preemptirqsoff` tracer** | preemptoff + irqsoff 合并 |
| **cyclictest** | 端到端量测；`-p` 指定优先级，看 **max latency** 而不是 avg |

> 📌 **RT 调优的标准流程**：`cyclictest` 发现 max latency 超标 →
> `preemptoff` tracer 抓出最长的关抢占区段 → 定位到具体函数 →
> 缩短它 / 换成 `migrate_disable()` / 拆分。

### 用户态对照（HFT 侧）

| 内核手段 | 用户态近似 | 差别 |
|---------|-----------|------|
| `preempt_disable()` | **`sched_setaffinity` 绑核** + 不主动阻塞 | 用户态**无法**阻止内核抢占你（只有 RT 优先级 + `isolcpus` 能接近） |
| `SCHED_FIFO` | `SCHED_FIFO` | RT 线程不会被 CFS 任务抢占（但仍会被更高 RT、中断、以及**内核里的 `preempt_disable()` 区**拖住） |
| `isolcpus` | `isolcpus` + `nohz_full` | 隔离核上没有其它任务 → 调度器几乎不介入 |
| — | ⭐ **内核的 `preempt_disable()` 区域会拖住 RT 线程** | **这是用户态控制不了的**——所以挑内核很重要（RT 或 `preempt=full`） |

⚠️ **一个常见误判**：以为"用了 `SCHED_FIFO` 就能保证延迟"。
实际上 **RT 线程在内核态执行时，如果踩进一段 `preempt_disable()` 区域，
照样要等它跑完**。所以 `preempt=full`（或 RT）对 HFT 的意义是
**缩短内核里那些不可抢占区段**，而不是"让用户态线程不被抢占"。

### 嵌入式：`PREEMPT_NONE` 上的"隐蔽假阴性"

回顾 §6 的那条：在 `PREEMPT_NONE` 内核上 `preempt_disable()` 就是 `barrier()`。
这意味着：

```
在 PREEMPT_NONE 上开发、测试都正常
    ↓
（因为 preempt_disable() 什么也没做，某些 per-CPU bug 被"天然不抢占"掩盖了）
    ↓
切到 PREEMPT / RT
    ↓
per-CPU 代码开始真正并发 → 隐藏的数据竞争暴露
```

反过来也一样：**在 `PREEMPT` 上开发、在 `PREEMPT_NONE` 上部署**，
可能发现"我明明 `preempt_disable()` 了怎么还是被调度走了"
（答案是：会走显式抢占点，比如 `cond_resched()`）。

> 📌 **规则**：**抢占模型必须和部署目标一致地在 CI 里测。**
> 这和 10.7 §HFT 里讲的 "UP 开发 / SMP 部署" 是同一类移植陷阱。

---

## 实践模板

### 模板 A：per-CPU 计数器（v6.6 推荐写法）

```c
DEFINE_PER_CPU(unsigned long, packets);

/* ---- 更新（v6.6 推荐：guard 自动配对）---- */
void count_packet(void)
{
	guard(preempt)();                    /* ⭐ 作用域结束自动 enable */
	this_cpu_inc(packets);
}

/* ---- 读取汇总（要跨 CPU 读，禁抢占没用！）---- */
unsigned long total_packets(void)
{
	unsigned long sum = 0;
	int cpu;
	for_each_possible_cpu(cpu)
		sum += per_cpu(packets, cpu);    /* ⚠️ 这只是一个【快照】，不是原子的 */
	return sum;
}
```

⚠️ **汇总循环里 `preempt_disable()` 是没用的**——你要读**别的** CPU 的槽，
而 `preempt_disable()` 只保证"我自己不迁移"。要精确汇总需要
`percpu_counter` 或额外加锁（见 [12.10](../../chapter-12-memory-management/notes/section-12.10-每个-CPU-的分配.md)）。

### 模板 B：既要防进程上下文、又要防下半部

```c
/* 会被 tasklet / softirq 访问 → 必须 _bh */
static void update_stats_bh_safe(struct my_dev *d, int cpu)
{
	unsigned long flags;
	local_irq_save(flags);              /* ⭐ 最保险：进程 + 中断 + 下半部全挡 */
	/* ... 操作 per-CPU 数据 ... */
	local_irq_restore(flags);
}
```

**选择判据表**（按"谁会访问这份数据"倒推）：

| 访问者 | 需要 |
|--------|------|
| 只有进程上下文 | `preempt_disable()` |
| + softirq / tasklet | `local_bh_disable()` 或 `spin_lock_bh()` |
| + 硬中断 | `local_irq_save()` 或 `spin_lock_irqsave()` |
| + NMI | ⭐ **以上全都挡不住 NMI** —— 只能用无锁结构或 `seqcount_latch_t`（见 10.8 §8） |

### 模板 C：RT 上也要成立的 per-CPU 临界区

```c
#include <linux/local_lock.h>

static DEFINE_LOCAL_IRQ_LOCK(my_stat_lock);      /* 或 DEFINE_LOCAL_LOCK() */

void update(void)
{
	local_lock_irqsave(&my_stat_lock, flags);
	this_cpu_inc(my_stat);
	local_unlock_irqrestore(&my_stat_lock, flags);
}
```

`local_lock_t` 的语义：

| 配置 | `local_lock()` 展开成 |
|------|---------------------|
| 非 RT | `preempt_disable()`（零额外开销） |
| **RT** | **`migrate_disable()` + 一把 per-CPU 的 `spinlock_t`** |

⭐ **这是 RT 上写 per-CPU 代码的正确姿势**——它在两个配置下都正确，
且在非 RT 上没有任何额外开销。

### 自检清单

| # | 检查 | 不通过怎么办 |
|---|------|------------|
| 1 | 临界区里有睡眠 / `GFP_KERNEL` 分配 / `copy_from_user()` 吗？ | 有 → ❌ 禁抢占区里不能睡，改用 mutex 或把分配挪到区外 |
| 2 | 用了 `in_atomic()` 判断"能不能睡"吗？ | ⚠️ 换成 `might_sleep()` 断言或 `in_task()` 等特异性判断（§5） |
| 3 | 用了 `in_irq()` / `in_softirq()` / `in_interrupt()` 吗？ | ⚠️ 已废弃，换 `in_hardirq()` / `in_serving_softirq()` |
| 4 | 数据会被中断访问吗？ | 会 → 不能用 `preempt_disable()`，要 `_irqsave`（§11 表） |
| 5 | 模块里用了 `preempt_enable_no_resched()` 吗？ | ❌ 模块里被 `#undef` 了（§7） |
| 6 | 目标配置是 `PREEMPT_NONE` 吗？ | 是 → `preempt_disable()` 其实只是 `barrier()`（§6） |
| 7 | 裸调了 `smp_processor_id()` 吗？ | ⚠️ 换成 `get_cpu()`，或确认三个稳定性条件之一成立 |
| 8 | 会在 RT 上跑吗？ | 会 → per-CPU 代码要用 `local_lock_t` / `migrate_disable()`（§10） |
| 9 | 配对能 100% 保证吗（含所有 `return` / `goto`）？ | 不能 → 改用 `guard(preempt)`（§8） |
| 10 | 临界区超过 1 µs 吗？ | 是 → 用 `preemptoff` tracer 确认真的影响延迟，再决定拆不拆 |

---

## 易错点核对表

| # | 易错点 | 正确做法 |
|---|--------|---------|
| 1 | 以为 `preempt_count` 在 `thread_info` 里 | ⚠️ **v6.6 x86 在 per-CPU `pcpu_hot` 里**（v6.2 引入）；通用架构仍在 `thread_info` |
| 2 | 以为 `preempt_disable()` 能挡中断 | ❌ 只挡抢占。要挡中断用 `local_irq_disable()` |
| 3 | 以为 `preempt_disable()` 能挡别的 CPU | ❌ 完全不挡。跨 CPU 要 spinlock / 原子 |
| 4 | 用 `in_atomic()` 判断"能不能睡眠" | ❌ **官方明令禁止**（"Do not use in_atomic() in driver code"），非抢占内核下持有 spinlock 会伪阴性 |
| 5 | 用 `in_softirq()` 判断"正在处理 softirq" | ❌ 它把"仅禁了 BH"也算进去。用 `in_serving_softirq()` |
| 6 | 以为 `PREEMPT_NONE` 上 `preempt_disable()` 也有计数器 | ❌ 它就是 `barrier()` |
| 7 | 以为 `preempt_enable()` 总会调度 | ❌ `!CONFIG_PREEMPTION` 时只递减、不调度 |
| 8 | 多调一次 `preempt_enable()` | ❌ 计数变负 → 该 CPU 永不抢占。用 `guard(preempt)` |
| 9 | 在模块里用 `preempt_enable_no_resched()` | ❌ 被 `#undef` 了，"Modules have no business playing preemption tricks" |
| 10 | 禁抢占区里调 `kmalloc(GFP_KERNEL)` / `copy_from_user()` | ❌ 会睡。开 `CONFIG_DEBUG_PREEMPT` 会抓到 |
| 11 | 以为 `preempt_disable()` 能保护"跨 CPU 汇总" | ❌ 它只保证自己不迁移，不阻止别人读别的 CPU 的槽 |
| 12 | 以为 `SCHED_FIFO` 就能躲开 `preempt_disable()` 区 | ❌ 内核里的禁抢占区照样拖住你 |
| 13 | 写 `preempt_notifier` 的 `sched_out` 里拿锁/睡眠 | ❌ 它跑在**持 rq->lock + 关中断**上下文 |
| 14 | 以为 `preempt_count` 只有抢占计数 | ❌ 一个字里塞了 preempt/softirq/hardirq/NMI 四个计数器（+ MSB 标志） |

---

## 常见陷阱

1. 混淆 preempt_disable() 和 local_irq_disable()——前者只禁抢占，后者还禁中断
2. 以为 preempt_disable() 后不能被中断——可以被中断，但不能被调度
3. 在 preempt_disable() 区域做耗时操作——会延迟调度器，增加系统延迟
4. **（v6.6 补充）** 用 `in_atomic()` 判断能否睡眠——官方禁止，且非抢占内核下有误判
5. **（v6.6 补充）** 用已废弃的 `in_irq()` / `in_softirq()` / `in_interrupt()`
6. **（v6.6 补充）** 在 `PREEMPT_NONE` 内核上以为 `preempt_disable()` 真的加了计数
7. **（v6.6 补充）** 模块里用 `preempt_enable_no_resched()`（已被 `#undef`）
8. **（v6.6 补充）** 在 RT 上用 `preempt_disable()` 保护 per-CPU 数据却忘了
   RT 上 spinlock 不再禁抢占 → 应该用 `local_lock_t` / `migrate_disable()`
9. **（v6.6 补充）** 手工配对 `preempt_disable/enable` 漏掉某个 `return` 路径

---

## 自测题

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `preempt_disable()` 的精确效果？

<details><summary>答案</summary>

① 递增 preempt_count 的 preempt 位。② 当前 CPU 上的内核代码不会被抢占（schedule() 检查 preempt_count == 0 才调度）。③ 中断仍可触发（hard IRQ）。④ softirq 仍可执行。⑤ 其他 CPU 不受影响。用于保护 per-CPU 数据（防止被另一进程在同 CPU 上访问）。对应 preempt_enable() 递减并检查 need_resched。

<details><summary>按 v6.6 修订/补充</summary>

**五条全部正确**，补六个源码层面的精确化：

**① "preempt 位"的具体布局**（`preempt.h:14-53`）——`preempt_count` 是
**一个 32 位字里塞了四个计数器**：

```
 bit 31 ─ NEED_RESCHED（x86，反相）
 bit 20-23 ─ NMI        (4 bit)
 bit 16-19 ─ HARDIRQ    (4 bit)
 bit  8-15 ─ SOFTIRQ    (8 bit)
 bit  0-7  ─ PREEMPT    (8 bit)   ← preempt_disable() 加在这一段
```

每层 `preempt_disable()` 加 `PREEMPT_OFFSET`（= 1），所以**最多嵌套 256 层**。

**② v6.6 x86 上它不在 `thread_info` 里**（§2）：

```c
return raw_cpu_read_4(pcpu_hot.preempt_count) & ~PREEMPT_NEED_RESCHED;
```

`pcpu_hot` 是一个 **64 字节、cacheline 对齐**的 per-CPU 结构体，
里面同时装着 `current_task`、`preempt_count`、`cpu_number`、`top_of_stack`——
**版本断崖：`pcpu_hot` 引入于 v6.2**（v6.1 的 `current.h` 只有独立的
`current_task` per-CPU 指针）。
⚠️ 通用架构（`asm-generic/preempt.h`）仍然用 `current_thread_info()->preempt_count`。

**③ 真实展开里 `barrier()` 的位置有讲究**（§3）：

```c
/* disable */  preempt_count_inc();  barrier();    /* 自增【之后】 */
/* enable  */  barrier();  if (dec_and_test()) __preempt_schedule();  /* 自减【之前】 */
```

一后一前，夹住临界区，防止编译器把里面的代码挪出去。

**④ 第 ③ 条"中断仍可触发、softirq 仍可执行"完全正确**，而且可以从位布局看得很清楚：
`preempt_disable()` 只动**位 0-7**，而 `HARDIRQ_MASK`（位 16-19）和
`SOFTIRQ_MASK`（位 8-15）是**独立的字段**——所以硬中断和 softirq
的嵌套计数**照常增加**，不受影响。

**⑤ 第 ⑤ 条"其他 CPU 不受影响"要强调**：这是 `preempt_disable()`
与 spinlock 的**本质区别**。`preempt_disable()` 是**纯 CPU 本地**的——
它不产生任何跨 CPU 的原子操作或缓存一致性流量。
这也是它比 spinlock 快得多的原因（代价是只能保护 per-CPU 数据）。

**⑥ 补一个原答案没说的关键前提**（§6）：

```c
#else /* !CONFIG_PREEMPT_COUNT */
#define preempt_disable()	barrier()
```

**在 `CONFIG_PREEMPT_COUNT=n`（即 `PREEMPT_NONE` / `PREEMPT_VOLUNTARY`）的内核上，
`preempt_disable()` 展开成 `barrier()`，根本没有计数器！**
此时它的唯一作用是防止 `get_user()/put_user()` 之类的可睡眠操作
被编译器挪进"受保护区域"（源码注释原文）。
所以第 ① 条"递增 preempt 位"**只对 `PREEMPT` / `RT` / `PREEMPT_DYNAMIC` 内核成立**。

</details>
</details>

**Q2.** `preempt_enable()` 时如果 need_resched 被设置会怎样？

<details><summary>答案</summary>

`preempt_enable()` 递减 preempt_count，如果 preempt_count 归零且 `need_resched` 被设置 → `preempt_schedule()` → `schedule()` 切换到更高优先级任务。这就是内核抢占点。`preempt_enable_no_resched()` 不检查 need_resched（延迟到下一个抢占点），用于明确不需要立即调度的场景。

<details><summary>按 v6.6 修订/补充</summary>

**正确**，补 x86 上那个非常漂亮的优化（§4）——
**v6.6 的 x86 把"减一"和"测试 need_resched"合并成了一条指令**。

**① NEED_RESCHED 位被塞进 MSB，而且【反相】**：

```c
#define PREEMPT_NEED_RESCHED	0x80000000
#define PREEMPT_ENABLED	(0 + PREEMPT_NEED_RESCHED)

static __always_inline void set_preempt_need_resched(void)
{
	raw_cpu_and_4(pcpu_hot.preempt_count, ~PREEMPT_NEED_RESCHED);   /* 清位 = 需要调度 */
}
```

反相后，"需要调度" = 位为 **0**，"不需要" = 位为 **1**。

**② 于是 `decl` 指令一次判定两件事**：

```c
static __always_inline bool __preempt_count_dec_and_test(void)
{
	return GEN_UNARY_RMWcc("decl", pcpu_hot.preempt_count, e, __percpu_arg([var]));
}
```

```
decl pcpu_hot.preempt_count
     结果 == 0  ⟺  计数值归零（可抢占） 且 NEED_RESCHED 位为 0（需要调度）
                    └────── 两件事用【一次减法】同时判定 ──────┘
```

`preempt_enable()` 的快路径就是 `barrier(); decl; je` —— **没有单独的位测试**。

**③ `preempt_count()` 对外必须掩掉这一位**，否则历史代码全崩：

```c
return raw_cpu_read_4(pcpu_hot.preempt_count) & ~PREEMPT_NEED_RESCHED;
```

注释："not to confuse all current users that think a non-zero value
indicates we cannot preempt"（否则 `in_atomic()` 在 x86 上永远返回真）。

**④ `should_resched()` 在 v6.6 是【等值比较】而不是位测试**：

```c
return unlikely(raw_cpu_read_4(pcpu_hot.preempt_count) == preempt_offset);
```

**⑤ 关于 `preempt_enable_no_resched()`，补两条约束**：

- 它**只在非 RT、且 `CONFIG_PREEMPT_COUNT` 打开时**才有意义；
- ⭐ **模块里它被 `#undef` 掉了**：
  ```c
  #ifdef MODULE
  /* Modules have no business playing preemption tricks. */
  #undef preempt_enable_no_resched
  #endif
  ```
  理由是"调用者知道后面马上会有抢占点"是个**全局假设**，模块作者通常没有这个视野。

**⑥ 补一条原答案没提的**：`preempt_enable()` **并非总会调度**——

```c
#else /* !CONFIG_PREEMPTION */
#define preempt_enable() \
do { barrier(); preempt_count_dec(); } while (0)
```

在 `PREEMPT_NONE` / `VOLUNTARY` 上，`preempt_enable()` **只递减，从不调度**。
真正的调度要等到下一个**显式抢占点**（如 `cond_resched()`、返回用户态）。

</details>
</details>

**Q3.** HFT 如何利用抢占控制降低延迟？

<details><summary>答案</summary>

① `SCHED_FIFO`：RT 线程不可被 CFS 抢占（只有更高 RT 或中断能抢占）。② `isolcpus`：隔离核上无其他任务，调度器几乎不触发。③ `nohz_full`：停止定时器中断，减少 `scheduler_tick()`。④ `preempt=full`：让非 RT 任务的内核路径也可被抢占（减少长尾延迟）。⑤ 内核模块中 `preempt_disable()` 临界区 <1us。

<details><summary>按 v6.6 修订/补充</summary>

**五条都对，而且第 ④ 条在 v6.6 里有了更好的实现方式。**

**① 关于 `preempt=full`**：这个启动参数依赖 **`CONFIG_PREEMPT_DYNAMIC`**
（`depends on HAVE_PREEMPT_DYNAMIC && !PREEMPT_RT`）。
它是通过 **static call** 实现的：

```c
DECLARE_STATIC_CALL(preempt_schedule, preempt_schedule_dynamic_enabled);
#define __preempt_schedule() \
do { \
	__STATIC_CALL_MOD_ADDRESSABLE(preempt_schedule); \
	asm volatile ("call " STATIC_CALL_TRAMP_STR(preempt_schedule) : ASM_CALL_CONSTRAINT); \
} while (0)
```

开销"negligible with HAVE_STATIC_CALL_INLINE"。
⚠️ 但注意：**`PREEMPT_DYNAMIC` 明确 `depends on !PREEMPT_RT`** ——
RT 是**编译期**决定的，不能启动时切。

**② 补一条最重要的认知修正**（§HFT 关联）：

> ⚠️ **用了 `SCHED_FIFO` 也躲不开内核里的 `preempt_disable()` 区域。**

RT 线程在**内核态**执行时（系统调用、页错误、中断返回路径），
如果踩进一段 `preempt_disable()` 区域，**照样要等它跑完**。
`SCHED_FIFO` 只解决"**用户态**不被 CFS 任务抢占"的问题。

所以第 ④ 条（`preempt=full` / RT）的真正价值是
**缩短内核里那些不可抢占区段**，而不是"让用户态线程不被抢占"。

**③ 在 RT 上，`preempt_disable()` 区域是"最后堡垒"**：
RT 已经把 spinlock（→rt_mutex）、中断处理（→线程化）、
softirq（→ksoftirqd 线程）**全都变成可抢占的了**，
**唯一剩下的真正不可抢占区就是显式 `preempt_disable()` 和关中断**。
所以 RT 内核调优的核心就是找出并缩短这些区域。

**④ 量化工具**（原答案缺这块）：

| 目的 | 工具 |
|------|------|
| 直接列出**最长的关抢占区段 + 调用栈** | ⭐ `echo preemptoff > /sys/kernel/tracing/current_tracer` |
| 关中断时长 | `irqsoff` tracer |
| 两者合并 | `preemptirqsoff` tracer |
| 抓"在禁抢占区里调了会睡眠的函数" | ⭐ **`CONFIG_DEBUG_PREEMPT`**（写驱动时的必备开关） |
| 端到端 | `cyclictest`，**看 max latency 不看 avg** |

标准流程：`cyclictest` 发现 max 超标 → `preemptoff` tracer 抓出最长区段 →
定位函数 → 缩短 / 换 `migrate_disable()` / 拆分。

**⑤ 第 ⑤ 条"临界区 <1µs"加一条可操作建议**：
**用 `guard(preempt)` 而不是手工配对**——这样临界区边界由作用域定义，
不可能漏掉某个 `return` 路径：

```c
void hot_path(void)
{
	guard(preempt)();
	this_cpu_inc(counter);
}
```

</details>
</details>

**Q4.** （v6.6 新增）`preempt_count` 里除了抢占计数还有什么？`in_atomic()` 为什么被官方禁用？

<details><summary>答案</summary>

**（a）位布局**（`preempt.h:14-53`）——一个 32 位字塞了四个计数器：

```
 bit 31    ─ PREEMPT_NEED_RESCHED（x86 专属，反相）
 bit 20-23 ─ NMI        (4 bit, NMI_MASK     = 0x00f00000)
 bit 16-19 ─ HARDIRQ    (4 bit, HARDIRQ_MASK = 0x000f0000)
 bit  8-15 ─ SOFTIRQ    (8 bit, SOFTIRQ_MASK = 0x0000ff00)
 bit  0-7  ─ PREEMPT    (8 bit, PREEMPT_MASK = 0x000000ff)
```

两个值得注意的取舍：
- HARDIRQ 只给 4 位，因为"中断处理期间是关中断的，不会有嵌套中断"；
  注释特别提到"there are a few **palaeontologic drivers** which reenable
  interrupts in the handler, so we need more than one bit here"。
- `SOFTIRQ_DISABLE_OFFSET = 2 * SOFTIRQ_OFFSET`——多出来的那一位用来
  区分"**正在处理 softirq**"（1 个单位）和"**只是 `local_bh_disable()` 了**"（2 个单位）。
  所以 `in_serving_softirq()` 能写成 `softirq_count() & SOFTIRQ_OFFSET`。

**（b）`in_atomic()` 被禁用的原因**（源码注释原文）：

```c
/*
 * Are we running in atomic context?  WARNING: this macro cannot
 * always detect atomic context; in particular, it cannot know about
 * held spinlocks in non-preemptible kernels.  Thus it should not be
 * used in the general case to determine whether sleeping is possible.
 * Do not use in_atomic() in driver code.
 */
#define in_atomic()	(preempt_count() != 0)
```

**核心问题：伪阴性。**
在 `CONFIG_PREEMPT_COUNT=n` 的内核上，spinlock **根本不修改 `preempt_count`**
（那个配置下 `preempt_disable()` 本身就是 `barrier()`），
所以**持有 spinlock 时 `in_atomic()` 仍然返回 false**——
明明不能睡眠，它却说可以。

**该用什么替代**：

| 目的 | 用什么 |
|------|--------|
| "我这段代码能不能睡眠" | ⭐ **`might_sleep()`** —— 让内核替你检查并告警，而不是自己 `if` 分支 |
| "我在不在进程上下文" | `in_task()` |
| "我在不在硬中断" | `in_hardirq()`（**不是**已废弃的 `in_irq()`） |
| "我在不在处理 softirq" | `in_serving_softirq()`（**不是** `in_softirq()`） |
| "我在不在 NMI" | `in_nmi()` |
| 想要 0~3 的分级 | `interrupt_context_level()` |

**顺带**：`in_irq()` / `in_softirq()` / `in_interrupt()` 三个宏
在 v6.6 已被源码明确标注 **deprecated**（`preempt.h:124`）。

</details>

**Q5.** （v6.6 新增）`preempt_disable()` 和 `migrate_disable()` 有什么区别？为什么 RT 上需要后者？

<details><summary>答案</summary>

| | `preempt_disable()` | `migrate_disable()` |
|--|--------------------|--------------------|
| 阻止被抢占 | ✅ | ❌（仍可被抢占） |
| 阻止迁移到别的 CPU | ✅ | ✅ |
| 影响高优先级任务的唤醒延迟 | ❌ 要等临界区结束 | ✅ 更短 |

**为什么 RT 上需要 `migrate_disable()`**（`preempt.h:387` 注释原文）：

> "**PREEMPT_RT breaks a number of assumptions traditionally held.** By
> forcing a number of primitives into becoming preemptible, they would also
> allow migration. This turns out to break a bunch of **per-cpu usage**."

具体说，per-CPU 数据的传统保护依赖这个隐含前提：

| 传统假设 | 非 RT | RT |
|---------|-------|-----|
| 持有 `spinlock_t` ⟹ 不迁移 | ✅（禁抢占） | ❌（rt_mutex 可抢占 → 可被迁移） |
| `preempt_disable()` ⟹ 不迁移 | ✅ | ✅ |
| softirq 上下文 ⟹ 不迁移 | ✅ | ❌（线程化 softirq 可抢占） |

→ **RT 上要显式用 `migrate_disable()`（或 `local_lock_t`）来恢复"不迁移"保证。**

**为什么官方文档又"不推荐"它**（`preempt.h:360` 的标题就叫
"Migrate-Disable and why it is undesired"）：

> "IOW it **trades latency / moves the interference term, but it stays in
> the system**, and as long as it remains unbounded, the system is not
> fully deterministic."

| | `preempt_disable()` | `migrate_disable()` |
|--|--------------------|--------------------|
| 高优先级任务唤醒延迟 | 差（要等临界区） | ✅ 好（可以先跑） |
| 低优先级任务可用带宽 | ✅ 好（可迁走） | 差（被钉住挤占带宽） |

**干扰项只是换了个地方，没有消失。** 注释的定性是：

> "This is a **'temporary' work-around at best**. The correct solution is
> getting rid of the above assumptions and reworking the code to employ
> explicit per-cpu locking or short preempt-disable regions."
> "The end goal must be to get rid of `migrate_disable()`."

**一个实现上的硬约束**：`migrate_disable()` / `migrate_enable()`
**都不允许阻塞**（注释："neither is allowed to block"），
因为它要能在持有 spinlock 的上下文里使用。

**实践建议**：写需要在两种配置下都正确的 per-CPU 代码，
用 **`local_lock_t`** 而不是裸 `migrate_disable()`：

| 配置 | `local_lock()` 展开成 |
|------|---------------------|
| 非 RT | `preempt_disable()`（**零额外开销**） |
| RT | `migrate_disable()` + per-CPU `spinlock_t` |

</details>

**Q6.** （v6.6 新增）`preempt_disable_nested()` 是干什么的？它和 seqlock 有什么关系？

<details><summary>答案</summary>

**它的语义按配置分叉**（`preempt.h:450`）：

```c
#define preempt_disable_nested()				\
do {								\
	if (IS_ENABLED(CONFIG_PREEMPT_RT))			\
		preempt_disable();				\
	else							\
		lockdep_assert_preemption_disabled();		\
} while (0)
```

| 配置 | 展开成 | 效果 |
|------|--------|------|
| 非 RT | `lockdep_assert_preemption_disabled()` | **什么都不做**，只让 lockdep 检查"你确实已处于禁抢占状态" |
| RT | `preempt_disable()` | 真的加一层 |

**用途**（kernel-doc 原文）：给那些"**在非 RT 上隐式享有禁抢占、但 RT 上没有**"
的临界区用，即：持有 spinlock/rwlock、softirq 上下文、中断处理函数。

**⭐ 和 seqlock 的关系**：kernel-doc 里**点名了 seqcount**——

> "The use cases are code sequences which are **not serialized by a
> particular lock instance**, e.g.:
>  - **seqcount write side critical sections where the seqcount is not
>    associated to a particular lock** and therefore the automatic
>    protection mechanism does not work. **This prevents a live lock
>    against a preempting high priority reader.**
>  - RMW per CPU variable updates like vmstat."

这正好对应 **10.8 §9 的约束②**：

| 10.8 的说法 | 这里的说法 |
|------------|-----------|
| seqlock 写侧绝不能被抢占，否则读者自旋一个 tick；**RT 读者 → 活锁** | "prevents a **live lock** against a **preempting high priority reader**" |
| 解法一：用 `seqlock_t`（内部是 `seqcount_spinlock_t`，有 RT 的 lock+unlock 技巧） | 这里的"**automatic protection mechanism**"就指它 |
| 裸 `seqcount_t` **没有关联锁**，自动机制不生效 | "**where the seqcount is not associated to a particular lock**" |
| — | → 解法二：**`preempt_disable_nested()`** |

**换句话说**：如果你用的是裸 `seqcount_t`（没有关联锁），
在 RT 上想避免活锁，官方推荐的就是 `preempt_disable_nested()`。

**为什么用 `_nested()` 而不是直接 `preempt_disable()`**：
因为在**非 RT** 上，这些临界区（持 spinlock、softirq、中断处理）
**本来就已经禁抢占了**，再加一层 `preempt_disable()` 是纯浪费。
所以非 RT 上它退化成一条 **lockdep 断言**（顺带帮你验证"你确实在禁抢占区"），
只有 RT 上才真的加一层。**这是"按配置分叉"的标准写法。**

第二个用例 **"RMW per CPU variable updates like vmstat"** 同理：
per-CPU 变量的"读-改-写"序列在非 RT 上靠 `preempt_disable()` 保护，
RT 上要显式补上。

</details>

</details>

---

→ [10.8 顺序锁](./section-10.8-顺序锁.md) · [10.10 排序和屏障](./section-10.10-排序和屏障.md) · [Ch 4.5 抢占与上下文切换](../../chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md) · [12.10 per-CPU 分配](../../chapter-12-memory-management/notes/section-12.10-每个-CPU-的分配.md)

---
