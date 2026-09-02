## ⑩ 排序和屏障 · Ordering and Barriers

锁解决「互斥」；屏障解决「**可见顺序**」。编译器与 CPU 为性能会 **重排** load/store — 对 **设备寄存器**、**无锁算法**、**发布-订阅** 会出鬼。

#### 三类乱序来源

| 来源 | 例子 |
|------|------|
| **编译器** | 把两次写调换顺序 |
| **CPU 存储缓冲 / 乱序执行** | 其它核暂时看见「新 B、旧 A」 |
| **设备 / DMA** | MMIO 写合并、顺序敏感 |

#### 常用屏障

| 屏障 | 作用 |
|------|------|
| **`rmb()`** | **读屏障** — 屏障前的读不与屏障后的读乱序 |
| **`wmb()`** | **写屏障** — 写顺序 |
| **`mb()`** | **全屏障** — 读写皆不越过 |
| **`barrier()`** | **仅编译器屏障** — 不约束 CPU |
| **`smp_rmb/wmb/mb()`** | SMP 变体 — UP 上可退化为空，跨核可见性用这组 |

```
发布者 CPU0:  写数据 payload  ── wmb() ── 写 flag=1
订阅者 CPU1:  读 flag==1     ── rmb() ── 读 payload
```

缺 `wmb`/`rmb` → 订阅者可能看到 flag=1 但 payload 仍是旧值。

#### 与原子/锁的关系

| 机制 | 序 |
|------|-----|
| 普通 `spin_lock` / `mutex` | **通常自带** 足够的 acquire/release 语义 |
| 裸 `atomic_set` + 无锁结构 | **你必须自己想清楚** 屏障 |
| `READ_ONCE` / `WRITE_ONCE` | 防编译器「拆载/合并」；不替代全序屏障 |

#### MMIO

访问设备寄存器常用 **`readl`/`writel`** 等 — 内部已含架构所需的顺序约束；对裸指针乱写 MMIO 极易踩坑。

**HFT：** 用户态 `memory_order_acquire/release`、环形缓冲区的 head/tail 发布，与内核屏障 **同一类问题**。无锁队列 bug = 偶现脏数据、极难复现。

→ [02-CSAPP 并发与内存](../../../02-computer-systems/chapter-12-concurrent-programming/) · [10.1 原子](./section-10.1-原子操作.md) · [10.8 seqlock](./section-10.8-顺序锁.md)

### 常见陷阱

1. 混淆 smp_mb() / smp_rmb() / smp_wmb()——全屏障/读屏障/写屏障，保证不同方向的重排
2. 以为 x86 不需要 memory barrier——x86 有 TSO 内存模型，大部分 barrier 是空操作，但 store-load 重排仍需要 mfence
3. 在 UP 上用 smp_mb()——UP 上 smp_mb() 是空操作（无 SMP 重排），应改用 barrier() 或不需要

---

> **本篇分工**：上面速查表**原样保留**。本篇往下**不复述**"屏障要配对"这种常识，
> 只做十二件事，**全部用 v6.6 源码 + `Documentation/memory-barriers.txt`（113KB）实证**：
>
> ① ⭐ **六类屏障的精确定义**（文档 `:375-505`），包括绝大多数资料漏掉的
> "地址依赖屏障"和两个隐式种类（ACQUIRE / RELEASE）；
> ② ⭐ **v6.6 x86 上每个屏障实际生成什么指令**——并订正一个流传很广的错误：
> **`smp_mb()` 在 x86 上是 `lock addl`，不是 `mfence`**；
> ③ ⭐ `smp_` 前缀的**真正含义**（`CONFIG_SMP=n` 时全部退化成 `barrier()`），
> 以及它和 `mb()`/`rmb()`/`wmb()`（**给设备用的、UP 上也生效**）的分工；
> ④ ⭐ `READ_ONCE()` / `WRITE_ONCE()` 的**两个被普遍误解的限制**——
> 它只约束**相邻的** ONCE 访问、且**必须在不同的 C 语句里**；
> ⑤ ⭐ **反保证**（anti-guarantees）：**bitfield 绝对不能用于并行同步**，
> 以及"正确对齐 + 基本标量"这个前提；
> ⑥ ⭐ **屏障必须配对**的完整矩阵（文档 `:932`）；
> ⑦ ⭐ **控制依赖 vs 地址依赖**——load-load 控制依赖**不成立**、
> load-store 控制依赖**成立**（因为 store 不被推测），以及编译器
> **把 `if` 整个消掉**和**把两个分支相同的 store 上提**这两个反例；
> ⑧ ⭐ **版本断崖 ×2**：显式地址依赖屏障 API 在 **v5.9 移除**；
> 驱动手写的 `mmiowb()` 在 v6.6 已被**自动化**（附那段"5 步 + Complain to your architects"的注释）；
> ⑨ release/acquire 的精确语义 + ⚠️ **RELEASE+ACQUIRE 对 ≠ 全屏障**；
> ⑩ `smp_mb__before_atomic()` / `__after_atomic()` —— **x86 上是空的**；
> ⑪ `smp_cond_load_acquire()` 怎么用控制依赖省掉一条屏障；
> ⑫ MMIO 侧的 `__io_br/ar/bw/aw` 钩子家族，以及为什么"裸指针写 MMIO"必踩坑。
>
> 所有定义与代码均核对自缓存的 v6.6 源码，行号可查。

---

## 1. ⭐ 六类屏障：文档原文的精确定义

`Documentation/memory-barriers.txt:375` 把屏障分成**四个基本种类 + 两个隐式种类**。
中文资料几乎只讲前三个，漏掉的第 (2) 类和第 (5)(6) 类恰恰是最常用的。

### 四个基本种类

| # | 种类 | 约束对象 | API |
|---|------|---------|-----|
| (1) | **写屏障** | 只约束 **store↔store** | `wmb()` / `smp_wmb()` |
| (2) | **地址依赖屏障**（历史） | 只约束**相互依赖的** load | ⚠️ **v5.9 起无显式 API**，由 `READ_ONCE()` 隐式提供 |
| (3) | **读屏障** | 只约束 **load↔load**（且隐含 (2)） | `rmb()` / `smp_rmb()` |
| (4) | **全屏障** | 约束**全部 load 与 store** | `mb()` / `smp_mb()` |

文档对每一类的措辞都很克制，值得逐字看：

- (1)："A write barrier is a **partial ordering on stores only**; it is **not
  required to have any effect on loads**."
- (3)："A read barrier is a **partial ordering on loads only**; it is **not
  required to have any effect on stores**."

⭐ **含义**：`smp_wmb()` **不保证**任何 load 的顺序，
`smp_rmb()` **不保证**任何 store 的顺序。
需要同时约束两者 → 必须用 `smp_mb()` 或 release/acquire 对。

### 两个隐式种类（这才是日常最该用的）

| # | 种类 | 语义 | API |
|---|------|------|-----|
| (5) | **ACQUIRE** | **单向渗透**屏障：保证**之后**的所有访存都发生在它**之后** | LOCK 操作、`smp_load_acquire()`、`smp_cond_load_acquire()` |
| (6) | **RELEASE** | **单向渗透**：保证**之前**的所有访存都发生在它**之前** | UNLOCK 操作、`smp_store_release()` |

⚠️ 注意单向性（文档原文）：

> "Memory operations that occur **before** an ACQUIRE operation **may appear
> to happen after** it completes."
> "Memory operations that occur **after** a RELEASE operation **may appear to
> happen before** it completes."

**这就是它们比 `smp_mb()` 便宜的原因**——只挡一个方向。

⭐ 最重要的一句（文档 `:504`）：

> "In addition, a **RELEASE+ACQUIRE pair is -not- guaranteed to act as a full
> memory barrier.** However, after an ACQUIRE on a given variable, all memory
> accesses preceding any prior RELEASE on that **same variable** are guaranteed
> to be visible."

| 你想要的效果 | 用什么 |
|------------|--------|
| 发布-订阅（消息传递） | RELEASE + ACQUIRE **配对** ✅ |
| 需要**全局**顺序（如 Dekker/Peterson 算法） | ❌ RC 对不够，必须 `smp_mb()` |

### 版本断崖：显式地址依赖屏障 API 在 v5.9 被移除

文档 `:433`：

> "[!] **Kernel release v5.9 removed kernel APIs for explicit
> address-dependency barriers.** Nowadays, APIs for marking loads from shared
> variables such as `READ_ONCE()` and `rcu_dereference()` provide **implicit**
> address-dependency barriers."

（历史背景：只有 DEC Alpha 真的需要这个屏障，因为它的 cache 会让
"依赖加载"看到旧数据。v4.15 给 Alpha 的 `READ_ONCE()` 加了 `smp_mb()`，
于是显式 API 就没必要了。）

---

## 2. ⭐ v6.6 x86 上每个屏障实际生成什么指令

看 `arch/x86/include/asm/barrier.h`（v6.6）：

```c
/* 64 位 */
#define __mb()	asm volatile("mfence":::"memory")       /* :22 */
#define __rmb()	asm volatile("lfence":::"memory")       /* :23 */
#define __wmb()	asm volatile("sfence" ::: "memory")     /* :24 */

#define __smp_mb()	asm volatile("lock; addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")  /* :57 */
#define __smp_rmb()	dma_rmb()                            /* :59 */
#define __smp_wmb()	barrier()                            /* :60 */

#define __dma_rmb()	barrier()                            /* :54 */
#define __dma_wmb()	barrier()                            /* :55 */

/* Atomic operations are already serializing on x86 */
#define __smp_mb__before_atomic()	do { } while (0)     /* :79 */
#define __smp_mb__after_atomic()	do { } while (0)     /* :80 */
```

### 完整对照表

| 屏障 | v6.6 x86-64 实际生成 | 大致周期 |
|------|---------------------|---------|
| `barrier()` | **无指令**（只约束编译器） | 0 |
| `smp_wmb()` | **无指令**（= `barrier()`） | 0 |
| `smp_rmb()` | **无指令**（= `dma_rmb()` = `barrier()`） | 0 |
| `smp_store_release()` | **无指令**（`barrier(); WRITE_ONCE()`） | 0 |
| `smp_load_acquire()` | **无指令**（`READ_ONCE(); barrier()`） | 0 |
| ⭐ **`smp_mb()`** | ⭐ **`lock; addl $0,-4(%rsp)`** | ~20-30 |
| `wmb()` | `sfence` | ~20 |
| `rmb()` | `lfence` | ~20 |
| `mb()` | `mfence` | ~30-60 |
| `smp_mb__before_atomic()` | **空** | 0 |
| `smp_mb__after_atomic()` | **空** | 0 |

### ⚠️ 订正：x86 的 `smp_mb()` 是 `lock addl`，**不是 `mfence`**

> 这是原速查表自测题 Q1/Q3 里的错误（第 16 条"凭记忆必错"）。

```c
#define __smp_mb()	asm volatile("lock; addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")
```

**为什么用 `lock addl` 而不是 `mfence`**：
两者都能做全屏障，但 `lock` 前缀的指令在多数微架构上**比 `mfence` 快**
（`mfence` 在某些 CPU 上还要等 store buffer 完全排空并影响后续 load 的推测）。
`lock; addl $0, -4(%rsp)` 是个"什么都不改"的原子 RMW，
副作用就是它带的 `lock` 语义 = 全屏障。

> 📌 **对照**：32 位 x86 上 `mb()` 用 ALTERNATIVE 在
> `lock; addl $0,-4(%esp)` 和 `mfence` 之间按 `X86_FEATURE_XMM2` 二选一
> （：15-20）—— 老 CPU 没有 `mfence`，只能用 `lock addl`。

### ARM64 对照（为什么嵌入式要格外小心）

| 屏障 | x86-64 | ARM64 |
|------|--------|-------|
| `smp_wmb()` | **0 指令** | `dmb ishst` |
| `smp_rmb()` | **0 指令** | `dmb ishld` |
| `smp_mb()` | `lock addl`（~20-30） | `dmb ish`（~30-50） |
| `smp_store_release()` | **0 指令** | `stlr` |
| `smp_load_acquire()` | **0 指令** | `ldar` |

⭐ **在 x86 上"免费"的 `smp_wmb()` / `smp_rmb()` / release / acquire，
在 ARM64 上全是真指令。**
这就是为什么在 x86 上"看起来没问题"的无锁代码，
**一移植到 ARM 就开始偶现脏数据**——根本不是偶现，是屏障本来就缺，
只是 x86 的 TSO 帮你兜住了。

---

## 3. ⭐ `smp_` 前缀的真正含义，以及和 `mb()` 的分工

`include/asm-generic/barrier.h`（v6.6）：

```c
#ifdef CONFIG_SMP
#define smp_mb()	do { kcsan_mb(); __smp_mb(); } while (0)     /* :99 */
#define smp_rmb()	do { kcsan_rmb(); __smp_rmb(); } while (0)   /* :103 */
#define smp_wmb()	do { kcsan_wmb(); __smp_wmb(); } while (0)   /* :107 */
#else	/* !CONFIG_SMP */
#define smp_mb()	barrier()                                     /* :113 */
#define smp_rmb()	barrier()                                     /* :117 */
#define smp_wmb()	barrier()                                     /* :121 */
#endif
```

| 族 | 约束对象 | UP 上 |
|----|---------|------|
| **`smp_mb/rmb/wmb()`** | **CPU ↔ CPU** | 退化成 `barrier()`（只有一个 CPU，不存在跨核乱序） |
| **`mb/rmb/wmb()`** | ⭐ **CPU ↔ 设备** | **照常生效**（注释："this is required on UP too when we're talking to devices"） |

源码注释（`:52`）：

```c
/*
 * Force strict CPU ordering. And yes, this is required on UP too when we're
 * talking to devices.
 *
 * Fall back to compiler barriers if nothing better is provided.
 */
```

⭐ **选型判据一句话**：

> **跨核用 `smp_*`，跨设备用 `mb/rmb/wmb()`（或 `dma_*`）。**

⚠️ 常见错误反过来用：
- 驱动里对 MMIO 用 `smp_wmb()` → **UP 内核上退化成 `barrier()`，设备照样看到乱序**；
- 无锁队列里用 `mb()` → 在 UP 上也付全屏障代价（虽然正确，但白花钱）。

### 还有一族：`dma_*` 和 `virt_*`

| 族 | 用途 |
|----|------|
| `dma_rmb()` / `dma_wmb()` / `dma_mb()` | **CPU ↔ 一致性 DMA 设备**（`__dma_rmb()` 在 x86 上就是 `barrier()`） |
| `virt_rmb()` / `virt_wmb()` / `virt_mb()` | **虚拟化**：guest ↔ 其它 guest / host 之间的顺序（定义上与 `smp_*` 同构，：161-169） |

### 每个屏障都挂了 KCSAN 钩子

```c
#define smp_mb()	do { kcsan_mb(); __smp_mb(); } while (0)
```

⭐ **开 `CONFIG_KCSAN` 时，每个屏障都会先调一次 `kcsan_mb()` 之类的钩子**
（`kcsan_mb` / `kcsan_rmb` / `kcsan_wmb` / `kcsan_release`）。
KCSAN 靠这些钩子建立 happens-before 关系，
**所以"我用了屏障但 KCSAN 还是报了竞争"往往说明屏障类型选错了**（比如该用 `smp_mb()` 却用了 `smp_wmb()`）。

---

## 4. ⭐ `READ_ONCE()` / `WRITE_ONCE()` 的两个被普遍误解的限制

### 它们到底是什么

`include/asm-generic/rwonce.h`（v6.6）：

```c
#define __READ_ONCE(x)	(*(const volatile __unqual_scalar_typeof(x) *)&(x))    /* :44 */

#define READ_ONCE(x)							\
({									\
	compiletime_assert_rwonce_type(x);				\
	__READ_ONCE(x);							\
})

#define __WRITE_ONCE(x, val)						\
do {									\
	*(volatile typeof(x) *)&(x) = (val);				\
} while (0)
```

**本质就是 `volatile` 强制转换**——生成**零额外指令**，只影响编译器行为。

### 限制 ①：它们**只约束相邻的 ONCE 访问**，且必须在**不同的 C 语句**里

头文件开头的注释（`:5-8`）：

> "The compiler is also forbidden from reordering successive instances of
> READ_ONCE and WRITE_ONCE, **but only when the compiler is aware of some
> particular ordering. One way to make the compiler aware of ordering is to
> put the two invocations of READ_ONCE or WRITE_ONCE in different C
> statements.**"

⚠️ 推论：

```c
/* ✅ 正确：两条独立语句 */
WRITE_ONCE(a, 1);
WRITE_ONCE(b, 2);

/* ❌ 危险：同一条语句里，编译器不保证顺序 */
f(READ_ONCE(x), READ_ONCE(y));      /* 求值顺序未定义 */
WRITE_ONCE(*p++, v);                /* 副作用混在一起 */
```

### 限制 ②：类型必须是"原生字长"或 `long long`

```c
#define compiletime_assert_rwonce_type(t)					\
	compiletime_assert(__native_word(t) || sizeof(t) == sizeof(long long),	\
		"Unsupported access size for {READ,WRITE}_ONCE().")
```

| 类型 | 能否用 | 说明 |
|------|--------|------|
| `int` / `long` / 指针 | ✅ | `__native_word` |
| `u64` / `long long` | ✅ | 特例放行（注释调侃："we rely on ... a strong prevailing wind"） |
| **`struct foo`（聚合类型）** | ⚠️ 头文件注释说"will also work on aggregate data types"，但**实际会触发 compiletime_assert** | 大结构体要另想办法（memcpy + 屏障，或先 copy 到本地） |
| **bitfield** | ❌ | 见 §5 |

### ⚠️ 最重要的一点：`READ_ONCE()` 不提供任何 CPU 顺序保证

文档 `:633` 的措辞很强硬：

> "**please carefully read the 'CONTROL DEPENDENCIES' section** ...
> The compiler can and does break dependencies."

以及文档 `:267`：

> "It _must_not_ be assumed that the compiler will do what you want with
> memory references that are **not protected by READ_ONCE() and WRITE_ONCE()**."

| 你需要什么 | 用什么 |
|-----------|--------|
| 只防编译器合并/拆载/重取 | `READ_ONCE()` / `WRITE_ONCE()` |
| 跨核的顺序 | ⭐ **另外加 `smp_*mb()` 或 release/acquire** |
| 依赖加载（指针追逐） | `READ_ONCE()`（**隐式**提供地址依赖屏障，见 §1） |

> 📌 两个典型用例（头文件注释 `:13-18`）：
> **(1)** 同一 CPU 上进程代码与 irq/NMI 处理函数之间的通信；
> **(2)** 防止编译器折叠/破坏那些"不需要顺序、或由显式屏障/原子指令提供顺序"的访问。
>
> 注意用例 (1) 是**单 CPU 场景**——这正是 `READ_ONCE()` 的核心定位。

---

## 5. ⭐ 反保证（anti-guarantees）：bitfield 绝对不能用于并行同步

`Documentation/memory-barriers.txt:310` 整段都是"不能假设什么"，其中两条
**直接推翻很多驱动的写法**：

### 反保证 ①：位域（bitfield）

> "(*) These guarantees **do not apply to bitfields**, because compilers often
> generate code to modify these using **non-atomic read-modify-write
> sequences**. **Do not attempt to use bitfields to synchronize parallel
> algorithms.**
>
> (*) Even in cases where bitfields are protected by locks, **all fields in a
> given bitfield must be protected by one lock.** If two fields in a given
> bitfield are protected by different locks, the compiler's non-atomic
> read-modify-write sequences **can cause an update to one field to corrupt
> the value of an adjacent field**."

⭐ **两条硬规则**：

| 规则 | 说明 |
|------|------|
| ❌ 不要用 bitfield 做并行同步 | 改一个 bit 是"读整个机器字 → 改 → 写回"，两个 CPU 并发就会丢更新 |
| ⚠️ 即使用锁，同一个 bitfield 的所有字段必须**共用一把锁** | 否则改 A 字段的 RMW 会把并发改 B 字段的结果**覆盖掉** |

> **这条在驱动里极其常见**：`struct dev_state { unsigned long flags; }` 用
> `unsigned long flags:1; unsigned long ready:1;` 然后分别加锁 —— **是错的**。

**正确做法**：用独立的基本类型字段 + `set_bit()`/`clear_bit()`（原子位操作，见 10.1），
或者整组字段共用一把锁。

### 反保证 ②：只保证"正确对齐 + 基本标量"

> "(*) These guarantees apply only to **properly aligned and sized scalar
> variables**. 'Properly sized' currently means variables that are the same
> size as 'char', 'short', 'int' and 'long'."

（并且文档还贴心地引用了 C11 §3.14 "memory location" 的定义，
说明这个保证是 C11 才标准化的——"beware when using older pre-C11
compilers (for example, gcc 4.6)"。）

### 另外两条"不能假设"

| 文档条目 | 含义 |
|---------|------|
| "It _must_not_ be assumed that **independent** loads and stores will be issued in the order given" | `X = *A; Y = *B; *D = Z;` 有 **6 种**合法执行顺序 |
| "It _must_ be assumed that **overlapping** memory accesses may be merged or discarded" | `X = *A; Y = *(A+4);` 可能被合并成**一次** 8 字节加载；两个 store 同理 |

⭐ 这两条解释了**为什么 `READ_ONCE()`/`WRITE_ONCE()` 是必需的**：
没有它们，编译器可以合并、拆开、重排、甚至**消掉**你的访问。

---

## 6. ⭐ 屏障必须配对：完整矩阵

`Documentation/memory-barriers.txt:932` 标题就是 "SMP BARRIER PAIRING"，
第一句：

> "When dealing with CPU-CPU interactions, certain types of memory barrier
> should **always be paired**. **A lack of appropriate pairing is almost
> certainly an error.**"

| CPU 1（写侧） | 可以和 CPU 2（读侧）的什么配对 |
|--------------|---------------------------|
| 全屏障 `smp_mb()` | 全屏障（也可以和大多数其它类型配对，但没有 multicopy atomicity） |
| acquire ↔ release | **acquire 配 release**（两者也都能和全屏障配对） |
| **写屏障 `smp_wmb()`** | 地址依赖屏障 / **控制依赖** / acquire / release / **读屏障** / 全屏障 |
| 读屏障 / 控制依赖 / 地址依赖屏障 | 写屏障 / acquire / release / 全屏障 |

⭐ 文档最后一句是判据：

> "**Basically, the read barrier always has to be there, even though it can be
> of the 'weaker' type.**"

### 三种配对范式（文档原文的例子）

```
范式 A：写屏障 + 读屏障
CPU 1                    CPU 2
=====================    =====================
WRITE_ONCE(a, 1);
<write barrier>          x = READ_ONCE(b);
WRITE_ONCE(b, 2);        <read barrier>
                         y = READ_ONCE(a);

范式 B：写屏障 + 隐式地址依赖屏障
CPU 1                    CPU 2
=====================    ==============================
a = 1;
<write barrier>          x = READ_ONCE(b);
WRITE_ONCE(b, &a);       <implicit address-dependency barrier>
                         y = *x;

范式 C：全屏障 + 控制依赖
CPU 1                    CPU 2
=====================    ==============================
r1 = READ_ONCE(y);
<general barrier>        if (r2 = READ_ONCE(x)) {
WRITE_ONCE(x, 1);             <implicit control dependency>
                              WRITE_ONCE(y, 1);
                         }
assert(r1 == 0 || r2 == 0);
```

⚠️ **范式 C 里 CPU 1 用的是【全屏障】不是写屏障** —— 因为它既有 load 又有 store。
用 `smp_wmb()` 在这里是不够的。

---

## 7. ⭐ 控制依赖 vs 地址依赖：一个能省屏障，一个不能

这是无锁代码里最容易写错的地方。文档 `:673` 有一整节。

### 核心结论

| 依赖类型 | 是否提供顺序 | 需要额外屏障吗 |
|---------|------------|--------------|
| **地址依赖**（第二个 load 的**地址**来自第一个 load） | ✅ 提供（由 `READ_ONCE()` 隐式保证） | 不需要 |
| **控制依赖 load→load**（用第一个 load 的值做 `if` 判断，再 load 第二个） | ❌ **不提供** | ⭐ **需要真的 `smp_rmb()`** |
| **控制依赖 load→store**（`if (q) WRITE_ONCE(b, 1);`） | ✅ 提供 | 不需要（**store 不被推测**） |

### 反例 ①：load-load 控制依赖**不成立**

```c
q = READ_ONCE(a);
<implicit address-dependency barrier>
if (q) {
	/* BUG: No address dependency!!! */
	p = READ_ONCE(b);
}
```

文档解释：

> "This will not have the desired effect because there is **no actual address
> dependency, but rather a control dependency that the CPU may short-circuit by
> attempting to predict the outcome in advance**"

CPU 会**预测** `q` 为真、提前把 `b` 加载了。正确写法：

```c
q = READ_ONCE(a);
if (q) {
	<read barrier>            /* ⭐ 必须显式加 */
	p = READ_ONCE(b);
}
```

### 反例 ②：load-store 控制依赖**成立**

```c
q = READ_ONCE(a);
if (q) {
	WRITE_ONCE(b, 1);         /* ✅ 不需要额外屏障 */
}
```

文档："**stores are not speculated**. This means that ordering *is* provided
for load-store control dependencies."

### 反例 ③：编译器会把 `if` 整个消掉

> "Worse yet, if the compiler is able to prove (say) that the value of
> variable 'a' is always non-zero, it would be well within its rights to
> optimize the original example by **eliminating the 'if' statement**"

```c
/* 你写的 */
q = READ_ONCE(a);
if (q) { WRITE_ONCE(b, 1); }

/* 编译器可能变成 */
q = a;
b = 1;      /* BUG: Compiler and CPU can both reorder!!! */
```

→ **"So don't leave out the READ_ONCE()."**

### 反例 ④：两个分支相同的 store 会被"上提"

```c
/* 看起来很安全：两个分支都加了 barrier() */
q = READ_ONCE(a);
if (q) {
	barrier(); WRITE_ONCE(b, 1); do_something();
} else {
	barrier(); WRITE_ONCE(b, 1); do_something_else();
}

/* 高优化级别下编译器会变成：*/
q = READ_ONCE(a);
barrier();
WRITE_ONCE(b, 1);        /* BUG: 条件没了，顺序保证消失 */
if (q) { do_something(); } else { do_something_else(); }
```

文档结论：

> "The conditional is **absolutely required, and must be present in the
> assembly code even after all compiler optimizations have been applied.**
> Therefore, if you need ordering in this example, you need **explicit** [barriers]."

⭐ **一句话总结**：**控制依赖只能用来省 load→store 的屏障；
想省 load→load 的屏障，必须让第二个 load 的**地址**真的依赖第一个 load。**

### v6.6 的现成工具

```c
#define smp_acquire__after_ctrl_dep()		smp_rmb()      /* barrier.h:234 */

#define smp_cond_load_acquire(ptr, cond_expr) ({		\
	__unqual_scalar_typeof(*ptr) _val;			\
	_val = smp_cond_load_relaxed(ptr, cond_expr);		\
	smp_acquire__after_ctrl_dep();				\
	(typeof(*ptr))_val;					\
})
```

`smp_cond_load_acquire()` 的注释说明了它的价值：

> "Equivalent to using `smp_load_acquire()` on the condition variable but
> **employs the control dependency of the wait to reduce the barrier on many
> platforms**."

——**自旋等待本身就是个控制依赖**，所以循环结束后补一个
`smp_acquire__after_ctrl_dep()`（= `smp_rmb()`）就够了，
不需要 `smp_mb()`。在 x86 上它还是零指令。

---

## 8. ⭐ 版本断崖 ×2：地址依赖屏障 API 与 `mmiowb()`

### 断崖 ①：显式地址依赖屏障 API 在 v5.9 移除

见 §1。现在的写法：**只写 `READ_ONCE()`**，地址依赖屏障由它隐式提供。
（老代码里如果看到 `smp_read_barrier_depends()`，那是 v5.9 之前的写法。）

### 断崖 ②：驱动手写的 `mmiowb()` 在 v6.6 已被**自动化**

`include/asm-generic/io.h:44`（v6.6）：

```c
/* serialize device access against a spin_unlock, usually handled there. */
#ifndef __io_aw
#define __io_aw()      mmiowb_set_pending()
#endif
```

配合 `include/asm-generic/mmiowb.h`（v6.6）里的三段逻辑：

```c
static inline void mmiowb_set_pending(void)      /* I/O 写访问器里自动调 */
{
	struct mmiowb_state *ms = __mmiowb_state();
	if (likely(ms->nesting_count))
		ms->mmiowb_pending = ms->nesting_count;
}

static inline void mmiowb_spin_lock(void)        /* spin_lock 里自动调 */
{
	struct mmiowb_state *ms = __mmiowb_state();
	ms->nesting_count++;
}

static inline void mmiowb_spin_unlock(void)      /* spin_unlock 里自动调 */
{
	struct mmiowb_state *ms = __mmiowb_state();
	if (unlikely(ms->mmiowb_pending)) {
		ms->mmiowb_pending = 0;
		mmiowb();                    /* ⭐ 只有真的写过 I/O 才 flush */
	}
	ms->nesting_count--;
}
```

⭐ **机制**：`mmiowb_set_pending()` 记录"我在持锁期间写过 I/O"，
`mmiowb_spin_unlock()` 在**放锁时**才真的 flush。
**驱动作者不再需要记得手工调 `mmiowb()`。**

而且 `CONFIG_MMIOWB=n` 时这三个全是**空宏**：

```c
#define mmiowb_set_pending()		do { } while (0)
#define mmiowb_spin_lock()		do { } while (0)
#define mmiowb_spin_unlock()		do { } while (0)
```

> 📌 **顺带一提**，那个头文件的注释是内核里少见的幽默（"FIVE easy steps"）：
> ```
> * 	1. Implement mmiowb() (and arch_mmiowb_state() if you're fancy)
> *	   in asm/mmiowb.h, then #include this file
> *	2. Ensure your I/O write accessors call mmiowb_set_pending()
> *	3. Select ARCH_HAS_MMIOWB
> *	4. Untangle the resulting mess of header files
> *	5. Complain to your architects
> ```
> —— 第 5 步是认真的。

⚠️ **验证提示**：`mmiowb` 这个词**不在** `Documentation/memory-barriers.txt` 里，
也不在 `asm-generic/barrier.h` 里。它在 `asm-generic/io.h` 和
`asm-generic/mmiowb.h`。**grep 时找错文件会得出"已被删除"的错误结论。**

---

## 9. release / acquire 的精确定义（与上/下游的配对）

`asm-generic/barrier.h:139`（通用版）：

```c
#define __smp_store_release(p, v)					\
do {									\
	compiletime_assert_atomic_type(*p);				\
	__smp_mb();                     /* ⭐ 通用架构：全屏障 */      \
	WRITE_ONCE(*p, v);						\
} while (0)

#define __smp_load_acquire(p)						\
({									\
	__unqual_scalar_typeof(*p) ___p1 = READ_ONCE(*p);		\
	compiletime_assert_atomic_type(*p);				\
	__smp_mb();                     /* ⭐ 通用架构：全屏障 */      \
	(typeof(*p))___p1;						\
})
```

x86 版（`arch/x86/include/asm/barrier.h:63`）——**便宜得多**：

```c
#define __smp_store_release(p, v)					\
do {									\
	compiletime_assert_atomic_type(*p);				\
	barrier();                      /* ⭐ 只编译屏障！ */          \
	WRITE_ONCE(*p, v);						\
} while (0)

#define __smp_load_acquire(p)						\
({									\
	typeof(*p) ___p1 = READ_ONCE(*p);				\
	compiletime_assert_atomic_type(*p);				\
	barrier();                      /* ⭐ 只编译屏障！ */          \
	___p1;								\
})
```

| | 通用架构 | x86 |
|--|---------|-----|
| `smp_store_release()` | `__smp_mb()` + store | **`barrier()` + store**（零指令） |
| `smp_load_acquire()` | load + `__smp_mb()` | **load + `barrier()`**（零指令） |

⭐ **这就是"用 release/acquire 代替 `smp_mb()`"的性能来源**——
在 x86 上是 **0 指令 vs `lock addl`**，在 ARM64 上是 `stlr` vs `dmb ish`。

⚠️ 但注意 `compiletime_assert_atomic_type(*p)`：
**release/acquire 的目标类型必须是"原子类型"**（`atomic_t` 或原生字长），
否则编译失败。

### ⚠️ 再强调一次 §1 那条

> "a **RELEASE+ACQUIRE pair is -not- guaranteed to act as a full memory barrier**"

| 场景 | release/acquire 够吗 |
|------|---------------------|
| 发布-订阅、ring buffer 的 head/tail | ✅ 够 |
| seqlock 的读写配对（10.8 §2） | ⚠️ **不够**——所以用 `smp_wmb()` + `smp_rmb()` 显式配对 |
| 需要"所有 CPU 看到同一个全局顺序" | ❌ 不够，用 `smp_mb()` |

---

## 10. `smp_mb__before_atomic()` / `__after_atomic()`：x86 上是空的

```c
/* arch/x86/include/asm/barrier.h:78 */
/* Atomic operations are already serializing on x86 */
#define __smp_mb__before_atomic()	do { } while (0)
#define __smp_mb__after_atomic()	do { } while (0)
```

通用架构则是全屏障（`asm-generic/barrier.h:131`）：

```c
#define __smp_mb__before_atomic()	__smp_mb()
#define __smp_mb__after_atomic()	__smp_mb()
```

| 架构 | 含义 |
|------|------|
| x86 | **原子 RMW 指令本身就带 `lock` 语义 = 全屏障**，前后不需要再补 |
| 通用 / ARM | 原子操作的"有序"版本才带屏障；**relaxed 版本前后要手动补** |

⭐ **使用场景**：当你用**非返回值**的原子操作做"发布"时：

```c
/* 典型 pattern：清 bit + 保证之前的写在别的 CPU 可见 */
smp_mb__before_atomic();
clear_bit(FREE, &obj->flags);
smp_mb__after_atomic();
```

在 x86 上这三行只有一条 `lock btr`；在 ARM 上前后各一条 `dmb ish`。

---

## 11. MMIO：为什么"裸指针写 MMIO"必踩坑

### 设备操作对顺序极度敏感

文档 `:202` 的例子（网卡的地址/数据双寄存器）：

```c
*A = 5;      /* 设置要读的内部寄存器编号 */
x = *D;      /* 读数据 */
```

**可能被 CPU 重排成**：

```
x = LOAD *D, STORE *A = 5        ← 先读了数据，才设置编号 → 必然出错
```

### v6.6 的 I/O 访问器钩子家族

`include/asm-generic/io.h`（v6.6）定义了四个钩子，架构可以覆写：

| 钩子 | 位置 | 默认 |
|------|------|------|
| `__io_br()` | **读之前** | `barrier()` |
| `__io_ar()` | **读之后** | `__io_br()`（默认继承） |
| `__io_bw()` | **写之前** | `barrier()` |
| `__io_aw()` | **写之后** | ⭐ `mmiowb_set_pending()` |
| `__io_pbw()` | **写之前**（"p" = prior） | `__io_bw()` |

⭐ **关键差异**：读侧默认前后都是 `barrier()`，
而**写侧后面挂的是 `mmiowb_set_pending()`**——这就是 §8 讲的自动 mmiowb 机制。

### 为什么必须用 `readl()` / `writel()`

| 裸指针写法的问题 | `readl/writel` 怎么解决 |
|-----------------|------------------------|
| 编译器可能合并/消除访问 | 内部有 `volatile` 语义 |
| 没有顺序钩子 | ⭐ 自动插入 `__io_br/ar/bw/aw` |
| 没有字节序转换 | 按 little-endian 语义转换 |
| **没有 `mmiowb` 追踪** | ⭐ `__io_aw()` 自动 `mmiowb_set_pending()` |
| 可能被推测执行（Spectre 类） | 架构可以插 `barrier_nospec()` |

⚠️ **`barrier_nospec()`**（`arch/x86/include/asm/barrier.h:52`）：

```c
#define barrier_nospec() alternative("", "lfence", X86_FEATURE_LFENCE_RDTSC)
```

——防止**推测执行**越过屏障，这是 Spectre 之后新增的屏障种类。

### 一个 x86 上的冷知识：`weak_wrmsr_fence()`

```c
/*
 * MFENCE makes writes visible, but only affects load/store
 * instructions.  WRMSR is unfortunately not a load/store
 * instruction and is unaffected by MFENCE.  The LFENCE ensures
 * that the WRMSR is not reordered.
 */
static inline void weak_wrmsr_fence(void)
{
	asm volatile("mfence; lfence" : : : "memory");
}
```

⭐ **`mfence` 管不到 `WRMSR`**（它不是 load/store 指令），
所以要用 `mfence; lfence` 组合。注释还说：大部分 WRMSR 本身就是
fully serializing，只有 **IA32_TSC_DEADLINE 和 X2APIC MSR** 需要这个。

---

## HFT / 嵌入式关联

### 屏障成本速查（决定了无锁算法划不划算）

| 操作 | x86-64 | ARM64 |
|------|--------|-------|
| 普通 store / load | ~0 | ~0 |
| `smp_store_release()` | **0 额外指令** | `stlr`（几周期） |
| `smp_load_acquire()` | **0 额外指令** | `ldar`（几周期） |
| `smp_wmb()` | **0 指令** | `dmb ishst` |
| `smp_rmb()` | **0 指令** | `dmb ishld` |
| `smp_mb()` | `lock addl`（~20-30） | `dmb ish`（~30-50） |
| `mb()` | `mfence`（~30-60） | `dmb`（~30-50） |

⭐ **对 HFT 的两条结论**：

1. **在 x86 上，release/acquire 是免费的** —— 能用它就绝不用 `smp_mb()`。
   一个 ring buffer 的生产者路径，`smp_store_release()` 比
   "`smp_wmb()` + 普通 store" 还要便宜（一个是 `barrier()`，一个是 `barrier()`，其实一样；
   但比 `smp_mb()` 便宜几十周期）。
2. ⚠️ **在 ARM64 上全都要付真钱** —— 一个高频 ring buffer 每次入队
   一条 `stlr` + 一条 `dmb ishst`，如果这个路径每秒跑几百万次，
   **屏障开销是可观测的**。这时应该考虑**批处理**（批量发布一次）。

> 📌 **精确的判断方法**：不要信本文给的数字（它们随微架构变化），
> 用 `perf stat -e` 数 `mem_inst_retire` 或者直接跑微基准。
> 内核自带 `tools/perf`，也有 `Documentation/` 下的 `intel-rapl` 之类。

### ⚠️ 用户态 C++ 的 `memory_order` 与内核屏障的对应

| 内核 | C++ `std::memory_order` |
|------|------------------------|
| `READ_ONCE()` + 无屏障 | `relaxed` |
| `smp_load_acquire()` | **`acquire`** |
| `smp_store_release()` | **`release`** |
| `smp_mb()` | **`seq_cst`**（且比 seq_cst 更强） |
| `barrier()` | `atomic_signal_fence`（**不是** `atomic_thread_fence`！） |

⭐ 最容易错的一条：
**`barrier()` 只管编译器，等价于 C++ 的 `atomic_signal_fence`，
不是 `atomic_thread_fence`。** 拿 `atomic_thread_fence(acquire)` 去"翻译"
`barrier()` 会得到更强的语义（也就不必要的开销）。

### 无锁 ring buffer 的完整模板（HFT 最常用）

```c
/* ===== 内核版（单生产者 / 单消费者）===== */
struct ring { u32 head ____cacheline_aligned;   /* 生产者写 */
              u32 tail ____cacheline_aligned;   /* 消费者写 */
              struct msg buf[N] ____cacheline_aligned; };

/* 生产者 */
u32 h = READ_ONCE(r->head);
u32 t = READ_ONCE(r->tail);              /* ⭐ relaxed 读：SPSC 下只有我改 head */
if (h - t == N) return -ENOSPC;
r->buf[h % N] = msg;                     /* 普通写，不用 WRITE_ONCE（无并发读者） */
smp_store_release(&r->head, h + 1);      /* ⭐ 发布：之前的 buf 写对消费者可见 */

/* 消费者 */
u32 t = READ_ONCE(r->tail);
u32 h = smp_load_acquire(&r->head);      /* ⭐ 获取：与生产者的 release 配对 */
if (h == t) return -EAGAIN;
msg = r->buf[t % N];
smp_store_release(&r->tail, t + 1);      /* 归还槽位 */
```

**逐条解释为什么这样就够**：

| 位置 | 为什么 |
|------|--------|
| `head` / `tail` 各占一个 cacheline | 避免伪共享（生产者写 head 不会 invalidate 消费者正在读的 tail 所在行） |
| 生产者读 `tail` 用 `READ_ONCE()`（relaxed） | SPSC 下**只有消费者会改 tail**；生产者读到稍旧的值只会"少放一个"，不会出错 |
| 生产者写 `head` 用 `smp_store_release()` | ⭐ **必须**：否则消费者可能先看到 head 更新、却读到还没写完的 buf |
| 消费者读 `head` 用 `smp_load_acquire()` | 与生产者的 release **配对** |
| 消费者写 `tail` 用 `smp_store_release()` | 让生产者看到槽位已归还（且保证它真的读完了 buf） |
| **不需要 `smp_mb()`** | 因为这是**发布-订阅**，不是全局顺序（见 §1 的警告） |

⚠️ **如果变成 MPSC / MPMC**：relaxed 读 `tail` 那一步就不成立了，
必须换成 `smp_load_acquire()`（或者接受"少放一个"的语义）。
**先想清楚是 SPSC 还是 MPMC，再选屏障。**

### 嵌入式：把 x86 上开发的代码搬到 ARM 的 checklist

| # | 检查 | 说明 |
|---|------|------|
| 1 | 所有 `smp_wmb()` / `smp_rmb()` 都还在吗？ | x86 上它们是零指令，删了也不报错 |
| 2 | 有没有依赖"x86 的 store-store 不重排"？ | ⚠️ ARM 会重排（10.8 自测题 Q3 那个 C++ seqlock 就是例子） |
| 3 | 有没有用 bitfield 做标志位同步？ | ❌ 见 §5，两种架构上都错，只是 x86 上更不容易复现 |
| 4 | `READ_ONCE()` / `WRITE_ONCE()` 在正确的地方吗？ | 缺了的话编译器在两个架构上都可能重排 |
| 5 | MMIO 用的是 `readl/writel` 还是裸指针？ | 裸指针在 ARM 上必出问题 |
| 6 | 有没有 `CONFIG_SMP=n` 的构建路径？ | 那条路径上 `smp_*` 全是 `barrier()` |

### 观测与调试

| 工具 | 用途 |
|------|------|
| ⭐ **KCSAN** | 内核的 data race 检测器。**它靠屏障建立 happens-before**（§3 的 `kcsan_mb()` 钩子），所以用错屏障类型会被它抓到 |
| **KASAN** | 不管顺序，管越界/UAF |
| **`CONFIG_DEBUG_ATOMIC_SLEEP`** | 抓"在原子上下文里睡了" |
| **litmus 测试** | `tools/memory-model/` 下有 herd7 + 一堆 litmus 测试，**可以形式化验证**一小段并发代码的所有可能执行结果 |
| ⭐ **LKMM（`tools/memory-model/`）** | 内核自己的内存模型（`linux-kernel.bell` / `cat` 文件）。**想确认"这样写够不够"，可以把它写成 litmus 跑一遍** |

> 📌 最强的一招：**把你的同步 pattern 写成 litmus 测试，用 herd7 跑一遍**。
> 比"我觉得应该没问题"可靠得多，也比在真机上等偶现 bug 快几个数量级。

---

## 实践模板

### 模板 A：发布-订阅（最常见）

```c
/* 发布者 */
WRITE_ONCE(data.value, 42);
smp_store_release(&data.ready, 1);        /* ⭐ release */

/* 订阅者 */
if (smp_load_acquire(&data.ready)) {      /* ⭐ acquire，与上面配对 */
	v = READ_ONCE(data.value);        /* 保证看到 42 */
}
```

**等价的"裸屏障"写法**（更啰嗦，但有时需要）：

```c
/* 发布者 */
WRITE_ONCE(data.value, 42);
smp_wmb();                                /* 只挡 store */
WRITE_ONCE(data.ready, 1);

/* 订阅者 */
if (READ_ONCE(data.ready)) {
	smp_rmb();                        /* 只挡 load */
	v = READ_ONCE(data.value);
}
```

**推荐第一种**：release/acquire 在 x86 上是零指令，
且**语义自解释**（读代码的人一眼看出这是发布-订阅）。

### 模板 B：自旋等待某个条件（用控制依赖省屏障）

```c
/* 等 obj->state 变成 READY，并 acquire 它之前的所有写 */
val = smp_cond_load_acquire(&obj->state, VAL == READY);
/* 等价于但比 smp_load_acquire() 更适合【等待】场景：
   它在 ARM 上能省掉循环里每轮一次的 barrier */
```

对比：

| 写法 | 每轮循环的开销（ARM） |
|------|---------------------|
| `while (smp_load_acquire(&s) != READY) cpu_relax();` | 每轮一条 `ldar` |
| ⭐ `smp_cond_load_acquire(&s, VAL == READY)` | 每轮普通 `ldr`，**只在退出时补一次 `dmb`** |

### 模板 C：MMIO 寄存器序列

```c
/* ✅ 正确：用访问器，顺序钩子自动插入 */
writel(INDEX_5, base + ADDR_REG);
val = readl(base + DATA_REG);       /* __io_ar() 保证在读之后 */

/* 需要"写完之后必须真的到设备"的场合 */
writel(cmd, base + CMD_REG);
/* 如果需要确保设备看到：用 readl() 回读（常见做法），或架构的 flush */
(void)readl(base + STATUS_REG);
```

⚠️ **不要**：

```c
/* ❌ 裸指针：编译器可以重排、合并、消除 */
volatile u32 __iomem *p = base;
p[ADDR_REG] = 5;
val = p[DATA_REG];
```

### 自检清单

| # | 检查 | 不通过怎么办 |
|---|------|------------|
| 1 | 每个写侧屏障都有**配对的读侧**屏障吗？ | 没有 → 几乎肯定是 bug（§6） |
| 2 | 该用 `smp_mb()` 的地方用了 `smp_wmb()` 吗？ | 有 load 参与就必须 `smp_mb()` |
| 3 | 用了 bitfield 做同步标志吗？ | ❌ 换成独立标量 + `set_bit()` 或共用一把锁（§5） |
| 4 | `READ_ONCE()` / `WRITE_ONCE()` 在**不同 C 语句**里吗？ | 同一语句里不保证顺序（§4） |
| 5 | 依赖控制依赖来省 load-load 屏障吗？ | ❌ load-load 控制依赖不成立，要真 `smp_rmb()`（§7） |
| 6 | 需要的是"全局顺序"还是"发布-订阅"？ | 全局顺序要 `smp_mb()`；RC 对不够（§1、§9） |
| 7 | 目标是 ARM 吗？ | 是 → `smp_wmb/rmb` 都是真指令，重新评估开销（§2） |
| 8 | `CONFIG_SMP=n` 上会构建吗？ | 会 → `smp_*` 退化成 `barrier()`；跨设备要用 `mb/rmb/wmb()` |
| 9 | 用了 `smp_mb__before_atomic()`？ | x86 上是空的（原子操作本身 serializing），但保留它没坏处 |
| 10 | MMIO 用的是 `readl/writel`？ | 不是 → 改（§11） |

---

## 易错点核对表

| # | 易错点 | 正确做法 |
|---|--------|---------|
| 1 | 说"x86 的 `smp_mb()` 是 `mfence`" | ❌ 是 **`lock; addl $0,-4(%rsp)`**（`mfence` 是 `mb()`） |
| 2 | 以为 `smp_wmb()` 也约束 load | ❌ 只约束 store↔store |
| 3 | 以为 `smp_rmb()` 也约束 store | ❌ 只约束 load↔load |
| 4 | 用 RELEASE+ACQUIRE 对代替全屏障 | ❌ 文档明说"**not** guaranteed to act as a full memory barrier" |
| 5 | 以为 `READ_ONCE()` 提供跨核顺序 | ❌ 它只防编译器；跨核要另加屏障 |
| 6 | 把两个 `READ_ONCE()` 写在**同一条 C 语句**里 | ❌ 顺序不保证，拆成两条语句 |
| 7 | 用 bitfield 做并行同步 | ❌ 编译器生成非原子 RMW；同一 bitfield 的所有字段还必须共用一把锁 |
| 8 | 用控制依赖省 load→**load** 的屏障 | ❌ CPU 会预测分支；必须真 `smp_rmb()` |
| 9 | 用控制依赖省 load→**store** 的屏障 | ✅ 这个成立（store 不被推测） |
| 10 | 在写代码侧屏障时不写读侧 | ❌ 屏障必须配对，"read barrier always has to be there" |
| 11 | 对 MMIO 用 `smp_*mb()` | ⚠️ UP 上退化成 `barrier()`；跨设备应该用 `mb/rmb/wmb()` 或访问器 |
| 12 | 驱动里手工调 `mmiowb()` | ⚠️ v6.6 已自动化（`mmiowb_set_pending()` + `mmiowb_spin_unlock()`） |
| 13 | 用 `barrier()` 当跨核屏障 | ❌ 它只约束编译器，等价于 C++ `atomic_signal_fence` |
| 14 | 以为 `mfence` 能管住 `WRMSR` | ❌ WRMSR 不是 load/store；需要 `mfence; lfence`（`weak_wrmsr_fence()`） |
| 15 | 用 `READ_ONCE()` 读聚合类型（struct） | ⚠️ `compiletime_assert_rwonce_type` 只允许原生字长或 `long long` |
| 16 | 删掉"x86 上看起来没用"的 `smp_wmb()` | ❌ 删了在 ARM 上就炸 |

---

## 常见陷阱

1. 混淆 smp_mb() / smp_rmb() / smp_wmb()——全屏障/读屏障/写屏障，保证不同方向的重排
2. 以为 x86 不需要 memory barrier——x86 TSO 下 smp_rmb/smp_wmb/release/acquire 都是零指令，但 store-load 重排仍需 barrier
3. 在 UP 上用 smp_mb()——UP 上退化成 barrier()；跨设备场景要用 mb/rmb/wmb()
4. **（v6.6 补充）** 说 `smp_mb()` 在 x86 上是 `mfence` —— 是 `lock addl`
5. **（v6.6 补充）** 拿 `barrier()` 当跨核屏障用（它只是编译屏障）
6. **（v6.6 补充）** 用 bitfield 做同步标志（非原子 RMW；且同 bitfield 各字段必须共用一把锁）
7. **（v6.6 补充）** 依赖"控制依赖"省 load-load 屏障（需要真 `smp_rmb()`）
8. **（v6.6 补充）** 拿 RELEASE+ACQUIRE 对当全屏障用
9. **（v6.6 补充）** 把两个 `READ_ONCE()` 写在同一条 C 语句里
10. **（v6.6 补充）** 在 ARM 上照搬 x86 "smp_wmb 免费"的假设

---

## 自测题

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `smp_mb()` / `smp_rmb()` / `smp_wmb()` 分别保证什么？

<details><summary>答案</summary>

smp_mb()：全屏障，之前的读写 + 之后的读写都不可跨屏障重排。smp_rmb()：读屏障，之前的读不可重排到之后的读之后。smp_wmb()：写屏障，之前的写不可重排到之后的写之后。x86 TSO 模型下：smp_rmb() = 空操作（loads 不重排），smp_wmb() = 空操作（stores 不重排），smp_mb() = `mfence`（禁止 store-load 重排）。ARM64：三者都是真实指令（`dmb ish`/`dmb ishld`/`dmb ishst`）。

<details><summary>按 v6.6 修订/补充</summary>

**三个语义定义都正确，但最后关于 x86 的部分有一处错误（第 16 条"凭记忆必错"）：**

> ⚠️ **x86 的 `smp_mb()` 不是 `mfence`，是 `lock; addl $0,-4(%rsp)`。**

`arch/x86/include/asm/barrier.h:57`（v6.6）：

```c
#define __smp_mb()	asm volatile("lock; addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")
```

`mfence` 对应的是 **`mb()`**（不带 `smp_` 前缀那个）：

```c
#define __mb()	asm volatile("mfence":::"memory")       /* :22 */
```

**为什么不用 `mfence`**：`lock` 前缀指令在多数微架构上比 `mfence` 快。
（`mfence` 有些实现还要额外等 store buffer 排空。）

**v6.6 x86-64 完整对照表**：

| 屏障 | 实际生成 |
|------|---------|
| `smp_wmb()` | **`barrier()`** —— 零指令 ✅ 原答案对 |
| `smp_rmb()` | **`dma_rmb()` = `barrier()`** —— 零指令 ✅ 原答案对 |
| `smp_mb()` | ⭐ **`lock; addl $0,-4(%rsp)`** ❌ 不是 `mfence` |
| `wmb()` | `sfence` |
| `rmb()` | `lfence` |
| `mb()` | `mfence` |

**另外补两条原答案没提的**：

1. **`smp_` 前缀的真正含义是 `CONFIG_SMP`**：
   ```c
   #else	/* !CONFIG_SMP */
   #define smp_mb()	barrier()
   #define smp_rmb()	barrier()
   #define smp_wmb()	barrier()
   #endif
   ```
   所以"UP 上用 smp_mb() 没意义"这条补充（速查表常见陷阱第 3 条）**是对的**，
   而且**所有三个** `smp_*` 都退化，不只是 `smp_mb()`。
2. **跨设备不能用 `smp_*`**，要用 `mb()/rmb()/wmb()` 或 `dma_*`——
   源码注释："And yes, this is required on UP too when we're talking to devices."

**关于 ARM 部分**：原答案"三者都是真实指令"方向正确，
但精确的助记符是 `dmb ish`（全）/ `dmb ishld`（读）/ `dmb ishst`（写）。
更省的是 release/acquire：`stlr` / `ldar`（比 `dmb` 便宜）。

</details>
</details>

**Q2.** `smp_store_release()` / `smp_load_acquire()` 相比 `smp_mb()` 有什么优势？

<details><summary>答案</summary>

smp_store_release(ptr, val)：等价于 smp_wmb() + WRITE_ONCE(*ptr, val)，只保证之前的读写不重排到这个 store 之后。smp_load_acquire(ptr)：等价于 READ_ONCE(*ptr) + smp_rmb()，只保证之后的读写不重排到这个 load 之前。优势：① 更精细——只关联一个操作，不影响其他操作。② 在 x86 上 release = 普通 store（无开销），acquire = 普通 load（无开销）。③ 代码更清晰。

<details><summary>按 v6.6 修订/补充</summary>

**三条优势都成立**，而且第 ② 条在 v6.6 的 x86 上**比你说的还要便宜**：

```c
/* arch/x86/include/asm/barrier.h:63 */
#define __smp_store_release(p, v)					\
do {									\
	compiletime_assert_atomic_type(*p);				\
	barrier();                      /* ⭐ 只编译屏障 */          \
	WRITE_ONCE(*p, v);						\
} while (0)

#define __smp_load_acquire(p)						\
({									\
	typeof(*p) ___p1 = READ_ONCE(*p);				\
	compiletime_assert_atomic_type(*p);				\
	barrier();                      /* ⭐ 只编译屏障 */          \
	___p1;								\
})
```

**x86 上 release/acquire 是【零额外指令】**——只有 `barrier()`。
对比 `smp_mb()`（= `lock addl`，~20-30 周期）。

**但注意：通用架构上它们并不便宜**（`asm-generic/barrier.h:139`）：

```c
#define __smp_store_release(p, v)					\
do {									\
	compiletime_assert_atomic_type(*p);				\
	__smp_mb();                     /* ⭐ 全屏障！ */            \
	WRITE_ONCE(*p, v);						\
} while (0)
```

→ 通用架构上 release 用的是**全屏障**（比 `smp_wmb()` 还贵）。
所以"release 一定比 smp_wmb 便宜"**只在特定架构（如 x86、ARM64）成立**。

**补一条被普遍忽略的硬约束**：目标类型必须过 `compiletime_assert_atomic_type(*p)`——
release/acquire 只能用于**原子类型**（`atomic_t` 或原生字长），对 `struct` 会编译失败。

**⚠️ 补一条最重要的限制**（文档 `memory-barriers.txt:504` 原文）：

> "a **RELEASE+ACQUIRE pair is -not- guaranteed to act as a full memory
> barrier.** However, after an ACQUIRE on a given variable, all memory
> accesses preceding any prior RELEASE on that **same variable** are
> guaranteed to be visible."

| 场景 | RC 对够吗 |
|------|----------|
| 发布-订阅、SPSC ring buffer 的 head/tail | ✅ 够（且是最佳选择） |
| seqlock 读写配对 | ⚠️ 不够——所以 v6.6 用的是 `smp_wmb()` + `smp_rmb()` 显式配对（10.8 §2） |
| 需要**全局**顺序（多变量、Dekker 类算法） | ❌ 必须 `smp_mb()` |

**还有个便宜可以捡**：`smp_cond_load_acquire()`（自旋等待场景）——
它利用"等待循环本身就是控制依赖"，只在**退出时**补一条
`smp_acquire__after_ctrl_dep()`（= `smp_rmb()`），
比"每轮循环都 `smp_load_acquire()`"在 ARM 上省很多。

</details>
</details>

**Q3.** HFT 中 memory barrier 误用会导致什么问题？

<details><summary>答案</summary>

① 缺少 barrier：消息传递 pattern 失败——`data = x; ready = true;` 如果 CPU 重排为 `ready = true; data = x;`，消费者看到 ready=true 但 data 还是旧值。② 过多 barrier：性能下降——每个 smp_mb() 在 x86 上是 `mfence`（~30 cycles），ARM64 上 `dmb`（~50 cycles）。HFT 无锁队列应精确用 release/acquire 替代 seq_cst。用 `std::atomic` + 正确的 memory_order 避免手动 barrier。

<details><summary>按 v6.6 修订/补充</summary>

**两条都对**，订正 + 补充：

**① 订正 x86 的指令**（同 Q1）：`smp_mb()` 在 x86 上是 **`lock addl`** 不是 `mfence`。
具体周期数随微架构差异很大——**不要信任何具体数字，自己用 `perf` 量**。

**② 补一个比"性能下降"严重得多的问题：bitfield。**

文档 `memory-barriers.txt:310` 的 anti-guarantees：

> "These guarantees **do not apply to bitfields**, because compilers often
> generate code to modify these using **non-atomic read-modify-write
> sequences**. **Do not attempt to use bitfields to synchronize parallel
> algorithms.**"
>
> "Even in cases where bitfields are protected by locks, **all fields in a
> given bitfield must be protected by one lock.** ... can cause an update to
> one field to **corrupt the value of an adjacent field**."

这在 HFT 里是**最典型的隐蔽 bug**：用 `struct { u64 ready:1; u64 ack:1; }`
当握手标志，两个字段分别由不同线程改 → **改一个会覆盖另一个**。
而且**加了锁也没用**（如果两把锁不同）。

**③ 补第三个问题：控制依赖的误用。**

```c
q = READ_ONCE(a);
if (q)
	p = READ_ONCE(b);      /* ❌ 控制依赖不保序！CPU 会预测分支提前加载 b */
```

想省这条 `smp_rmb()` 是不行的。**只有 load→store 的控制依赖成立**
（因为 store 不被推测）：

```c
q = READ_ONCE(a);
if (q)
	WRITE_ONCE(b, 1);      /* ✅ 这个成立 */
```

**④ 补第四个问题：编译器会把 `if` 整个消掉**（文档原文例子）：
如果编译器能证明 `a` 恒非零，它会把
`q = READ_ONCE(a); if (q) WRITE_ONCE(b, 1);` 优化成
`q = a; b = 1;` —— 条件没了，顺序保证随之消失。
**所以 `READ_ONCE()` 一个都不能省。**

**⑤ 关于"用 release/acquire 替代 seq_cst"——完全同意，并且给出 v6.6 的对应表**：

| 内核 | C++ |
|------|-----|
| `READ_ONCE()`（无屏障） | `relaxed` |
| `smp_load_acquire()` | `acquire` |
| `smp_store_release()` | `release` |
| `smp_mb()` | `seq_cst`（且更强） |
| `barrier()` | ⭐ **`atomic_signal_fence`**（**不是** `atomic_thread_fence`） |

⚠️ 最后一行是最容易翻译错的一处。

**⑥ 补一个"怎么验证"的手段**：内核自带
**`tools/memory-model/`（LKMM）**——可以把你的同步 pattern 写成 litmus 测试，
用 `herd7` 穷举所有合法执行结果。**比在真机上等偶现 bug 快几个数量级。**

</details>
</details>

**Q4.** （v6.6 新增）`READ_ONCE()` 到底保证了什么？不保证什么？

<details><summary>答案</summary>

**它保证的（编译器层面，零指令）**：

1. **单次访问**：不会被编译器拆成多次（如 64 位变量在 32 位上被拆成两次 32 位读）；
2. **不合并、不消除**：不会和相邻的同类访问合并，不会被"优化掉"；
3. **相邻 ONCE 访问之间不重排**——⭐ **但有限制**（见下）；
4. **隐式地址依赖屏障**（v5.9 起）：`Q = READ_ONCE(P); D = READ_ONCE(*Q);` 的顺序成立。

（`asm-generic/rwonce.h:44` 的实现就是 `volatile` 强制转换：
`(*(const volatile __unqual_scalar_typeof(x) *)&(x))`。）

**它【不】保证的**：

| 不保证 | 说明 |
|--------|------|
| ❌ **任何 CPU 顺序** | 它不生成任何指令，跨核顺序必须另外加 `smp_*mb()` / release-acquire |
| ❌ **原子性**（多字节聚合类型） | 头文件注释直言 `__READ_ONCE()` "may result in tears" |
| ⚠️ **同一条 C 语句里的两个 ONCE 访问** | 头文件注释："only when the compiler is aware of some particular ordering. One way ... is to put the two invocations ... **in different C statements**" |

**类型限制**（`compiletime_assert_rwonce_type`）：

```c
compiletime_assert(__native_word(t) || sizeof(t) == sizeof(long long),
	"Unsupported access size for {READ,WRITE}_ONCE().")
```

→ 只允许原生字长或 `long long`。**`struct` / bitfield 会编译失败**（bitfield 还有更严重的语义问题，见 §5）。

**两个官方用例**（头文件注释 `:13-18`）：

1. **同一 CPU 上**进程代码与 irq/NMI 处理函数之间的通信；
2. 防止编译器折叠/破坏那些"本身不需要顺序、或由显式屏障/原子指令提供顺序"的访问。

⭐ **一句话**：`READ_ONCE()` 是**给编译器看的**，不是给 CPU 看的。
需要跨核顺序，请另加屏障。

</details>

**Q5.** （v6.6 新增）为什么"控制依赖"有时候能省屏障、有时候不能？

<details><summary>答案</summary>

因为**只有 load→store 的控制依赖成立，load→load 的不成立**。

**load→load：不成立**（文档 `:682` 的例子）

```c
q = READ_ONCE(a);
if (q) {
	/* BUG: No address dependency!!! */
	p = READ_ONCE(b);
}
```

文档解释：

> "there is no actual address dependency, but rather a control dependency
> **that the CPU may short-circuit by attempting to predict the outcome in
> advance**, so that other CPUs see the load from b as having happened
> before the load from a."

CPU **会预测分支**，提前把 `b` 加载了。必须显式加 `smp_rmb()`：

```c
q = READ_ONCE(a);
if (q) {
	<read barrier>          /* ⭐ 必须 */
	p = READ_ONCE(b);
}
```

**load→store：成立**（文档 `:708`）

```c
q = READ_ONCE(a);
if (q) {
	WRITE_ONCE(b, 1);       /* ✅ 不需要额外屏障 */
}
```

文档："**stores are not speculated**. This means that ordering *is* provided
for load-store control dependencies."

**对比"地址依赖"**——这个是成立的，而且由 `READ_ONCE()` **隐式**提供：

```c
Q = READ_ONCE(P);
D = READ_ONCE(*Q);          /* ⭐ 第二个 load 的【地址】来自第一个 load */
```

**三者的判定表**：

| 依赖类型 | 第二个操作 | 地址来自第一个吗？ | 成立？ | 需要额外屏障 |
|---------|-----------|-----------------|-------|------------|
| 地址依赖 | load | ✅ | ✅ | 不需要（READ_ONCE 隐式提供） |
| 控制依赖 | load | ❌ | ❌ | ⭐ 需要 `smp_rmb()` |
| 控制依赖 | store | ❌ | ✅ | 不需要（store 不被推测） |

**还有两个编译器层面的陷阱**：

1. **编译器会消掉 `if`**：能证明 `a` 恒非零时，
   `q = READ_ONCE(a); if (q) WRITE_ONCE(b,1);`
   会被优化成 `q = a; b = 1;`（条件没了 → 顺序保证消失）。
2. **两个分支相同的 store 会被上提**：

   ```c
   /* 你写的 */
   q = READ_ONCE(a);
   if (q)      { barrier(); WRITE_ONCE(b, 1); do_x(); }
   else        { barrier(); WRITE_ONCE(b, 1); do_y(); }

   /* 高优化级别下变成 */
   q = READ_ONCE(a);
   barrier();
   WRITE_ONCE(b, 1);       /* BUG: 条件没了 */
   if (q) { do_x(); } else { do_y(); }
   ```

   文档结论："The conditional is **absolutely required**, and must be present
   in the assembly code even after all compiler optimizations have been applied."

**v6.6 的现成工具**：`smp_cond_load_acquire()` 正是利用
"自旋等待 = 控制依赖"来省屏障：

```c
_val = smp_cond_load_relaxed(ptr, cond_expr);   /* 每轮普通 load */
smp_acquire__after_ctrl_dep();                  /* 只在退出时补 smp_rmb() */
```

</details>

**Q6.** （v6.6 新增）驱动里对 MMIO 的写，需要自己调 `mmiowb()` 吗？

<details><summary>答案</summary>

**不需要——v6.6 里这件事已经自动化了。**

`include/asm-generic/io.h:44`（v6.6）：

```c
/* serialize device access against a spin_unlock, usually handled there. */
#ifndef __io_aw
#define __io_aw()      mmiowb_set_pending()
#endif
```

机制在 `include/asm-generic/mmiowb.h`（v6.6）：

```c
static inline void mmiowb_set_pending(void)      /* I/O 写访问器里自动调 */
{
	struct mmiowb_state *ms = __mmiowb_state();
	if (likely(ms->nesting_count))
		ms->mmiowb_pending = ms->nesting_count;    /* 记录"持锁期间写过 I/O" */
}

static inline void mmiowb_spin_unlock(void)      /* spin_unlock 里自动调 */
{
	struct mmiowb_state *ms = __mmiowb_state();
	if (unlikely(ms->mmiowb_pending)) {
		ms->mmiowb_pending = 0;
		mmiowb();                             /* ⭐ 只有真写过才 flush */
	}
	ms->nesting_count--;
}
```

**三个要点**：

1. **"记录 + 延迟 flush"**：`mmiowb_set_pending()` 只是打标记；
   真正的 `mmiowb()` 在 **`spin_unlock()` 时**才执行，
   而且**只有真的写过 I/O 才执行**（`if (unlikely(ms->mmiowb_pending))`）。
2. **`CONFIG_MMIOWB=n` 时全是空宏**：
   ```c
   #define mmiowb_set_pending()		do { } while (0)
   #define mmiowb_spin_lock()		do { } while (0)
   #define mmiowb_spin_unlock()		do { } while (0)
   ```
3. **`mmiowb()` 本身还在**，但它现在是**架构钩子**，不是驱动 API——
   只有 `asm/mmiowb.h` 的实现者和 `mmiowb_spin_unlock()` 会调它。

**那驱动该做什么？** ⭐ **用 `readl()` / `writel()` 而不是裸指针**。

`asm-generic/io.h` 定义的四个钩子会被访问器自动插入：

| 钩子 | 位置 | 默认实现 |
|------|------|---------|
| `__io_br()` | 读之前 | `barrier()` |
| `__io_ar()` | 读之后 | `__io_br()` |
| `__io_bw()` | 写之前 | `barrier()` |
| `__io_aw()` | 写之后 | ⭐ `mmiowb_set_pending()` |

**裸指针写 MMIO 会同时丢掉四样东西**：volatile 语义、顺序钩子、字节序转换、
以及 mmiowb 追踪。

**顺带**：那个头文件的注释是内核里少见的幽默——给架构移植者的 "FIVE easy steps"
最后一步是 **"5. Complain to your architects"**（认真的）。

</details>

</details>

---

→ [10.9 禁止抢占](./section-10.9-禁止抢占.md) · [10.11 选型速查](./section-10.11-选型速查Ch-9--Ch-10.md) · [10.1 原子操作](./section-10.1-原子操作.md) · [10.8 seqlock](./section-10.8-顺序锁.md)

---
