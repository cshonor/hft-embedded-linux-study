## ⑧ 顺序锁 · Seqlocks

**偏袒写者** 的读写同步：读者 **无锁快速路径**，通过序号检测是否与写者冲突；冲突则重读。

| 角色 | 行为 |
|------|------|
| **写者** | 拿锁（或关抢占）→ 序号变奇 → 写数据 → 序号变偶 |
| **读者** | 读序号 → 读数据 → 再读序号；若期间序号变了 → **重试** |

```
读者:
  do {
    seq = read_seqbegin(&lock);
    /* 读共享数据快照 */
  } while (read_seqretry(&lock, seq));
```

#### 与 rwlock 对比

| | rwlock | seqlock |
|--|--------|---------|
| 偏向 | **读者**（写者可能饿） | **写者**（读者可能重试） |
| 读者 | 要拿读锁 | **通常不写锁变量**（只读 seq） |
| 适合 | 读多写少、写可等 | 读极多、写很少、**写必须及时** |
| 读者副作用 | — | 读侧 **不可有副作用**（可能重复执行） |

#### 适用数据

| 适合 | 不适合 |
|------|--------|
| 统计计数器、时钟相关、配置快照 | 读者要「只执行一次」的副作用 |
| 写极短 | 写很长（读者狂重试） |

**HFT：** 行情最新价、全局配置版本号很适合 seqlock 思维；用户态也可做 seqlock 风格。注意 **多字段一致**：必须整段读在 begin/retry 之间，并配合屏障语义。

→ [10.3 rwlock](./section-10.3-读-写自旋锁.md) · [10.10 屏障](./section-10.10-排序和屏障.md)

### 常见陷阱

1. 以为 seqlock 是通用读写锁——只适合写少读多 + 读端可容忍重试的场景
2. 在读端忽略 sequence 检查——seqlock 读端必须检查前后 sequence 一致，否则可能读到半写状态
3. 在写端用多个步骤——写端持锁期间应尽快完成，持锁时间 = 写者互斥时间

---

> **本篇分工**：上面速查表**原样保留**。本篇往下**不复述**"奇数/偶数序号"这个常识，
> 只做十一件事，**全部用 v6.6 源码实证**（`include/linux/seqlock.h`，38.9KB，
> 配套 `Documentation/locking/seqlock.rst`）：
>
> ① 拆开 **两个层次**：`seqcount_t`（纯计数器）与 `seqlock_t`（计数器 + 内嵌 spinlock）；
> ② ⭐ **v6.6 的 `seqlock_t` 里装的是 `seqcount_spinlock_t`，不是裸 `seqcount_t`**
> ——源码注释给出原因（防止 RT 上读者饿死写者），这是绝大多数中文资料没写的；
> ③ ⭐ `sequence` 字段**不是 `atomic_t`**，就是个 `unsigned`，
> 全靠 `READ_ONCE` + 屏障——把它当 atomic 用会漏掉真正的难点；
> ④ 精确列出**四处屏障的位置**（写 begin 一侧、写 end 一侧、读 retry 一侧、
> 以及 `__read_seqcount_begin` 里**故意没有**屏障的那个变体）；
> ⑤ 读侧**四个 begin 变体**的差异表（等不等偶数、带不带屏障、带不带 lockdep）；
> ⑥ ⭐ **三类读者**：普通读者 / `read_seqlock_excl()`（**不改 seq**！）/
> `read_seqbegin_or_lock()`（先乐观后加锁，用 `seq` 的奇偶**复用同一个变量**）；
> ⑦ 写侧变体表：`write_seqlock` / `_bh` / `_irq` / `_irqsave` 的选择判据；
> ⑧ ⭐ **版本断崖**：`seqcount_LOCKNAME_t`（关联锁 seqcount）**v5.10 引入**，
> 从 5 种砍到 4 种（`seqcount_ww_mutex_t` **删于 v5.19**），
> 但**官方文档至今仍列 5 种**——文档遗留，别照抄；
> ⑨ `seqcount_latch_t`：双缓冲 MVCC，**每次 +1 而不是 +2**，专为 NMI 读者设计；
> ⑩ ⭐ **三条致命约束**（不能有指针 / 写侧不可被抢占 / RT 活锁），
> 逐条给出源码或文档原文；
> ⑪ 真实用例 `u64_stats_sync`：**在 64 位架构上是完全的空操作**。
>
> 所有常量与代码均核对自缓存的 v6.6 源码，行号可查。

---

## 1. 两个层次：`seqcount_t` 是计数器，`seqlock_t` 才是锁

`seqlock` 这个词其实指两样东西，理清这点是看懂整个头文件的前提：

```
        seqcount_t                       seqlock_t
   ┌──────────────────┐          ┌──────────────────────────┐
   │ unsigned sequence│          │ seqcount_spinlock_t      │
   │ [lockdep dep_map]│          │   ├─ seqcount_t seqcount │
   └──────────────────┘          │   └─ spinlock_t *lock    │
                                 │ spinlock_t lock          │
   只计数，不管写者互斥           └──────────────────────────┘
   写者互斥靠【外部】锁              计数器 + 【内嵌】写者互斥
```

`seqcount_t`（`include/linux/seqlock.h:64`，v6.6）：

```c
typedef struct seqcount {
	unsigned sequence;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map dep_map;
#endif
} seqcount_t;
```

**只有 4 字节**（开 lockdep 时才多一个 `dep_map`）。
它**不解决"多个写者互斥"**的问题——文档原文：

> "This is the raw counting mechanism, which does not protect against
> multiple writers. Write side critical sections must thus be serialized
> by an external lock."

`seqlock_t`（`:797`）：

```c
typedef struct {
	/*
	 * Make sure that readers don't starve writers on PREEMPT_RT: use
	 * seqcount_spinlock_t instead of seqcount_t. Check __SEQ_LOCK().
	 */
	seqcount_spinlock_t seqcount;
	spinlock_t lock;
} seqlock_t;
```

> ⭐ **这是本篇最值得记住的一条**：v6.6 的 `seqlock_t` **不是**"裸 `seqcount_t` + spinlock"，
> 而是"**`seqcount_spinlock_t`** + spinlock"。注释讲得很直白：
> 用 `seqcount_spinlock_t`（关联锁版本）是为了**在 PREEMPT_RT 上不让读者饿死写者**。
> 机制见 §12。

### 初始化对照

| | `seqcount_t` | `seqlock_t` |
|--|-------------|------------|
| 动态 | `seqcount_init(&s)` | `seqlock_init(&sl)`（`:811`：先 `spin_lock_init` 再 `seqcount_spinlock_init`，**把锁指针回灌进 seqcount**） |
| 静态 | `SEQCNT_ZERO(name)`（`:117`） | `DEFINE_SEQLOCK(sl)`（`:819`） |
| C99 结构体初始化 | `.seq = SEQCNT_ZERO(foo.seq)` | `.seql = __SEQLOCK_UNLOCKED(foo.seql)` |

`__SEQLOCK_UNLOCKED` 的定义（`:789`）值得看一眼——它在**静态初始化期**就把
`&(lockname).lock` 的地址塞进了 seqcount：

```c
#define __SEQLOCK_UNLOCKED(lockname)					\
	{								\
		.seqcount = SEQCNT_SPINLOCK_ZERO(lockname, &(lockname).lock), \
		.lock =	__SPIN_LOCK_UNLOCKED(lockname)			\
	}
```

（同一个变量 `lockname` 被引用了两次，一次取地址一次初始化——这是内核里
少见的"自引用静态初始化"，能成立是因为指针只是存起来、不会在初始化阶段解引用。）

---

## 2. 序号协议：为什么是 **+2**，以及四处屏障在哪

### 写侧

```c
static inline void do_raw_write_seqcount_begin(seqcount_t *s)   /* :466 */
{
	kcsan_nestable_atomic_begin();
	s->sequence++;
	smp_wmb();                    /* ⭐ 屏障在【自增之后】 */
}

static inline void do_raw_write_seqcount_end(seqcount_t *s)     /* :487 */
{
	smp_wmb();                    /* ⭐ 屏障在【自增之前】 */
	s->sequence++;
	kcsan_nestable_atomic_end();
}
```

| 时刻 | `sequence` | 状态 |
|------|-----------|------|
| 空闲 | 偶数 | 稳定 |
| `write_seqcount_begin()` 后 | **奇数** | 写入进行中 |
| `write_seqcount_end()` 后 | **偶数**（+2） | 又稳定了 |

⭐ **两处 `smp_wmb()` 的方向是相反的**，这是 seqlock 的正确性核心：

```
写者:                                   读者:
  seq++                                   seq1 = READ_ONCE(seq)   /* 期望偶数 */
  smp_wmb()   ← 阻止【后面的数据写】            smp_rmb()
       │        重排到 seq++ 之前            v = READ_ONCE(data)
       ▼                                    smp_rmb()  ← read_seqcount_retry
  写 data                                   seq2 = READ_ONCE(seq)
  smp_wmb()   ← 阻止【前面的数据写】
       │        重排到 seq++ 之后            若 seq2 != seq1 → 重试
       ▼
  seq++
```

- **begin 侧的 `smp_wmb()`**：保证"序号已经变奇"这件事**先于**任何数据写被看到。
  否则读者可能看到偶数序号、却读到写了一半的数据。
- **end 侧的 `smp_wmb()`**：保证所有数据写**先于**"序号恢复偶数"被看到。

### 读侧

```c
static inline int do_read_seqcount_retry(const seqcount_t *s, unsigned start)  /* :446 */
{
	smp_rmb();
	return do___read_seqcount_retry(s, start);      /* READ_ONCE(s->sequence) != start */
}
```

⭐ **屏障在 retry 里，不在 begin 里**（`read_seqcount_begin` 内部的 `smp_rmb()` 是
`raw_read_seqcount_begin` 加的，见 §5）。这个位置是有讲究的：

- 读侧必须保证"**读数据**"发生在"**再读一次 seq**"**之前**，
  所以 `smp_rmb()` 放在第二次读 seq 之前；
- 它与写者 **end 侧**的 `smp_wmb()` 配对。

---

## 3. ⭐ `sequence` 不是 `atomic_t`，就是个 `unsigned`

这是最容易误解的一点。看读写两边的实际语句：

```c
s->sequence++;                              /* 写侧：普通自增，非原子 */
__seq = READ_ONCE(s->seqcount.sequence);    /* 读侧：READ_ONCE，非 atomic_read */
```

| 常见误解 | v6.6 真相 |
|---------|----------|
| "`sequence` 是 `atomic_t`，所以不用加锁" | ❌ 它是 `unsigned`。头文件只 `#include <linux/compiler.h>` 等，**没有** `atomic.h` 语义 |
| "自增是原子的，所以安全" | ⚠️ **`s->sequence++` 本身不是原子 RMW**。它安全是因为**写者之间已被 spinlock 互斥**，绝不会有两个写者同时自增 |
| "读者之间也要互斥" | ❌ 读者只读，多个读者并发读 `unsigned` 没有撕裂问题（对齐的单字长访问） |

**所以 seqlock 的免锁性来自三件事的叠加**：

1. **写者互斥**由 spinlock（或外部锁 / `preempt_disable()`）保证 → 自增不会交错；
2. **单字长访问不撕裂** → 读者读到的是某个完整版本的值，不会读到"半个字"；
3. **屏障**保证版本与数据的配对可见顺序。

> **推论（也是坑）**：如果你**去掉写者互斥**又用 seqcount，
> 两个写者并发 `s->sequence++` 就可能丢一次自增 → 序号**永久停在一个奇数上** →
> 所有读者**永远重试，系统死锁**。
> `u64_stats_sync.h` 的注释把这条列为使用约束第 1 条（见 §11）。

---

## 4. 写侧变体表：怎么选 `_bh` / `_irq` / `_irqsave`

`seqlock_t` 的写侧（`write_seqlock()`，`:865`）**先拿内嵌 spinlock，
再动 seqcount**：

```c
static inline void write_seqlock(seqlock_t *sl)
{
	spin_lock(&sl->lock);
	do_write_seqcount_begin(&sl->seqcount.seqcount);
}
```

> 注意用的是 `do_write_seqcount_begin()`（内部版）而不是公开的
> `write_seqcount_begin()`。头文件注释（`:858`）说明了原因：
> **"no redundant lockdep_assert_held() checks are added"** ——
> 因为 `spin_lock()` 已经让 lockdep 知道了锁被持有，再断言一次是浪费。

选择判据**只取决于读者会出现在哪些上下文**：

| 变体 | 额外做了什么 | 什么时候用 |
|------|------------|-----------|
| `write_seqlock()` | 仅 spin_lock | 读者**只**在进程上下文 |
| `write_seqlock_bh()` | `spin_lock_bh()` | 读者（或其它写者）可能在 **softirq / tasklet / 下半部** |
| `write_seqlock_irq()` | `spin_lock_irq()` | 读者可能在**硬中断**上半部 |
| `write_seqlock_irqsave()` | `spin_lock_irqsave()` | 同上，且调用点的中断状态未知（需要恢复） |

⭐ **判据不是"我这边会不会被中断"，而是"读者会不会在中断里跑"**。
因为读者是自旋重试的：如果读者在中断里、写者不关中断，
那么中断一打进来，读者就会看到奇数序号并**在中断上下文里自旋**
——而写者被中断抢占了，根本跑不完 → **死锁**。

对应的读侧排他变体也一样成对提供：`read_seqlock_excl_bh()` /
`_irq()` / `_irqsave()`（§7）。

### 裸 `seqcount_t` 的写侧要自己管抢占

```c
#define raw_write_seqcount_begin(s)					\
do {									\
	if (seqprop_preemptible(s))					\
		preempt_disable();					\
									\
	do_raw_write_seqcount_begin(seqprop_ptr(s));			\
} while (0)
```

`seqprop_preemptible(s)` 是个**编译期常量**（`_Generic` 分发到各
`seqcount_LOCKNAME_t` 的 `__seqprop_##lockname##_preemptible()`）：

| seqcount 类型 | `preemptible` | 写侧是否自动 `preempt_disable()` |
|--------------|--------------|-------------------------------|
| `seqcount_t` | false | ❌ **你自己负责**（文档要求"必须显式关抢占"） |
| `seqcount_raw_spinlock_t` | false | ❌（raw spinlock 本身就关抢占） |
| `seqcount_spinlock_t` | `__SEQ_RT` = `IS_ENABLED(CONFIG_PREEMPT_RT)` | 非 RT 否 / **RT 上是** |
| `seqcount_rwlock_t` | `__SEQ_RT` | 同上 |
| `seqcount_mutex_t` | **true** | ✅ 自动关（mutex 不隐式关抢占） |

⭐ 这张表解释了为什么"用 `seqlock_t` 更省心"——文档原话：
"If it's desired to automatically handle the sequence counter writer
serialization and non-preemptibility requirements, use seqlock_t instead."

---

## 5. 读侧四个 begin 变体：等不等偶数、带不带屏障

| API | 等偶数？（自旋） | `smp_rmb()` | lockdep | 适用 |
|-----|----------------|-----------|---------|------|
| `read_seqcount_begin()`（`:351`） | ✅ | ✅ | ✅ | **默认**，通用 |
| `raw_read_seqcount_begin()`（`:342`） | ✅ | ✅ | ❌ | 明确不想付 lockdep 代价 |
| `__read_seqcount_begin()`（`:325`） | ✅ | ❌ | ❌ | ⚠️ 调用者**自己**保证后续有屏障 |
| `raw_seqcount_begin()`（`:395`） | ❌ | ✅（在 `raw_read_seqcount` 里） | ❌ | 极热的短读段，**用一次必然失败的重读换掉一条分支指令** |

`__read_seqcount_begin` 的自旋（`:325`）：

```c
#define __read_seqcount_begin(s)					\
({									\
	unsigned __seq;							\
									\
	while ((__seq = seqprop_sequence(s)) & 1)			\
		cpu_relax();						\
									\
	kcsan_atomic_next(KCSAN_SEQLOCK_REGION_MAX);			\
	__seq;								\
})
```

⭐ **注意这个"等偶数"的自旋是无界的**——它只 `cpu_relax()`，没有退避、
没有超时、没有抢占点。这正是 §10 里"写侧绝不能被抢占"的原因。

`raw_seqcount_begin()` 的写法很妙（`:395`）：

```c
	/*
	 * If the counter is odd, let read_seqcount_retry() fail
	 * by decrementing the counter.
	 */
	raw_read_seqcount(s) & ~1;
```

**不等待，直接把最低位清掉**。如果此刻是奇数，清掉后得到一个偶数，
但它**不等于**任何真实的稳定值 → 结尾的 `read_seqcount_retry()` 必然判定失败 → 重试。
代价：省掉一条分支指令；收益场景：源码注释说得很克制——
"**Use this only in special kernel hot paths where the read section is
small and has a high probability of success through other external means.**"

对应的"不带屏障"版 retry 也有：`__read_seqcount_retry()`（`:423`），
注释要求"Callers should ensure that `smp_rmb()` or equivalent ordering is
provided"。**这一对 `__` 前缀 API 就是给"我自己排屏障"的专家用的**，
普通代码别碰。

---

## 6. ⭐ 三类读者 —— 第二种很多人不知道

`Documentation/locking/seqlock.rst` 把读者分成三类，v6.6 全部实现：

### ① 普通读者（无锁，可能重试）

```c
do {
	seq = read_seqbegin(&foo_seqlock);
	/* ... 读数据 ... */
} while (read_seqretry(&foo_seqlock, seq));
```

**写者从不等待普通读者**——这是 seqlock "偏袒写者"的字面含义。

### ② 加锁读者 `read_seqlock_excl()`（`:1012`）

```c
static inline void read_seqlock_excl(seqlock_t *sl)
{
	spin_lock(&sl->lock);
}
```

⭐ **就一行 `spin_lock()`，完全不碰 seqcount！** 源码注释：

> "A locking reader exclusively locks out *both* other writers *and* other
> locking readers, but **it does not update the embedded sequence number**.
> Locking readers act like a normal `spin_lock()`/`spin_unlock()`."

两个关键推论：

1. **加锁读者与普通读者不互斥**——普通读者照样能在加锁读者持有 spinlock 期间
   无锁读（它只看 seqcount，而 seqcount 没变）。
   ⚠️ 所以加锁读者**不能**用来"阻止别人读"，只能阻止**写者**和**其它加锁读者**。
2. **`read_seqlock_excl()` 之间是完全互斥的**，不像 rwlock 那样允许多个读者并发。
   文档原话："Unlike `rwlock_t`, only one locking reader can acquire it."
   —— **它是"排他的读者"，不是"共享的读者"**。

### ③ 混合读者 `read_seqbegin_or_lock()`（`:1140`）

**先乐观无锁试一次，失败就退化成加锁读者**——用于"写活动突然爆发时
无锁读者会饿死（重试循环太多）"的场景：

```c
	int seq = 0;                       /* ⭐ 必须初始化为偶数 */
	do {
		read_seqbegin_or_lock(&foo_seqlock, &seq);
		/* ... 读数据 ... */
	} while (need_seqretry(&foo_seqlock, seq));
	done_seqretry(&foo_seqlock, seq);
```

实现（`:1140`）：

```c
static inline void read_seqbegin_or_lock(seqlock_t *lock, int *seq)
{
	if (!(*seq & 1))	/* Even  → 无锁模式，seq 存序号值 */
		*seq = read_seqbegin(lock);
	else			/* Odd   → 加锁模式，seq 只当标记用 */
		read_seqlock_excl(lock);
}

static inline int need_seqretry(seqlock_t *lock, int seq)
{
	return !(seq & 1) && read_seqretry(lock, seq);   /* 加锁模式永不重试 */
}

static inline void done_seqretry(seqlock_t *lock, int seq)
{
	if (seq & 1)
		read_sequnlock_excl(lock);                /* 加锁模式才解锁 */
}
```

⭐ **`seq` 这个变量被"重载"了两种语义**，靠奇偶区分：

| `seq` 的值 | 含义 | 当前处于 |
|-----------|------|---------|
| 偶数 | 上一次 `read_seqbegin()` 拿到的**序号值** | 无锁模式 |
| 奇数 | 一个**标记**（表示"我已经退化成加锁读者了"） | 加锁模式 |

状态迁移：

```
第 1 次:  seq = 0（偶数）   → read_seqbegin()        → 无锁读
          need_seqretry():  读到奇数 → 返回 true     → 重试
第 2 次:  seq = 奇数        → read_seqlock_excl()    → ⭐ 退化成加锁读
          need_seqretry():  !(seq&1) 为假 → 返回 false → 退出循环
          done_seqretry():  seq 为奇数 → read_sequnlock_excl()
```

**为什么这样就不会饿死了**：加锁读者会挡住写者，写者一旦被挡住，
`sequence` 就稳定下来，加锁读者这一次读必然成功、无需重试。
**代价是这一次读退化成了 spinlock 临界区**（会阻塞写者）。

---

## 7. ⭐ 关联锁 seqcount：`seqcount_LOCKNAME_t` 与它的版本断崖

### 它解决什么问题

裸 `seqcount_t` 的写侧需要外部锁保证互斥，但**编译器和 lockdep 都不知道**这层关系。
`seqcount_LOCKNAME_t` 把"哪把锁负责写者串行化"**在初始化时记录下来**，
让 lockdep 能自动校验。

头文件注释（`:120`）：

```c
/*
 * A sequence counter which associates the lock used for writer
 * serialization at initialization time. This enables lockdep to validate
 * that the write side critical section is properly serialized.
 */
```

### 开销：lockdep 关掉时是**零**

```c
#if defined(CONFIG_LOCKDEP) || defined(CONFIG_PREEMPT_RT)
#define __SEQ_LOCK(expr)	expr
#else
#define __SEQ_LOCK(expr)
#endif
```

`__SEQ_LOCK()` 在**非 lockdep 且非 RT** 的构建下展开成**空**——
那个 `locktype *lock` 字段**根本不存在于结构体里**。
文档原话："This lock association is a NOOP if lockdep is disabled and has
neither storage nor runtime overhead."

> ⭐ **这是内核里"调试信息零成本抽象"的教科书范例**——
> 同一份代码，开调试时多一个指针 + 断言，关调试时字段直接消失。

### v6.6 实际有的 4 种（`:274`）

```c
SEQCOUNT_LOCKNAME(raw_spinlock, raw_spinlock_t,  false,    s->lock,        raw_spin, raw_spin_lock(s->lock))
SEQCOUNT_LOCKNAME(spinlock,     spinlock_t,      __SEQ_RT, s->lock,        spin,     spin_lock(s->lock))
SEQCOUNT_LOCKNAME(rwlock,       rwlock_t,        __SEQ_RT, s->lock,        read,     read_lock(s->lock))
SEQCOUNT_LOCKNAME(mutex,        struct mutex,    true,     s->lock,        mutex,    mutex_lock(s->lock))
```

（`SEQCOUNT_LOCKNAME(lockname, locktype, preemptible, lockmember, lockbase, lock_acquire)`，
第 3 个参数就是 §4 那张表的 `preemptible`。）

### ⭐ 版本断崖：从 v5.10 的 5 种，到 v6.6 的 4 种

| 版本 | `SEQCOUNT_LOCKNAME` 家族 | `ww_mutex` 计数 | 说明 |
|------|-------------------------|----------------|------|
| **v5.9** | **0** | — | ⭐ **关联锁机制尚未引入** |
| **v5.10** | 5 种（含 `ww_mutex`） | 4 | ⭐ **v5.10 引入**；与头文件版权 "(C) 2020 Linutronix GmbH" 吻合 |
| v5.15 / v5.16 / v5.17 / v5.18 | 5 种 | 4~5 | 稳定期 |
| **v5.19** | **4 种** | **0** | ⭐ **`seqcount_ww_mutex_t` 在此被删** |
| v6.0 / v6.1 / v6.2 / v6.3 | 4 种 | 0 | |
| **v6.6** | 4 种 | **0** | 与 v5.19 起一致 |

**判定方法**：抓多个 tag 的同名文件做 `grep -c 'ww_mutex'` + 字节数对比。
v5.18 → v5.19 的字节数从 39100 掉到 38820，同时 `ww_mutex` 计数从 5 归零。

### ⚠️ 官方文档是过期的（第 15 条"凭记忆必错"）

`Documentation/locking/seqlock.rst:110-116` 至今列着 **5 种**：

```
  - ``seqcount_spinlock_t``
  - ``seqcount_raw_spinlock_t``
  - ``seqcount_rwlock_t``
  - ``seqcount_mutex_t``
  - ``seqcount_ww_mutex_t``      ← ⚠️ v6.6 里【不存在】
```

实测：**`seqcount_ww_mutex_t` 这个类型名在 v6.6 的整个 `seqlock.h` 里 grep 不到**，
`include/linux/ww_mutex.h`（13KB）里也**完全没有**。
只剩下一个孤儿宏：

```c
#define SEQCNT_WW_MUTEX_ZERO(name, lock) 	SEQCOUNT_LOCKNAME_ZERO(name, lock)   /* :294 */
```

它是 `SEQCOUNT_LOCKNAME_ZERO` 的别名，而后者对任何 `seqcount_LOCKNAME_t` 都成立——
**所以这个宏本身没坏，只是它暗示的那个类型已经没了。**

> 📌 **教训**：官方文档也会滞后。**类型和宏是否真实存在，以头文件为准，不以 .rst 为准。**
> 这与 10.4 里 `DEFINE_SEMAPHORE` 从 v6.4 起要两个参数是同一类问题——
> 文档/记忆滞后于代码。

---

## 8. `seqcount_latch_t`：双缓冲 MVCC，每次 **+1 而不是 +2**

```c
typedef struct {
	seqcount_t seqcount;
} seqcount_latch_t;                                   /* :647 */

static inline void raw_write_seqcount_latch(seqcount_latch_t *s)   /* :780 */
{
	smp_wmb();	/* prior stores before incrementing "sequence" */
	s->seqcount.sequence++;                       /* ⭐ +1，不是 +2 */
	smp_wmb();      /* increment "sequence" before following stores */
}
```

### 与普通 seqlock 的三点根本差异

| | 普通 `seqlock_t` | `seqcount_latch_t` |
|--|-----------------|-------------------|
| 每次写入的增量 | **+2**（begin 一次、end 一次） | **+1**（只调一次 `raw_write_seqcount_latch`） |
| 奇偶的含义 | 奇 = **写入中，读者要重试** | 奇/偶 = **读哪一份副本的索引** |
| 读者被打断 | ❌ 危险（会自旋一个 tick） | ✅ **安全**——这是它的设计目的 |

### 工作机制（MVCC）

```
struct latch_struct {
	seqcount_latch_t seq;
	struct data_struct data[2];      /* ⭐ 双份存储 */
};

/* 写者（外部已串行化） */
smp_wmb();
latch->seq.sequence++;      /* 切到 data[1] */
smp_wmb();
modify(latch->data[0]);     /* 改那份【没人读】的副本 */
smp_wmb();
latch->seq.sequence++;      /* 切回 data[0] */
smp_wmb();
modify(latch->data[1]);     /* 改另一份 */

/* 读者 */
do {
	seq = raw_read_seqcount_latch(&latch->seq);
	idx = seq & 0x01;                        /* ⭐ 用最低位选副本 */
	entry = data_query(latch->data[idx]);
} while (raw_read_seqcount_latch_retry(&latch->seq, seq));
```

读者永远被指向"**当前稳定的那一份**"，写者改的是"**另一份**"。
所以**读者可以在写者写了一半的时候安全地插进去读**——
它读的是另一份副本。

### 什么时候需要它

文档原文：

> "Use `seqcount_latch_t` when the write side sections cannot be protected
> from interruption by readers. This is typically the case when the read
> side can be invoked from **NMI handlers**."

⭐ **NMI 是不可屏蔽的，你没法用 `write_seqlock_irq()` 挡住它**
（`local_irq_disable()` 也挡不住 NMI）。所以只要读者可能在 NMI 里跑，
普通 seqlock 就不成立，只能用 latch。

**代价**：存储翻倍。源码注释：
"the trade-off is doubling the cost of storage; we have to maintain two
copies of the entire data structure."

**还有两个限制**（源码 NOTE / NOTE2）：
- 动态数据结构里**新增条目**的发布**不**被 latch 保护——一次迭代可能跨过整个修改过程；
- 动态对象的**生命周期**要另外用 **RCU** 管。

---

## 9. ⭐ 三条致命约束（违反就是活锁，不是"结果不对"）

### 约束 ①：保护的数据里**不能有指针**

`Documentation/locking/seqlock.rst:33`：

> "This mechanism cannot be used if the protected data contains pointers,
> as the writer can invalidate a pointer that the reader is following."

这是**最容易被忽略**的一条。seqlock 只保证"你读到的字节没有撕裂"，
**不保证对象的生命周期**。读者拿着指针往下走的时候，
写者可能已经把它 `kfree()` 了 → **use-after-free**。

> 数据要带指针 → 用 **RCU**（RCU 同时管生命周期和一致性）。

### 约束 ②：写侧**绝不能**被抢占或被读者中断

`Documentation/locking/seqlock.rst:27`：

> "A sequence counter write side critical section must never be preempted
> or interrupted by read side sections. Otherwise the reader will spin for
> the entire scheduler tick due to the odd sequence count value and the
> interrupted writer. **If that reader belongs to a real-time scheduling
> class, it can spin forever and the kernel will livelock.**"

后果递进：

| 场景 | 后果 |
|------|------|
| 写者被普通读者抢占 | 读者自旋**整整一个调度 tick**（`__read_seqcount_begin` 里的 `while (seq & 1) cpu_relax()` 没有退避） |
| 读者是 RT 调度类 | **RT 任务优先级高于写者 → 写者永远跑不完 → 永久活锁** |
| 读者在 NMI | 写者挡不住（NMI 不可屏蔽）→ 必须用 `seqcount_latch_t` |

对应到 API 就是 §4 那张表：**要么用会关抢占的锁（spinlock），
要么显式 `preempt_disable()`，要么用 `seqlock_t`。**

### 约束 ③：写者之间必须互斥（否则序号永久卡在奇数）

见 §3 的推论。`u64_stats_sync.h:16` 把这条列为使用约束第 1 条：

> "1) Write side must ensure mutual exclusion, or one seqcount update could
> be lost, **thus blocking readers forever**."

---

## 10. RT 上为什么要用 `seqcount_spinlock_t`：那套 lock+unlock 技巧

回到 §1 埋的问题。PREEMPT_RT 上 `spinlock_t` 变成**可睡眠的 rt_mutex**，
于是 `seqlock_t` 的写侧**不再隐式关抢占** —— 直接违反约束 ②，读者会活锁。

v6.6 的解法写在头文件注释里（`:145`）：

> "To remain preemptible while avoiding a possible livelock caused by the
> reader preempting the writer, use a different technique: **let the reader
> detect if a `seqcount_LOCKNAME_t` writer is in progress. If that is the
> case, acquire then release the associated LOCKNAME writer serialization
> lock.** This will allow any possibly-preempted writer to make progress."

对应代码就在 `__seqprop_##lockname##_sequence()`（`:212`）：

```c
	unsigned seq = READ_ONCE(s->seqcount.sequence);

	if (!IS_ENABLED(CONFIG_PREEMPT_RT))
		return seq;

	if (preemptible && unlikely(seq & 1)) {
		__SEQ_LOCK(lock_acquire);                  /* 拿一下关联锁 */
		__SEQ_LOCK(lockbase##_unlock(s->lock));    /* 立刻放掉 */

		/*
		 * Re-read the sequence counter since the (possibly
		 * preempted) writer made progress.
		 */
		seq = READ_ONCE(s->seqcount.sequence);
	}

	return seq;
```

⭐ **"读者去帮写者跑完"** —— 读者发现序号是奇数（有写者在进行中），
就**申请并立刻释放**那把关联锁。如果写者正被抢占、卡在锁的临界区里，
读者这次 lock+unlock 会**让写者被唤醒并跑完**（rt_mutex 的语义：
释放时把锁交给等待者）。写者跑完，序号恢复偶数，读者再读就不再自旋。

**为什么 `preemptible` 参数在这里是判据**：只有当关联锁**是可抢占类型**
（RT 上的 `spinlock_t` / `rwlock_t` / 永远是 `mutex`）时这套技巧才需要。
`raw_spinlock_t` 在 RT 上也关抢占，不需要。

而且注意 `__seqprop_##lockname##_preemptible()`（`:232`）在 RT 上**返回 false**：

```c
	if (!IS_ENABLED(CONFIG_PREEMPT_RT))
		return preemptible;

	/* PREEMPT_RT relies on the above LOCK+UNLOCK */
	return false;
```

注释直白："PREEMPT_RT relies on the above LOCK+UNLOCK"——
RT 上既然已经用 lock+unlock 技巧解决了，写侧就**不需要**再 `preempt_disable()` 了。

> 📌 这套技巧在 `Documentation/locking/locktypes.rst` 里被要求
> "must be implemented for all of PREEMPT_RT sleeping locks"——
> **它是 PREEMPT_RT 的通用模式，不止 seqlock 用。**

---

## 11. 真实用例：`u64_stats_sync` 在 **64 位上是空操作**

网络子系统到处在用的 `struct u64_stats_sync`
（`include/linux/u64_stats_sync.h`，v6.6）就是 seqcount 的一层包装：

```c
typedef struct {
#if BITS_PER_LONG == 32 && defined(CONFIG_SMP)
	seqcount_t	seq;
#endif
} struct u64_stats_sync;                 /* :66 附近，SMP+32位才有内容 */
```

| 架构 | `struct u64_stats_sync` | `u64_stats_fetch_begin()` |
|------|------------------------|--------------------------|
| **32 位 SMP** | 含 `seqcount_t seq` | 真 `read_seqcount_begin()`（`:170`） |
| **32 位 UP** | 空 | 空 |
| **64 位**（x86-64 / ARM64） | **空** | **空**（直接读，不重试） |

头文件开头就把目的写清楚了：

> "Protect against 64-bit values **tearing on 32-bit architectures**.
> ... - Use a seqcount on 32-bit
> ... - **The whole thing is a no-op on 64-bit architectures.**"

⭐ **对 HFT 的直接意义**：在 x86-64 上，`u64_stats_*` 系列**零开销**——
因为它们要解决的是"32 位上 64 位变量读撕裂"，而 x86-64 上对齐的 64 位读写
本来就不撕裂。**不要以为你在用 seqlock 保护 64 位计数器，其实什么都没做**
（反过来说：在 64 位上你也不需要它）。

### 它的五条使用约束（头文件原文，逐条都是坑）

| # | 约束 | 违反后果 |
|---|------|---------|
| 1 | **写侧必须保证互斥** | 一次 seqcount 更新丢失 → **读者永久阻塞** |
| 2 | **写侧必须关抢占** | seqcount 读者抢占写者 → **双方永久自旋** |
| 3 | 若有 IRQ 上下文的读者/写者，写侧必须用 `_irqsave()` 变体 | 中断里自旋 + 写者被中断卡住 → 死锁 |
| 4 | 一次读多个计数器时，**只能保证"每个变量内部一致"，不保证"几个变量之间一致"** | 语义误用 |
| 5 | 读者可以睡眠 / 被抢占 / 被中断（纯读） | — |

**第 4 条特别重要**，很多统计代码的 bug 就出在这：

```
正确理解：  fetch_begin → 读 bytes → 读 packets → fetch_retry
            ✅ bytes 自身不撕裂、packets 自身不撕裂
            ❌ 但 bytes 和 packets 可能来自【不同的写入批次】

要"几个字段互相一致"？→ 不能靠多次 fetch，必须把它们放进【同一个】
                        seqlock 读临界区（这也是本篇 §"适用数据"里
                        "配置快照"适合 seqlock 的原因）
```

---

## HFT / 嵌入式关联

### seqlock 的延迟画像：读者零阻塞，写者零等待

| | 读者 | 写者 |
|--|------|------|
| 阻塞别人？ | ❌（不修改任何共享变量） | ✅（spinlock 挡其它写者） |
| 被别人阻塞？ | ⚠️ **可能被写者拖进重试循环** | ❌ **从不等待任何读者** |
| 最坏延迟 | **无界**（写者不停 → 读者一直重试） | 有界（等于其它写者的临界区之和） |

⭐ **seqlock 换来的是"写者的延迟确定"，代价是"读者的延迟不确定"**。
这和 10.3 的结论正好反过来——**v6.6 的 qrwlock 是 FIFO 公平的**（读者不饿写者），
而 seqlock **明确偏袒写者**（文档标题就写着 "no writer starvation"）。

**选型判据**：

| 你要保证谁的延迟 | 选 |
|-----------------|---|
| 写者绝不能被读者拖慢（如：时钟更新、行情最新价刷新） | **seqlock** |
| 读者绝不能被写者饿死 | rwlock（v6.6 的 qrwlock 是公平的） |
| 两者都要 | 重新设计（减少共享 / RCU / 分片） |

### 经典 HFT 用例对照

| 场景 | 为什么适合 seqlock |
|------|------------------|
| **行情最新价 / 快照** | 写极频繁（每笔 tick）但极短；读者（策略）绝不能阻塞写者 |
| **全局配置版本号** | 写极少（改配置时）；读者极多（每个决策点读）；读段极短 |
| **时钟 / 时间源** | 内核里 seqlock 的**原始用例**（x86-64 vsyscall gettimeofday，见头文件版权行） |
| **统计计数器** | `u64_stats_sync` 就是为此设计的（但见 §11：64 位上是空操作） |

### ⚠️ 用户态 seqlock 的两个 C++ 陷阱

原速查表 Q3 给了一段 C++ 实现，思路对，但有两处需要在 v6.6 视角下订正
（详见自测题 Q3 的修订块），这里先给结论：

| # | 陷阱 | 说明 |
|---|------|------|
| 1 | **`ts.value` 是非原子变量却被并发写** | 在 C++ 里这是 **data race → UB**；编译器可以据此做任何优化。内核靠 `READ_ONCE`/`WRITE_ONCE` 规避（它们本质是 `volatile` 强制每次访存 + 保持字长对齐），C++ 侧**必须**用 `std::atomic` 的 `relaxed` 加载/存储，或至少 `volatile` + 注释说明 |
| 2 | **写端第一个 store 用 `release` 是不够的** | `release` 只保证"**之前**的读写不会被重排到它之后"，而 seqlock 写 begin 需要的是"**之后**的数据写不会被重排到 seq 自增之前" → 那是 **`acquire` 语义的方向**。内核的做法是 `s->sequence++; smp_wmb();`（自增 + 全屏障）。C++ 正确写法：<br>`seq.store(s+1, relaxed); std::atomic_thread_fence(release); value = ...; std::atomic_thread_fence(release); seq.store(s+2, release);` |

（在 x86 上因为 store-store 不重排，原写法"碰巧能跑"；
在 ARM64 上就可能出错——**而这正是嵌入式移植最容易翻车的地方**。）

### 嵌入式：ARM 上的屏障代价

| 架构 | `smp_rmb()` 实际指令 | 大致代价 |
|------|---------------------|---------|
| x86-64 | **空**（TSO 内存模型） | 0 |
| ARM64 | `ldar` / `dmb ishld` | 十几 ~ 几十周期 |
| ARM32 | `dmb` | 更贵 |

**含义**：seqlock 的读侧每次要**两次** `smp_rmb()`（begin 一次、retry 一次），
在非 x86 上这是真金白银的开销。所以：

> **在 ARM 嵌入式上，seqlock 的"无锁"优势比在 x86 上小得多。**
> 如果读段本身只有一两个字段，直接 `local_lock()` / 关抢占
> 或者干脆用 per-CPU 数据可能更划算。**先量再选。**

### 观测：怎么知道读者在狂重试

| 手段 | 做法 |
|------|------|
| **KCSAN** | v6.6 的 seqlock 内建 KCSAN 支持（`kcsan_atomic_next(KCSAN_SEQLOCK_REGION_MAX)`、`kcsan_flat_atomic_begin/end`）。开启后 KCSAN 会把整个读临界区当成一个"原子区"来分析，能抓出"读段里混了不该有的共享写" |
| **手工计数** | 在 `do { } while (read_seqretry(...))` 外面加一个 per-CPU 重试计数器，`/proc` 或 debugfs 导出。**重试率 > 1% 就该换方案** |
| **ftrace** | 给写者函数挂 `function_graph`，看写临界区时长。**写临界区变长 → 读者重试率立刻上升** |
| **BPF** | 对写者函数做直方图（` bpf` 的 `hist`），关注 P99 而不是均值 |

---

## 实践模板

### 模板 A：内核里的基本用法（`seqlock_t`）

```c
static DEFINE_SEQLOCK(md_seq);          /* :819，静态初始化 */
static struct market_data {
	u64 last_px, last_qty;
	u64 update_ns;
} ____cacheline_aligned md;

/* ---- 写者（行情回调，要求延迟确定）---- */
static void on_tick(u64 px, u64 qty)
{
	write_seqlock(&md_seq);                     /* spin_lock + seq++ */
	md.last_px  = px;
	md.last_qty = qty;
	md.update_ns = ktime_get_ns();
	write_sequnlock(&md_seq);                   /* seq++ + spin_unlock */
}

/* ---- 读者（策略决策，允许重试）---- */
static bool read_snapshot(u64 *px, u64 *qty, u64 *ns)
{
	unsigned seq;
	do {
		seq = read_seqbegin(&md_seq);
		*px = md.last_px;                   /* ⚠️ 整段读都要在 begin/retry 之间 */
		*qty = md.last_qty;
		*ns  = md.update_ns;
	} while (read_seqretry(&md_seq, seq));
	return true;
}
```

⚠️ 如果读者可能在 **softirq**（如 NAPI poll）里跑 → 写者必须换成
`write_seqlock_bh()`；硬中断里 → `write_seqlock_irqsave()`（§4 的表）。

### 模板 B：写者爆发时防读者饿死

```c
	int seq = 0;                                  /* ⭐ 必须是偶数 */
	do {
		read_seqbegin_or_lock(&md_seq, &seq);
		*px  = md.last_px;
		*ns  = md.update_ns;
	} while (need_seqretry(&md_seq, seq));
	done_seqretry(&md_seq, seq);                  /* ⭐ 别忘了，加锁模式在这里解锁 */
```

判据：**先测重试率**。无锁读段的重试率超过 ~1% 才值得上这个模板，
因为它会让尾部请求退化成 spinlock 临界区。

### 模板 C：NMI 里要读 → 只能用 latch

```c
struct latch_snap {
	seqcount_latch_t seq;
	struct market_data data[2];                /* ⭐ 双份 */
};

/* 写者（外部已串行化）*/
raw_write_seqcount_latch(&snap.seq);          /* +1，切到另一份 */
modify(snap.data[idx_of_inactive_copy]);

/* 读者（可以在 NMI 里）*/
unsigned seq, idx;
do {
	seq = raw_read_seqcount_latch(&snap.seq);
	idx = seq & 1;
	px = snap.data[idx].last_px;
} while (raw_read_seqcount_latch_retry(&snap.seq, seq));
```

### 迁移自检清单

| # | 检查 | 不通过怎么办 |
|---|------|------------|
| 1 | 保护的数据里有**指针**吗？ | 有 → 不能用 seqlock，用 RCU |
| 2 | 写侧会被抢占吗？ | 用 `seqlock_t`；裸 seqcount 要显式 `preempt_disable()` |
| 3 | 读者会在 IRQ / softirq / **NMI** 里跑吗？ | IRQ → `_irq` 变体；**NMI → 只能用 `seqcount_latch_t`** |
| 4 | 写者之间互斥了吗？ | 裸 seqcount 必须外部串行化，否则序号卡奇数、读者永久阻塞 |
| 5 | 读段有副作用吗（计数、日志、分配）？ | 有 → 重试会重复执行，要挪到循环外 |
| 6 | 读段会不会太长？ | 长 → 重试代价高，考虑 RCU |
| 7 | 目标平台是 ARM 吗？ | 是 → 读侧两次 `smp_rmb()` 有真实开销，先量测再决定 |
| 8 | 会不会跑在 `PREEMPT_RT` 上？ | 会 → 必须用 `seqlock_t`（内部是 `seqcount_spinlock_t`），别用裸 `seqcount_t` |

---

## 易错点核对表

| # | 易错点 | 正确做法 |
|---|--------|---------|
| 1 | 以为 `seqlock_t` 里是裸 `seqcount_t` | ❌ v6.6 是 `seqcount_spinlock_t`（RT 防读者饿写者） |
| 2 | 以为 `sequence` 是 `atomic_t` | ❌ 就是 `unsigned`，靠写者互斥 + `READ_ONCE` + 屏障 |
| 3 | 用 seqlock 保护含指针的数据 | ❌ 写者可能 `kfree()` 掉读者正跟着的指针 → UAF。用 RCU |
| 4 | 写侧不关抢占 | ❌ 读者会自旋一个 tick；RT 读者 → **活锁** |
| 5 | 读者在 NMI 里还用普通 seqlock | ❌ NMI 挡不住。用 `seqcount_latch_t` |
| 6 | 照抄文档用 `seqcount_ww_mutex_t` | ❌ v6.6 没有这个类型（v5.19 删），只剩孤儿宏 |
| 7 | 以为 `read_seqlock_excl()` 会更新 seq | ❌ 就一行 `spin_lock()`，不碰 seqcount |
| 8 | 以为多个 `read_seqlock_excl()` 能并发 | ❌ 它是**排他**读者，同时只能一个 |
| 9 | `read_seqbegin_or_lock()` 的 `seq` 不初始化为偶数 | ❌ 必须初始化为偶数，否则第一次就退化成加锁读 |
| 10 | `read_seqbegin_or_lock()` 之后忘了 `done_seqretry()` | ❌ 加锁模式下不调就**永久持锁** |
| 11 | 用 `u64_stats_*` 以为在 64 位上也有保护 | ❌ 64 位上是**空操作**（本来就不需要） |
| 12 | 多个字段分多次 `u64_stats_fetch_*` 以为互相一致 | ❌ 只保证每个变量内部不撕裂 |
| 13 | 用户态 C++ 用 `release` store 当 seqlock 写 begin | ❌ 方向反了，需要 release **fence**；x86 上碰巧能跑，ARM 上翻车 |
| 14 | 拿 `__read_seqcount_begin()` 当通用 API | ❌ 它**不带** `smp_rmb()`，调用者必须自己补屏障 |

---

## 常见陷阱

1. 以为 seqlock 是通用读写锁——只适合写少读多 + 读端可容忍重试的场景
2. 在读端忽略 sequence 检查——seqlock 读端必须检查前后 sequence 一致，否则可能读到半写状态
3. 在写端用多个步骤——写端持锁期间应尽快完成，持锁时间 = 写者重试窗口
4. **（v6.6 补充）** 用 seqlock 保护含指针的结构体 → use-after-free
5. **（v6.6 补充）** 裸 `seqcount_t` 写侧不关抢占 → 读者自旋一个 tick，RT 上活锁
6. **（v6.6 补充）** 以为 `read_seqlock_excl()` 是"共享读锁" → 它是**排他**的，且不动 seq
7. **（v6.6 补充）** 在 NMI 读者场景用普通 seqlock → 必须用 `seqcount_latch_t`
8. **（v6.6 补充）** 按 `Documentation/locking/seqlock.rst` 列单用 `seqcount_ww_mutex_t` → v6.6 已删
9. **（v6.6 补充）** 在 ARM 上照搬 x86 的"seqlock 零开销"结论 → 两次 `smp_rmb()` 是真开销

---

## 自测题

<details>
<summary>自测题（点击展开）</summary>

**Q1.** seqlock 的工作原理？读写端各做什么？

<details><summary>答案</summary>

写端：`write_seqlock()` → sequence++（奇数）→ 写数据 → sequence++（偶数）→ `write_sequnlock()`。写端之间互斥（spinlock）。读端：`seq1 = read_seqbegin()` → 读数据 → `seq2 = read_seqretry(seq1)` → 如果 seq1 是奇数或 seq1 != seq2 → 重读。读端无锁（不阻塞写者），但可能需要重试。

<details><summary>按 v6.6 修订/补充</summary>

**主干完全正确**，补六个源码层面的细节：

**① `write_seqlock()` 的实际实现**（`seqlock.h:865`）：

```c
spin_lock(&sl->lock);
do_write_seqcount_begin(&sl->seqcount.seqcount);
```

注意第二行走的是**内部版** `do_write_seqcount_begin()`，不是公开的
`write_seqcount_begin()`——注释说明是为了"不重复做 lockdep 断言"。

**② 屏障位置**（§2），这是最常被漏掉的：

```c
/* begin */  s->sequence++;  smp_wmb();     /* ⭐ 屏障在自增【之后】 */
/* end   */  smp_wmb();  s->sequence++;     /* ⭐ 屏障在自增【之前】 */
```

方向相反，各司其职：begin 侧防止"数据写"被重排到 seq++ 之前；
end 侧防止"数据写"被重排到 seq++ 之后。读侧的 `smp_rmb()` 在
`read_seqcount_retry()` 里（`:446`），与写者 **end 侧**的 `smp_wmb()` 配对。

**③ `read_seqbegin()` 会先自旋等到偶数**（`__read_seqcount_begin`，`:325`）：

```c
while ((__seq = seqprop_sequence(s)) & 1)
	cpu_relax();
```

所以原答案里"seq1 是奇数 → 重试"在**默认 API 下其实不会发生**
（begin 已经等过了）。会拿到奇数的是那个"不等待"的变体
`raw_seqcount_begin()`（`:395`，用 `& ~1` 把最低位清掉，让结尾必然重试）。

**④ `sequence` 不是 `atomic_t`**（§3）。它就是 `unsigned`，
写侧 `s->sequence++` 是**普通自增**。它安全的前提是
**写者之间已被 spinlock 互斥**——去掉这个前提，两个写者并发自增
可能丢一次更新 → 序号**永久停在奇数** → 所有读者永久重试。

**⑤ `seqlock_t` 里装的不是裸 `seqcount_t`**（§1）而是 `seqcount_spinlock_t`，
目的是在 PREEMPT_RT 上防止读者饿死写者（机制见 §10）。

**⑥ 写端互斥靠的是内嵌 spinlock**，所以严格说"写端之间互斥（spinlock）"
只对 `seqlock_t` 成立；裸 `seqcount_t` 需要**外部**锁，且要自己 `preempt_disable()`。

</details>
</details>

**Q2.** seqlock 适合什么场景？不适合什么？

<details><summary>答案</summary>

适合：① 写极少读极多。② 读端可以容忍偶尔重试。③ 数据简单（几个字段，重读代价低）。典型：`jiffies`（时间戳）、`getnstimeofday()`、统计计数器。不适合：① 复杂数据结构（链表/树），重读代价高。② 写频繁（写者互斥 + 读者频繁重试）。③ 读端需要阻塞写者。这些场景用 RCU 或 rwlock。

<details><summary>按 v6.6 修订/补充</summary>

**三条"适合"、三条"不适合"都成立**，补三条源码硬约束（§9）——
违反它们不是"性能差"，是**活锁或 UAF**：

| # | 硬约束 | 违反后果 | 出处的原文 |
|---|--------|---------|-----------|
| 1 | 保护的数据**不能含指针** | 写者 `kfree()` 掉读者正跟着的指针 → **use-after-free** | `seqlock.rst:33` |
| 2 | 写侧**绝不能**被抢占/被读者中断 | 读者自旋一个 tick；**RT 读者 → 内核活锁** | `seqlock.rst:27` |
| 3 | 写者之间**必须**互斥 | 丢一次 seqcount 更新 → **读者永久阻塞** | `u64_stats_sync.h:16` |

**关于 `jiffies` / `getnstimeofday()` 这条举例要更新**：
`Documentation/locking/seqlock.rst` 开头写的是
"used for data that's rarely written to (**e.g. system time**)"，
而 seqlock 机制本身**起源于 x86-64 vsyscall 的 gettimeofday**
（`seqlock.h` 版权行："Based on x86_64 vsyscall gettimeofday:
Keith Owens, Andrea Arcangeli"）。所以这个例子是**史实正确**的。

但要注意 **v6.6 的 timekeeping 用的不是 `seqlock_t` 而是
`seqcount_raw_spinlock_t`**（`struct timekeeper` 的 seq 字段），
属于"裸 seqcount + 外部 raw spinlock"模式——因为 timekeeping
有自己的锁，不需要 seqlock 内嵌的那把。

**补两条文档没写、但工程上很关键的判据**：

1. **"写频繁"要量化**：判据不是"写多不多"，而是**读者的重试率**。
   加个 per-CPU 重试计数器，`重试率 > 1%` 就该换方案
   （或者上 `read_seqbegin_or_lock()` 这个混合读者，§6-③）。
2. **"读端需要阻塞写者"其实有办法**：用 `read_seqlock_excl()`（§6-②）。
   但要注意它是**排他**的——同时只能有一个加锁读者，且它**不阻止普通无锁读者**。

</details>
</details>

**Q3.** HFT 中 seqlock 的用户态实现？

<details><summary>答案</summary>

```c
// 无锁读取时间戳
struct { std::atomic<uint32_t> seq; uint64_t value; } ts;
// 写端
uint32_t s = ts.seq.load(std::memory_order_relaxed);
ts.seq.store(s + 1, std::memory_order_release);  // 奇数
ts.value = rdtsc();
ts.seq.store(s + 2, std::memory_order_release);  // 偶数
// 读端
uint32_t s1, s2; uint64_t v;
do {
    s1 = ts.seq.load(std::memory_order_acquire);
    v = ts.value;
    s2 = ts.seq.load(std::memory_order_acquire);
} while (s1 != s2 || s1 & 1);  // 重试
```

<details><summary>按 v6.6 修订/补充</summary>

**思路正确，但有两处会在 ARM64 上翻车。**

#### 陷阱 ①：`ts.value` 是非原子变量却并发读写 → C++ 里是 data race（UB）

内核靠 `READ_ONCE()` / `WRITE_ONCE()` 规避（本质是 `volatile` 强制每次访存
+ 保证访问宽度不被拆分）。C++ 侧必须二选一：

```cpp
// 方案 A（推荐）：用 atomic 的 relaxed 访问
struct { std::atomic<uint32_t> seq; std::atomic<uint64_t> value; } ts;
v = ts.value.load(std::memory_order_relaxed);
ts.value.store(rdtsc(), std::memory_order_relaxed);

// 方案 B：volatile（不推荐，但能对应内核的 READ_ONCE 语义）
struct { volatile uint32_t seq; volatile uint64_t value; } ts;
```

**不要用普通 `uint64_t`** —— 编译器有权假设"没有并发写"而把
`v = ts.value;` 提到循环外，或者拆成两次 32 位读。

#### 陷阱 ②：写端第一个 store 用 `release` **方向反了**

`memory_order_release` 的语义是"**之前**的读写不能被重排到这个 store **之后**"。
但 seqlock 写 begin 需要的是"**之后**的数据写不能被重排到 seq 自增**之前**"
——这是相反的方向。

内核的正确做法（`do_raw_write_seqcount_begin`，`:466`）：

```c
s->sequence++;          /* 普通自增 */
smp_wmb();              /* ⭐ 全屏障，挡住【后面】的数据写 */
```

C++ 等价写法：

```cpp
// 写端（正确版）
uint32_t s = ts.seq.load(std::memory_order_relaxed);
ts.seq.store(s + 1, std::memory_order_relaxed);
std::atomic_thread_fence(std::memory_order_release);   // ⭐ fence，不是 store
ts.value.store(rdtsc(), std::memory_order_relaxed);
std::atomic_thread_fence(std::memory_order_release);   // ⭐
ts.seq.store(s + 2, std::memory_order_release);
```

**为什么原写法在 x86 上"碰巧能跑"**：x86 是 TSO 内存模型，
store-store 不重排，所以"自增 → 数据写"的顺序天然保持。
**但 ARM64 会重排** → 数据先写、序号后变奇 → 读者可能读到撕裂的数据
却看到偶数序号 → **静默返回错误值**（比崩溃更难查）。

> 📌 这正是"**在 x86 上开发、在 ARM 上部署**"这类项目最典型的翻车点。
> 这类 bug 不会在开发机上复现。

#### 另外两点

1. **读端循环里的 `s1 & 1` 判断是多余的**（默认 API 下）：
   内核的 `read_seqcount_begin()` 会先自旋等到偶数才返回。
   如果想保留"不等待、直接让结尾重试"的优化，可以用 `& ~1` 技巧
   （对应内核的 `raw_seqcount_begin()`，`:395`），省一条分支。
2. **重试循环要有上界**。内核里 seqlock 读段极短、且有"写侧不可抢占"兜底，
   所以无界重试是安全的；用户态没有这个保证，
   建议加一个重试计数上限，超过就退回加锁路径（等价于内核的
   `read_seqbegin_or_lock()` 思路）。

</details>
</details>

**Q4.** （v6.6 新增）`read_seqlock_excl()` 和 `read_seqbegin()` 有什么区别？加锁读者能不能阻止普通读者？

<details><summary>答案</summary>

**`read_seqlock_excl()` 的实现只有一行**（`seqlock.h:1012`）：

```c
static inline void read_seqlock_excl(seqlock_t *sl)
{
	spin_lock(&sl->lock);
}
```

三个关键事实：

**① 它完全不碰 seqcount。** 源码注释：
"it does not update the embedded sequence number"。
所以加锁读者**不会**让 `sequence` 变奇。

**② 因此它阻止不了普通读者。** 普通读者只看 `sequence`，
而加锁读者持锁期间 `sequence` 没变 → 普通读者照样无锁读进去。
> ⚠️ **`read_seqlock_excl()` 的语义是"挡住写者"，不是"独占数据"。**

**③ 多个加锁读者之间是排他的。** 文档原话：
"Unlike `rwlock_t`, only one locking reader can acquire it."
——名字里带 read，但行为和 `spin_lock()` 一模一样。

**完整互斥关系表**：

| | 普通读者 | 加锁读者 | 写者 |
|--|---------|---------|------|
| **普通读者** | ✅ 可并发 | ✅ 可并发（⭐ 不互斥） | ❌ 会被拖进重试循环 |
| **加锁读者** | ✅ 可并发（⭐ 不互斥） | ❌ 互斥 | ❌ 互斥 |
| **写者** | ✅ **写者从不等待普通读者** | ❌ 互斥 | ❌ 互斥 |

**什么时候用它**：读段有副作用、不能重复执行，或者需要在
读的过程中保证"写者不会插进来"（例如先读 A 再读 B，中间不能被写者打断）。

**什么时候用 `read_seqbegin_or_lock()` 更好**：
写活动有突发尖峰时，纯无锁读者可能一直重试（饿死）。
混合模式先乐观无锁试一次，失败就退化成加锁读——
**加锁后写者被挡住，`sequence` 稳定，这一次读必然成功**。
代价是那一次退化成了 spinlock 临界区。

</details>

**Q5.** （v6.6 新增）在 PREEMPT_RT 内核上，为什么要让 `seqlock_t` 用 `seqcount_spinlock_t` 而不是裸 `seqcount_t`？

<details><summary>答案</summary>

因为 **RT 上 `spinlock_t` 变成了可睡眠的 rt_mutex，不再隐式关抢占**，
于是 seqlock 的"写侧不可被抢占"约束（§9 约束②）直接被破坏：
如果写者被抢占，无锁读者会看到奇数序号并在
`while (seq & 1) cpu_relax();` 里自旋；**如果读者是 RT 调度类，
优先级高于写者 → 写者永远跑不完 → 内核活锁**。

**v6.6 的解法**是一套"**读者帮写者跑完**"的技巧（`seqlock.h:145` 注释块）：

```c
	unsigned seq = READ_ONCE(s->seqcount.sequence);

	if (!IS_ENABLED(CONFIG_PREEMPT_RT))
		return seq;

	if (preemptible && unlikely(seq & 1)) {
		__SEQ_LOCK(lock_acquire);                  /* 拿一下关联锁 */
		__SEQ_LOCK(lockbase##_unlock(s->lock));    /* 立刻放掉 */

		/* 写者（可能正被抢占）因此得以推进 */
		seq = READ_ONCE(s->seqcount.sequence);
	}
	return seq;
```

**为什么这套技巧要求"关联锁"存在**：读者需要知道"该拿哪把锁才能让写者推进"
——这个信息只存在于 `seqcount_LOCKNAME_t` 的 `->lock` 指针里，
裸 `seqcount_t` **没有这个字段**，所以无解。这就是
`seqlock_t` 必须用 `seqcount_spinlock_t` 的根本原因（源码注释：
"Make sure that readers don't starve writers on PREEMPT_RT"）。

**两个附带效果**：

1. **`__SEQ_LOCK()` 在有 lockdep 或 RT 时才展开**：
   ```c
   #if defined(CONFIG_LOCKDEP) || defined(CONFIG_PREEMPT_RT)
   #define __SEQ_LOCK(expr)	expr
   #else
   #define __SEQ_LOCK(expr)
   #endif
   ```
   所以非 RT、非 lockdep 的构建里，`->lock` 指针**不存在于结构体**——零开销。

2. **RT 上写侧反而不 `preempt_disable()`**：
   `__seqprop_##lockname##_preemptible()` 在 RT 上直接 `return false`
   （注释："PREEMPT_RT relies on the above LOCK+UNLOCK"）——
   既然已经用 lock+unlock 解决活锁，就不需要牺牲抢占性了。

> 📌 `Documentation/locking/locktypes.rst` 要求这套技巧
> "must be implemented for **all** of PREEMPT_RT sleeping locks"——
> 它是 RT 的通用模式，不止 seqlock 用。

</details>

**Q6.** （v6.6 新增）`Documentation/locking/seqlock.rst` 里列的 `seqcount_ww_mutex_t`，在 v6.6 里能用吗？

<details><summary>答案</summary>

**不能——这个类型在 v6.6 里不存在。**（§7）

`Documentation/locking/seqlock.rst:110-116` 至今列着 5 种，
其中最后一种是 `seqcount_ww_mutex_t`。但实测：

| 检查 | 结果 |
|------|------|
| `grep -i ww_mutex include/linux/seqlock.h`（v6.6） | **只有 1 处**：`:294` 的 `SEQCNT_WW_MUTEX_ZERO` 宏 |
| `seqcount_ww_mutex_t` 这个**类型名** | **grep 不到**（头文件里从未出现） |
| `SEQCOUNT_LOCKNAME(ww_mutex, ...)` 实例化 | **没有**（v6.6 只有 raw_spinlock / spinlock / rwlock / mutex 四行） |
| `include/linux/ww_mutex.h`（13KB，v6.6） | 提到 seqcount 的地方：**0** |

**版本断崖**（抓多版本同名文件 `grep -c ww_mutex`）：

| 版本 | `ww_mutex` 计数 | 说明 |
|------|----------------|------|
| v5.9 | —（`SEQCOUNT_LOCKNAME` 计数为 **0**） | ⭐ 关联锁机制**尚未引入** |
| v5.10 | 4~5 | ⭐ **v5.10 引入**，含 `seqcount_ww_mutex_t` |
| v5.15 / v5.16 / v5.17 / v5.18 | 5 | 稳定期 |
| **v5.19** | **0** | ⭐ **在此被删**（文件同时从 39100B 缩到 38820B） |
| v6.0 ~ v6.6 | 0 | 一直没回来 |

**残留的孤儿宏**（`seqlock.h:294`）：

```c
#define SEQCNT_WW_MUTEX_ZERO(name, lock) 	SEQCOUNT_LOCKNAME_ZERO(name, lock)
```

它只是 `SEQCOUNT_LOCKNAME_ZERO` 的别名，而后者对**任何**
`seqcount_LOCKNAME_t` 都成立——所以这个宏本身不会报错，
只是它暗示的那个类型已经不存在了。**照着文档写会得到一个
"未定义类型"的编译错误。**

> 📌 **教训（Ch10 第 15 条"凭记忆必错"）**：
> **官方文档也会滞后。判断一个类型/宏是否存在，以头文件为准，不以 .rst 为准。**
> 同类问题：10.4 里 `DEFINE_SEMAPHORE` 从 v6.4 起要两个参数；
> 10.2 里书上讲 ticket lock 而 v6.6 早就是 qspinlock。
> **写内核代码前，grep 头文件是唯一可靠的验证手段。**

</details>

</details>

---

→ [10.7 大内核锁](./section-10.7-大内核锁.md) · [10.9 禁止抢占](./section-10.9-禁止抢占.md) · [10.10 屏障](./section-10.10-排序和屏障.md) · [10.3 rwlock](./section-10.3-读-写自旋锁.md)

---

> ↔ [ULK Ch5 §5 顺序锁与RCU](../../../16-linux-kernel-deep/chapter-05-kernel-synchronization/notes/section-5-顺序锁与RCU.md)
