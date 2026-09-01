## ① 原子操作 · Atomic Operations

**其他同步机制的基础** — 对共享变量的修改在指令级 **不可分割**：要么全做完，要么完全没做；中间状态对其它 CPU/中断不可见。

> **本篇分工**：实体书已讲原子操作的基本概念与 API 用法。本篇**不复述这些**，只做三件事：
> ① 讲清 **v6.6 里 atomic API 的三层结构**（`arch_atomic_*` / `atomic_*` / `raw_atomic_*`，
> 书上完全没有，因为这是 **v5.8 才重构出来的**）；
> ② 给出 **Linux 自己的 atomic 内存序规则** —— 它和 C11 `memory_order` **不是一套东西**，
> 且这条规则在 x86 上有个非常省钱的推论；
> ③ 用 v6.6 的 `refcount.h` 头部注释逐字讲清 **refcount_t 的"饱和"设计**，
> 包括那段"为何不引入 cmpxchg 循环"的工程取舍论证。
>
> 所有常量与代码均核对自缓存的 v6.6 源码，行号可查。

---

## 1. v6.6 的 atomic API 是**三层**的（书上只有一层）

书的年代，`atomic_inc()` 就是架构头里的一个静态内联函数。现在不是了。

```c
/* include/linux/atomic.h —— v6.6 全文骨架 */
#include <asm/atomic.h>
#include <asm/barrier.h>
...
#include <linux/atomic/atomic-arch-fallback.h>
#include <linux/atomic/atomic-long.h>
#include <linux/atomic/atomic-instrumented.h>
```

三个头文件各管一段：

| 层 | 头文件 | 命名 | 职责 |
|----|--------|------|------|
| **架构实现** | `arch/x86/include/asm/atomic.h` | `arch_atomic_*` | 真正的内联汇编。架构没实现的操作由下一层补 |
| **通用 fallback** | `linux/atomic/atomic-arch-fallback.h` | `arch_atomic_*` | 架构没提供的操作，用已有原语拼（如用 `cmpxchg` 循环模拟 `fetch_or`） |
| **插桩包装** | `linux/atomic/atomic-instrumented.h` | `atomic_*` | 给每个操作套上 **KASAN / KCSAN** 检查，再转调 `arch_atomic_*` |

于是调用链是：

```
atomic_inc(&v)                      ← 你写的
  └─ arch_atomic_inc(&v)            ← atomic-instrumented.h（带 KCSAN 数据竞争检测）
       └─ asm volatile(LOCK_PREFIX "incl %0")   ← arch/x86/.../atomic.h
```

x86 的实现长这样：

```c
/* arch/x86/include/asm/atomic.h:22-63 */
static __always_inline int arch_atomic_read(const atomic_t *v)
{
	/*
	 * Note for KASAN: we deliberately don't use READ_ONCE_NOCHECK() here,
	 * it's non-inlined function that increases binary size and stack usage.
	 */
	return __READ_ONCE((v)->counter);
}

static __always_inline void arch_atomic_set(atomic_t *v, int i)
{
	__WRITE_ONCE(v->counter, i);
}

static __always_inline void arch_atomic_add(int i, atomic_t *v)
{
	asm volatile(LOCK_PREFIX "addl %1,%0"
		     : "+m" (v->counter)
		     : "ir" (i) : "memory");
}

static __always_inline void arch_atomic_inc(atomic_t *v)
{
	asm volatile(LOCK_PREFIX "incl %0"
		     : "+m" (v->counter) :: "memory");
}
```

> 注意 `arch_atomic_read` 就是 `__READ_ONCE`，`arch_atomic_set` 就是 `__WRITE_ONCE` ——
> **它们是普通读写**，不是原子 RMW。这与常见误解（"atomic_read 有原子性"）相反。

### 1.1 ⚠️ 版本断崖：这个三层结构是 **v5.8** 才有的

用多版本同名文件 diff 定位（skill 的标准手法）：

| 版本 | `atomic-instrumented.h` | `atomic-arch-fallback.h` |
|------|-------------------------|--------------------------|
| v5.6 | ❌ | ❌ |
| **v5.7** | ❌ | ❌ |
| **v5.8** | ✅ | ✅ |
| v6.6 | ✅ | ✅ |

**断崖在 v5.8**（Peter Zijlstra 主导的 atomic 头文件重构）。

**为什么要这么拆**：
- 以前每个架构都要把所有 atomic 操作实现一遍（有几百个），漏一个就可能静默出错
- 现在架构**只实现自己有硬件支持的**，剩下的由 `atomic-arch-fallback.h` 用通用原语自动拼
- 插桩（KCSAN 检测数据竞争、KASAN 检测越界）只需要写在**一层**里，所有架构自动受益

**实践含义**：写驱动时用到的 `atomic_*` 是**带插桩的那层**。
如果在极热路径上想省掉插桩开销，可以用 `raw_atomic_*`（直接到架构层）—— 但会丢掉 KCSAN 保护。

### 1.2 `LOCK_PREFIX`：UP 机器上 lock 前缀会被**运行时改成 nop**

```c
/* arch/x86/include/asm/alternative.h:39-51 */
#ifdef CONFIG_SMP
#define LOCK_PREFIX_HERE \
		".pushsection .smp_locks,\"a\"\n"	\
		".balign 4\n"				\
		".long 671f - .\n" /* offset */		\
		".popsection\n"				\
		"671:"

#define LOCK_PREFIX LOCK_PREFIX_HERE "\n\tlock; "

#else /* ! CONFIG_SMP */
#define LOCK_PREFIX_HERE ""
#define LOCK_PREFIX ""
#endif
```

`LOCK_PREFIX_HERE` 把每条 `lock` 指令的地址**记进 `.smp_locks` 段**。
启动时若发现是单处理器，内核会遍历这张表，把每个 `lock` 前缀**就地替换成 nop**。

> 与 15.1 讲的 `alternative_io`（LA57 运行时判定）、15.7 的 `pgtable_l5_enabled()` 是
> **同一套机制的两个应用**：把"启动时才知道的事"变成"零运行时分支"。

---

## 2. `atomic_t` 的真实定义与 API 全景

```c
/* include/linux/types.h:172-182 */
typedef struct {
	int counter;
} atomic_t;

#define ATOMIC_INIT(i) { (i) }

#ifdef CONFIG_64BIT
typedef struct {
	s64 counter;
} atomic64_t;
#endif
```

| 事实 | 说明 |
|------|------|
| `atomic_t` 是 **`int`**，32 位 | 书上说"至少 32 位"，实际在 v6.6 所有架构上就是 32 位 |
| `atomic64_t` 是 **`s64`**，有符号 | 注意是 `s64` 不是 `u64` |
| `atomic64_t` **只在 `CONFIG_64BIT` 下存在** | 32 位系统上没有这个类型 |
| 包在 `struct` 里 | 为了**类型安全**：`atomic_t*` 不能和 `int*` 混用 |

### 2.1 为什么不用裸 `int`

| 理由 | 说明 |
|------|------|
| **类型安全** | 原子 API 只接 `atomic_t*`，把裸 `int` 传进 `atomic_inc()` 会**编译报错** |
| **屏蔽架构差异** | x86 用 `lock` 前缀、ARM64 用 LSE 指令 / LL-SC 循环，藏在头文件后面 |
| **防错误优化** | 编译器不能把 `atomic_t` 的访问当成普通变量随意重排或合并 |
| **插桩点** | 有了统一类型，KCSAN/KASAN 才能统一挂钩 |

### 2.2 API 分类（v6.6 完整分档）

| 类别 | 例子 | 返回旧值？ | 内存序 |
|------|------|-----------|--------|
| **非 RMW** | `atomic_read`、`atomic_set` | — | **无屏障**（就是 `READ_ONCE`/`WRITE_ONCE`） |
| **RMW（不返回）** | `atomic_inc`、`atomic_dec`、`atomic_add`、`atomic_sub`、`atomic_and/or/xor` | ❌ | **full barrier** |
| **RMW（返回）** | `atomic_add_return`、`atomic_inc_return`、`atomic_fetch_add`、`atomic_dec_and_test`、`atomic_inc_and_test`、`atomic_add_negative`、`atomic_sub_and_test` | ✅ | **full barrier** |
| **交换 / 比较交换** | `atomic_xchg`、`atomic_cmpxchg`、`atomic_try_cmpxchg` | ✅ | **full barrier** |
| **显式序变体** | `atomic_*_relaxed`、`_acquire`、`_release` | 视基类 | 见第 3 节 |

---

## 3. ⚠️ 订正：Linux 的 atomic 内存序规则，**不是 C11 那套**

这是最容易踩的坑。Linux **没有**采用 C11 `memory_order` 的六档模型，
而是自己定义了一套以"缺省就最强"为原则的规则。

```c
/* include/linux/atomic.h:11-24 —— 注释逐字 */
/*
 * Relaxed variants of xchg, cmpxchg and some atomic operations.
 *
 * We support four variants:
 *
 * - Fully ordered: The default implementation, no suffix required.
 * - Acquire: Provides ACQUIRE semantics, _acquire suffix.
 * - Release: Provides RELEASE semantics, _release suffix.
 * - Relaxed: No ordering guarantees, _relaxed suffix.
 *
 * For compound atomics performing both a load and a store, ACQUIRE
 * semantics apply only to the load and RELEASE semantics only to the
 * store portion of the operation. Note that a failed cmpxchg_acquire
 * does -not- imply any memory ordering constraints.
 */
```

**四档规则**：

| 写法 | 语义 | 对应 C11 |
|------|------|----------|
| `atomic_inc(&v)`（无后缀） | **full barrier**（前后都挡） | `memory_order_seq_cst` |
| `atomic_inc_return_acquire(&v)` | acquire（只挡后面） | `memory_order_acquire` |
| `atomic_inc_return_release(&v)` | release（只挡前面） | `memory_order_release` |
| `atomic_inc_return_relaxed(&v)` | **无屏障**，只保证原子性 | `memory_order_relaxed` |
| `atomic_read(&v)` / `atomic_set()` | **无屏障** | `memory_order_relaxed` |

实现方式是"在 relaxed 版本外面加显式屏障"：

```c
/* include/linux/atomic.h:41-69 */
#define __atomic_op_acquire(op, args...)				\
({									\
	typeof(op##_relaxed(args)) __ret  = op##_relaxed(args);		\
	__atomic_acquire_fence();					\
	__ret;								\
})

#define __atomic_op_release(op, args...)				\
({									\
	__atomic_release_fence();					\
	op##_relaxed(args);						\
})

#define __atomic_op_fence(op, args...)					\
({									\
	typeof(op##_relaxed(args)) __ret;				\
	__atomic_pre_full_fence();					\
	__ret = op##_relaxed(args);					\
	__atomic_post_full_fence();					\
	__ret;								\
})
```

### 3.1 ⭐ x86 上的重要推论：acquire/release 变体**零额外开销**

```c
/* arch/x86/include/asm/barrier.h:79-80 */
#define __smp_mb__before_atomic()	do { } while (0)
#define __smp_mb__after_atomic()	do { } while (0)
```

**在 x86 上这两个屏障是空操作。**

原因：x86 的 `lock` 前缀指令本身就是 **full barrier**（TSO 模型下 `lock` 指令前后都不允许重排），
所以"在 relaxed 原子操作外加屏障"这件事**一件事都不用做**。

| 架构 | `atomic_*_acquire` 的额外开销 |
|------|------------------------------|
| **x86-64** | **零**（`__atomic_acquire_fence` 展开成空） |
| ARM64 | 需要 `ldar`/`stlr` 或显式 `dmb`，**有开销** |

> **HFT 含义**：在 x86 上，`atomic_*_relaxed` 相比普通 `atomic_*` **省不下屏障**
> （因为 RMW 的 lock 指令本来就是 full barrier，relaxed 也一样要用 lock 指令）。
> relaxed 的收益主要体现在：① 非 x86 架构；② 让 KCSAN/稀疏检查更准确地反映你的意图；
> ③ 表达设计意图给读代码的人。
> **真正能省开销的是：把 RMW 换成 `READ_ONCE` + `WRITE_ONCE`（单生产者场景），
> 或者用 `atomic_read`/`atomic_set` 的 relaxed 语义。**

### 3.2 x86 的 `smp_mb()` 是怎么实现的

```c
/* arch/x86/include/asm/barrier.h:57 */
#define __smp_mb()	asm volatile("lock; addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")
```

用一条 **`lock addl $0, -4(%rsp)`** 当屏障 —— 对栈顶下面 4 字节做无意义的加锁加法。
选它是因为：
1. `lock` 指令天然是 full barrier
2. 写 `-4(%rsp)` 的位置是 **red zone / 未使用区**，不影响数据
3. 比 `mfence` 快（老 CPU 上 `mfence` 更慢）

---

## 4. x86-64 上的真实指令

| C 代码 | 生成的指令 | 说明 |
|--------|-----------|------|
| `atomic_read(&v)` | `movl (%rdi), %eax` | 普通读，**没有 lock** |
| `atomic_set(&v, 1)` | `movl $1, (%rdi)` | 普通写，**没有 lock** |
| `atomic_inc(&v)` | `lock incl (%rdi)` | |
| `atomic_add(3, &v)` | `lock addl $3, (%rdi)` | 注意是 `addl` 不是 `incl` |
| `atomic_xchg(&v, n)` | `xchg %eax, (%rdi)` | `xchg` **隐含 lock**，不需前缀 |
| `atomic_cmpxchg(&v, o, n)` | `lock cmpxchgl %eax, (%rdi)` | 返回的是**旧值** |
| `smp_mb()` | `lock addl $0, -4(%rsp)` | |

`arch_atomic_add` 用 `addl` 而 `arch_atomic_inc` 用 `incl` ——
这是**源码里就写死的**（见 1 节的两段内联汇编），不是编译器选的。

**开销的量级**（不给死数字，给量纲）：

| 场景 | 量级 | 机制 |
|------|------|------|
| 无争用、line 已在本地 L1 | **十几到几十 cycle** | `lock` 指令本身不算慢，慢在它必须获得 cache line 的独占权（MESI 的 M 态） |
| 无争用、line 在别的核 | + 一次跨核传输（**几十 cycle**） | cache line 迁移 |
| **有争用**（多核抢同一 line） | **数百 cycle 起，且随争用核数非线性恶化** | cache line ping-pong |
| 跨 NUMA 节点争用 | 再翻一倍量级 | 走 QPI/UPI |

```bash
# 量测方法（自己机器上跑一次，比背数字有用）
# 1) 看 cache line 争用
perf c2c record -a -- ./your_app
perf c2c report --stdio        # 找 HITM（Hit-Modified）热点

# 2) 看原子操作占比
perf stat -e mem_inst_retired.lock_loads ./your_app
```

---

## 5. `atomic64_t` 与 `atomic_long_t`

```c
/* include/linux/types.h:178-182 */
#ifdef CONFIG_64BIT
typedef struct {
	s64 counter;
} atomic64_t;
#endif
```

| 类型 | 用途 | 32 位系统上 |
|------|------|------------|
| `atomic_t` | 32 位原子 | 存在 |
| `atomic64_t` | 64 位原子 | **不存在**（`CONFIG_64BIT` 守卫） |
| `atomic_long_t` | 跟随 `long` 宽度的原子 | 存在，`atomic-long.h` 把它映射到 `atomic_t` |

`include/linux/atomic/atomic-long.h` 做的事：64 位机上把 `atomic_long_*` 全部
`#define` 成 `atomic64_*`，32 位机上定义成 `atomic_*`。
**写"跟指针一样宽"的计数器时用 `atomic_long_t`**，可移植性最好。

> ⚠️ 32 位机上 `atomic64_t` 的实现曾经依赖一个**全局散列表的锁**（`atomic64_lock`），
> 代价极高。现代 32 位架构（如 ARMv7）多已实现原生的 64 位原子（LPAE / `ldrexd`）。
> 但**能不用就别用** —— 这也是为什么内核里大量用 `unsigned long` + `atomic_long_t` 而不是 64 位。

---

## 6. 原子位操作

| 原子版 | 非原子版（`__` 前缀） | 语义 |
|--------|---------------------|------|
| `set_bit(nr, addr)` | `__set_bit` | 置位 |
| `clear_bit(nr, addr)` | `__clear_bit` | 清零 |
| `change_bit(nr, addr)` | `__change_bit` | 翻转 |
| `test_and_set_bit(nr, addr)` | `__test_and_set_bit` | 置位并返回旧值 |
| `test_and_clear_bit(nr, addr)` | `__test_and_clear_bit` | 清零并返回旧值 |
| `test_bit(nr, addr)` | — | 只读测试（本来就不需原子） |

**关键区别**：
- **原子版**带 `lock` 前缀，可用于**跨 CPU 共享**的位图
- **非原子版**（`__` 前缀）是普通读改写，**必须在自旋锁保护下用**，
  或用于确定只有一个 CPU 会碰的数据

```c
/* 典型用法：内核里大量位图（cpumask、page flags、futex 位） */
if (!test_and_set_bit(PG_locked, &page->flags)) {
	/* 我是第一个设置这个位的 —— 我拿到了"锁" */
}
```

> **HFT 含义**：位图 + `test_and_set_bit` 是内核里最轻量的"抢占式标志"，
> 不需要分配锁对象。但要小心 **false sharing** —— 同一个 cache line 里的不同 bit
> 被不同核原子操作，照样 ping-pong。热路径上的标志位要**各自独占一个 cache line**。

---

## 7. ⭐ `refcount_t`：饱和设计（v6.6 头部注释逐字）

书的年代还没有 `refcount_t`（它是 **v4.11** 引入的）。这是"引用计数专用"的原子类型。

```c
/* include/linux/refcount.h:115-117 */
#define REFCOUNT_INIT(n)	{ .refs = ATOMIC_INIT(n), }
#define REFCOUNT_MAX		INT_MAX
#define REFCOUNT_SATURATED	(INT_MIN / 2)
```

| 常量 | 值 | 十六进制 |
|------|----|----------|
| `REFCOUNT_MAX` | `INT_MAX` | `0x7FFF_FFFF` |
| `REFCOUNT_SATURATED` | `INT_MIN / 2` | **`0xC000_0000`** |

### 7.1 设计核心：饱和，而不是阻止

```c
/* include/linux/refcount.h:7-56 —— 注释逐字（节选） */
/*
 * Variant of atomic_t specialized for reference counts.
 *
 * The interface matches the atomic_t interface (to aid in porting) but only
 * provides the few functions one should use for reference counting.
 *
 * Saturation semantics
 * ====================
 *
 * refcount_t differs from atomic_t in that the counter saturates at
 * REFCOUNT_SATURATED and will not move once there. This avoids wrapping the
 * counter and causing 'spurious' use-after-free issues. In order to avoid the
 * cost associated with introducing cmpxchg() loops into all of the saturating
 * operations, we temporarily allow the counter to take on an unchecked value
 * and then explicitly set it to REFCOUNT_SATURATED on detecting that underflow
 * or overflow has occurred. Although this is racy when multiple threads
 * access the refcount concurrently, by placing REFCOUNT_SATURATED roughly
 * equidistant from 0 and INT_MAX we minimise the scope for error:
 *
 * 	                           INT_MAX     REFCOUNT_SATURATED   UINT_MAX
 *   0                          (0x7fff_ffff)    (0xc000_0000)    (0xffff_ffff)
 *   +--------------------------------+----------------+----------------+
 *                                     <---------- bad value! ---------->
 *
 * (in a signed view of the world, the "bad value" range corresponds to
 * a negative counter value).
 */
```

**这段注释是整个设计最精彩的地方，逐句解读**：

1. **问题**：`atomic_t` 从 0 再减就回绕到 `0xFFFFFFFF`（有符号看是 -1），
   再 `atomic_dec_and_test` 就好几次才回到 0 —— 攻击者可以精确控制释放时机 → **UAF**
2. **朴素解法**：给每个操作加 `cmpxchg` 循环做饱和检查 → **太慢**（引用计数是极热路径）
3. **内核的解法**：**先让它减，检测到下溢/溢出后，再显式设成饱和值**
   - 这个修复**本身是 racy 的**（检查与设置之间别人可能又改了）
   - 但通过把饱和值放在 `0xC0000000`（**距 0 和 INT_MAX 大致等距**），
     把"逃逸出饱和区"的难度最大化
4. **安全性论证**（注释给的算术）：

```
    (UINT_MAX+1-REFCOUNT_SATURATED) / PID_MAX_LIMIT =
    0x40000000 / 0x400000 = 0x100 = 256
```

   要从饱和值一路涨（或跌）回"正常值"，需要 **256 次**连续的竞态命中。
   考虑到调度时序和需要连续命中，注释的结论是 "there doesn't appear to be a practical
   avenue of attack"。

**这就是内核典型的工程取舍**：不接受"每个 refcount 操作都加 cmpxchg 循环"的性能代价，
换一个有理论窗口但**实践中不可利用**的竞态，同时把故障模式从 **UAF（任意代码执行）**
降级为 **WARN + 内存泄漏**。

### 7.2 五种饱和类型

```c
/* include/linux/refcount.h:119-125 */
enum refcount_saturation_type {
	REFCOUNT_ADD_NOT_ZERO_OVF,
	REFCOUNT_ADD_OVF,
	REFCOUNT_ADD_UAF,
	REFCOUNT_SUB_UAF,
	REFCOUNT_DEC_LEAK,
};
```

| 类型 | 触发场景 | 后果 |
|------|----------|------|
| `REFCOUNT_ADD_NOT_ZERO_OVF` | `refcount_inc_not_zero` 时已饱和 | 对象正在被释放，还想要引用 → **漏引用** |
| `REFCOUNT_ADD_OVF` | `refcount_add` 溢出 | 计数爆炸 |
| `REFCOUNT_ADD_UAF` | 对已释放对象增引用 | **UAF 攻击信号** |
| `REFCOUNT_SUB_UAF` | 对已释放对象减引用 | **UAF 攻击信号** |
| `REFCOUNT_DEC_LEAK` | 减到 0 以下 | 泄漏（对象永不释放） |

### 7.3 `refcount_dec` 的真实实现

```c
/* include/linux/refcount.h:336-361 */
static inline void __refcount_dec(refcount_t *r, int *oldp)
{
	int old = atomic_fetch_sub_release(1, &r->refs);

	if (oldp)
		*oldp = old;

	if (unlikely(old <= 1))
		refcount_warn_saturate(r, REFCOUNT_DEC_LEAK);
}

static inline void refcount_dec(refcount_t *r)
{
	__refcount_dec(r, NULL);
}
```

⚠️ **注意**：`refcount_dec()` **只是 WARN，并不"阻止"减到 0 以下**。
它用的是 `atomic_fetch_sub_release`（无 cmpxchg 循环）。
真正的保护是：
- 一旦进入饱和区，值不再有意义，后续的 `refcount_dec_and_test` 不会返回 true
- 于是对象**永不释放** → 故障模式变成**内存泄漏**而不是 UAF

`refcount_dec_and_test()` 的内存序注释值得单独看：

```c
/* include/linux/refcount.h:319-334 */
/**
 * refcount_dec_and_test - decrement a refcount and test if it is 0
 * @r: the refcount
 *
 * Similar to atomic_dec_and_test(), it will WARN on underflow and fail to
 * decrement when saturated at REFCOUNT_SATURATED.
 *
 * Provides release memory ordering, such that prior loads and stores are done
 * before, and provides an acquire ordering on success such that free()
 * must come after.
 */
```

**release + 成功时 acquire** —— 这正是引用计数需要的：
- 减之前的所有读写不能跑到减之后（否则别人可能看到半初始化状态）
- 归零成功时要 acquire，保证 `free()` 发生在我之前的所有访问之后

### 7.4 `atomic_t` vs `refcount_t` 对照

| | `atomic_t` | `refcount_t` |
|---|---|---|
| 引入版本 | 原始 | **v4.11** |
| 语义 | 通用原子整数 | **只用于引用计数** |
| 0 再减 | **回绕到 -1 / UINT_MAX** → 可精确控制释放 → **UAF** | 饱和 + WARN → **泄漏** |
| 溢出 | 静默回绕 | 饱和 + WARN |
| API 数量 | 几十个 | **只暴露引用计数该用的那几个**（`inc`/`dec`/`dec_and_test`/`inc_not_zero`…） |
| 内存序 | 默认 full barrier | `release`，成功时 `acquire` |
| 引入版本 | — | v4.11（2017，针对 CVE 级 refcount 溢出攻击潮） |

---

## 8. 常见陷阱（在原 3 条基础上扩充到 7 条）

1. **把原子操作当万能锁** —— 只保证单操作原子性，多操作组合仍需锁
2. **混淆 `atomic_t` 和 `refcount_t`** —— 见 7.4 对照表
3. **以为 `atomic_read()` 是原子 RMW** —— 它**就是 `READ_ONCE`**，普通读，
   不提供任何跨 CPU 的同步保证。它保证的只是"这次读不会被编译器拆坏/优化掉"
4. **在 x86 上期待 `_relaxed` 能省很多** —— RMW 的 `lock` 指令本来就是 full barrier，
   relaxed 省不下 barrier（见 3.1）。真正的优化是**换算法**（减少共享、批处理、per-CPU）
5. **忽略 false sharing** —— 两个无关的原子变量落在同一 cache line，
   照样 ping-pong。热路径变量要 `__cacheline_aligned_in_smp`
6. **用 `atomic_t` 做"标志位 + 判断 + 动作"** —— `if (atomic_read(&flag) == X) do_something()`
   不是原子的，读完之后状态可能已经变了。要**读-改-判断**一气呵成用
   `atomic_cmpxchg` / `atomic_fetch_*` / `atomic_dec_and_test`
7. **位操作的原子版与非原子版混用** —— 同一位图上，一边用 `set_bit` 一边用 `__set_bit`，
   等于没有原子性

---

## 9. HFT 关联

| 场景 | 建议 |
|------|------|
| 热路径计数器（包数、字节数、序号） | **per-CPU 计数 + 读时聚合**，比跨核原子快 1~2 个数量级 |
| 单生产者单消费者队列 | `WRITE_ONCE`/`READ_ONCE` + release/acquire 就够，**不需要原子 RMW** |
| 多生产者 | 用 `atomic_fetch_add` 一次抢占槽位，再写数据（ticket 模式） |
| 生命周期管理 | **无脑用 `refcount_t`**，别用 `atomic_t` |
| 就绪标志 | `atomic_set` + `smp_store_release` / `smp_load_acquire`，或 `atomic_xchg` |
| 争用诊断 | `perf c2c` 找 HITM 热点；`perf stat -e mem_inst_retired.lock_loads` 看 lock 指令密度 |

```c
/* HFT 里最常见的错误优化：把原子当便宜货用 */
atomic_t packets;                       /* ❌ 所有核都在打这个计数器 */
atomic_inc(&packets);

/* 正确做法：per-CPU 累加，读的时候才汇总 */
DEFINE_PER_CPU(unsigned long, packets); /* ✅ 无争用，纯本地 inc */

static inline void count_packet(void)
{
	this_cpu_inc(packets);              /* 编译成单条 inc，无 lock 前缀 */
}

static unsigned long total_packets(void)
{
	unsigned long total = 0;
	int cpu;
	for_each_possible_cpu(cpu)
		total += per_cpu(packets, cpu); /* 读路径才付出代价，且可低频调用 */
	return total;
}
```

> 这与 12.10（per-CPU）是同一个主题的两面：**先消除共享，再谈原子**。
> 原子操作的代价不在指令本身，而在 **cache line 的跨核迁移**。

---

→ [10.2 自旋锁](./section-10.2-自旋锁.md) · [10.10 屏障](./section-10.10-排序和屏障.md) · [02-CSAPP 并发](../../../02-computer-systems/chapter-12-concurrent-programming/) · [12.10 每个 CPU 的分配](../../chapter-12-memory-management/notes/section-12.10-每个-CPU-的分配.md)

> ↔ [19.3 特定数据类型（refcount_t 分家、内核 char 无符号）](../../chapter-19-portability/notes/section-19.3-特定数据类型.md) · [19.7 处理器排序](../../chapter-19-portability/notes/section-19.7-处理器排序.md)

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `atomic_t` 和 `refcount_t` 的区别？

<details><summary>答案</summary>

`atomic_t`：纯原子计数器，`atomic_dec(&v)` 可以从 0 变成 -1（UAF 漏洞）。`refcount_t`：引用计数专用，`refcount_dec()` 在 0 时 WARN + 阻止下溢。6.x 内核中 `task_struct` 的 usage 已从 `atomic_t` 改为 `refcount_t`。安全代码应始终用 `refcount_t` 管理生命周期。

> **⚠️ 按 v6.6 修订/补充**
>
> 1. **"`refcount_dec()` 阻止下溢"这个说法不准确**。看真实实现
>    （`include/linux/refcount.h:336`）：
>    ```c
>    int old = atomic_fetch_sub_release(1, &r->refs);
>    if (unlikely(old <= 1))
>            refcount_warn_saturate(r, REFCOUNT_DEC_LEAK);
>    ```
>    它是**先减后检查**，用的是 `atomic_fetch_sub_release`——**没有 cmpxchg 循环，也没有"阻止"**。
>    真正的保护是**饱和机制**：一旦进入饱和区（值 ≈ `REFCOUNT_SATURATED = 0xC0000000`），
>    值不再有意义，后续 `refcount_dec_and_test` 不会返回 true，于是**对象永不释放**。
>    → 故障模式从 **UAF（可被利用）** 降级为 **WARN + 内存泄漏**。
>
> 2. **为什么故意不"阻止"**：注释写得很清楚 ——
>    "In order to avoid the cost associated with introducing cmpxchg() loops into all of
>    the saturating operations"。引用计数是极热路径，加 cmpxchg 循环的代价不可接受。
>    内核选择接受一个**理论存在但实践中不可利用**的竞态窗口，
>    并通过把饱和值放在 `0xC0000000`（距 0 与 INT_MAX 大致等距）把逃逸难度拉到最大。
>    注释给出的算术：逃逸需要连续命中约 **256 次**竞态（`(UINT_MAX+1-0xC0000000)/PID_MAX_LIMIT`）。
>
> 3. **五种饱和类型**（不只是"下溢"一种）：
>    `REFCOUNT_ADD_NOT_ZERO_OVF` / `REFCOUNT_ADD_OVF` / `REFCOUNT_ADD_UAF` /
>    `REFCOUNT_SUB_UAF` / `REFCOUNT_DEC_LEAK`。
>
> 4. **`refcount_dec_and_test()` 的内存序**：release（减之前的操作不被重排到后面），
>    **成功时额外 acquire**（保证 `free()` 在所有先前访问之后）。
>    这是引用计数语义的关键，单纯用 `atomic_dec_and_test` 模拟需要自己加屏障。
>
> 5. **`refcount_t` 是 v4.11 引入的**（不是"6.x 才有"），
>    直接动因是 2016~2017 那一波 refcount 溢出 → UAF 的 CVE 潮。

</details>

**Q2.** `atomic_inc(&v)` 在 x86-64 上实际生成什么指令？

<details><summary>答案</summary>

`lock incl (%rdi)`——LOCK 前缀 + incl 指令。LOCK 前缀锁 cache line（通过 MESI 协议的 Read-Modify-Write 周期），确保原子性。开销：~20-40 cycles（无争用时）。争用时 cache line bouncing，可达数百 cycles。ARM64 上生成 `ldaxr`/`stlxr`（独占加载/存储）循环。

> **⚠️ 按 v6.6 修订/补充**
>
> **指令部分基本正确**，但有几个点要修正和补充：
>
> 1. **指令是源码里写死的**，不是编译器选的（`arch/x86/include/asm/atomic.h:51`）：
>    ```c
>    static __always_inline void arch_atomic_inc(atomic_t *v)
>    {
>            asm volatile(LOCK_PREFIX "incl %0"
>                         : "+m" (v->counter) :: "memory");
>    }
>    ```
>    而 `atomic_add` 用的是 `LOCK_PREFIX "addl"`（`:31`）——**两个不同的助记符**。
>
> 2. **`LOCK_PREFIX` 在 UP 机器上会被运行时改成 nop**（`alternative.h:47`）：
>    SMP 下是 `LOCK_PREFIX_HERE "\n\tlock; "`，UP 下是空字符串。
>    而且 `LOCK_PREFIX_HERE` 把每条 lock 指令的地址记进 `.smp_locks` 段，
>    启动时可就地打补丁为 nop。所以 **UP 配置下原子操作零额外开销**。
>
> 3. **"~20-40 cycles"这个数字要看作量级，且区分三种情形**：
>    - 无争用、line 已在本核 L1：十几到几十 cycle
>    - 无争用、line 在别的核：+ 一次跨核传输
>    - **有争用**：数百 cycle 起，且随争用核数**非线性**恶化；跨 NUMA 再翻一倍量级
>
>    真正的瓶颈不是 `lock` 指令本身，而是**必须拿到 cache line 的独占权（MESI 的 M 态）**。
>    → 别背数字，用 `perf c2c` 量：
>    ```bash
>    perf c2c record -a -- ./your_app && perf c2c report --stdio   # 找 HITM 热点
>    ```
>
> 4. **ARM64 那条要更新**：现代 ARM64（**ARMv8.1+ LSE**）用的是**单条指令**
>    `stadd` / `ldadd` / `swp` / `cas`，**不是** LL/SC 循环。
>    `ldaxr`/`stlxr` 循环是**没有 LSE 时的回退路径**。
>    v6.6 的 ARM64 默认开 `CONFIG_ARM64_LSE_ATOMICS`。
>
> 5. **补充一个反直觉的点**：`atomic_read()` **不生成任何 lock 指令**——
>    它就是 `__READ_ONCE((v)->counter)`，一条普通 `mov`。

</details>

**Q3.** HFT 用户态如何高效使用原子操作？

<details><summary>答案</summary>

```c
// 无锁 SPSC 队列
std::atomic<size_t> head{0}, tail{0};
// 生产者
head.store(head.load(std::memory_order_relaxed) + 1,
           std::memory_order_release);
// 消费者
size_t h = head.load(std::memory_order_acquire);
if (h > tail.load(std::memory_order_relaxed)) {
    // 有数据
}
// 关键: release/acquire 配对, 避免 seq_cst 的全屏障开销
```

> **⚠️ 按 v6.6 / 现代 C++ 修订**
>
> 1. **这段代码有个真 bug**：`head.load(relaxed) + 1` 再 `store(release)`。
>    在**单生产者**前提下它碰巧正确（没有别人改 head），但这是**侥幸**不是设计。
>    一旦有两个生产者就是丢更新。SPSC 用 `store(release)` 是对的，
>    但**任何"读-改-写"都应该显式用 `fetch_add`**：
>    ```c
>    head.fetch_add(1, std::memory_order_release);   // 原子 RMW，意图明确
>    ```
>    在 x86 上 `fetch_add(release)` 与 `store(release)` 生成**完全一样的指令**
>    （`lock xadd` vs `mov` + `mfence`… 实际前者更优），所以**没有性能理由用手写版本**。
>
> 2. **SPSC 场景下连 release/acquire 都可能过量**。
>    如果消费者用 `acquire` 读 head，再用 `relaxed` 读数据，是错的——
>    **acquire 必须配对**：生产者写数据 → `release` store head；
>    消费者 `acquire` load head → 才能安全读数据。
>    ```c
>    // 生产者
>    buf[slot] = data;                                  // 1. 先写数据
>    head.store(i + 1, std::memory_order_release);      // 2. release 发布
>
>    // 消费者
>    size_t h = head.load(std::memory_order_acquire);   // 3. acquire 获取
>    if (h > t) consume(buf[t]);                        // 4. 现在读数据安全
>    ```
>
> 3. **x86 上 release store 和 acquire load 是零开销的**（编译成普通 `mov`），
>    因为 x86 的 TSO 已经保证了 load-load / store-store 不重排。
>    省掉的开销是 **`seq_cst`**：它在 x86 上退化成 `xchg`（隐含 lock）或 `mfence`。
>    所以"避免 seq_cst"这条建议**在 x86 上完全成立**，而且收益不小。
>
> 4. **内核态的对应物是 `smp_store_release` / `smp_load_acquire`**，
>    以及原子操作的 `_release` / `_acquire` 后缀变体（见本篇第 3 节）。
>    ⚠️ 但注意：**Linux 的 `_acquire`/`_release` 语义与 C11 略有差异** ——
>    Linux 的"无后缀 = full barrier"比 C11 的 `seq_cst` 更强。

</details>

**Q4.** v6.6 里 `atomic_inc(&v)` 的完整调用链是什么？为什么要有三层？

<details><summary>答案</summary>

```
atomic_inc(&v)                                    ← atomic-instrumented.h
  └─ arch_atomic_inc(&v)                          ← 架构实现或通用 fallback
       └─ asm volatile(LOCK_PREFIX "incl %0")     ← arch/x86/include/asm/atomic.h:51
```

三层分别是：

| 层 | 头文件 | 命名 | 职责 |
|----|--------|------|------|
| 架构实现 | `arch/x86/include/asm/atomic.h` | `arch_atomic_*` | 内联汇编 |
| 通用 fallback | `linux/atomic/atomic-arch-fallback.h` | `arch_atomic_*` | 架构没实现的，用 cmpxchg 等原语拼出来 |
| 插桩包装 | `linux/atomic/atomic-instrumented.h` | `atomic_*` | 套上 KASAN / KCSAN 检查 |

顶层 `include/linux/atomic.h` 的末尾按序包含这三个：

```c
#include <linux/atomic/atomic-arch-fallback.h>
#include <linux/atomic/atomic-long.h>
#include <linux/atomic/atomic-instrumented.h>
```

**为什么这么拆**（2019~2020 年 Peter Zijlstra 的重构）：
1. 以前每个架构要把几百个 atomic 操作全实现一遍，漏一个就静默出错
2. 现在架构**只实现自己有硬件指令支持的**，其余自动由 fallback 层拼
3. 插桩（KCSAN 数据竞争检测、KASAN 越界检测）**只需写一层**，所有架构自动受益

**版本断崖**：多版本 diff 定位在 **v5.8**（v5.7 的 `atomic.h` 里没有这两个 include，
v5.8 起有）。

**实践推论**：写驱动用的 `atomic_*` 是**带插桩那层**。
极热路径想省插桩开销可用 `raw_atomic_*`，代价是丢掉 KCSAN 保护。

</details>

**Q5.** 哪些 atomic 操作不带内存屏障？在 x86 上用 `_relaxed` 能省多少？

<details><summary>答案</summary>

**不提供屏障的**：
- `atomic_read()` —— 就是 `READ_ONCE`，普通读
- `atomic_set()` —— 就是 `WRITE_ONCE`，普通写
- 所有带 `_relaxed` 后缀的变体

**提供 full barrier 的**（无后缀的 RMW）：
`atomic_inc` / `atomic_add` / `atomic_xchg` / `atomic_cmpxchg` /
`atomic_add_return` / `atomic_fetch_*` / `atomic_*_and_test` 等。

四档规则（`include/linux/atomic.h:11-24` 注释）：
无后缀 = full ordered；`_acquire`；`_release`；`_relaxed`。

**⚠️ x86 上的关键推论：`_relaxed` 省不下屏障。**

```c
/* arch/x86/include/asm/barrier.h:79-80 */
#define __smp_mb__before_atomic()	do { } while (0)
#define __smp_mb__after_atomic()	do { } while (0)
```

这两个"屏障"在 x86 上是**空操作**，因为 x86 的 `lock` 指令**本身已经是 full barrier**。
而 `_relaxed` 版的 RMW **照样要发 `lock` 指令**（否则就没有原子性了）。

所以：

| 操作 | x86 上是否省开销 |
|------|------------------|
| `atomic_add` → `atomic_add_relaxed` | ❌ **不省**（都要 lock 指令） |
| `atomic_read` 本身 | 已经是普通 mov，无从再省 |
| 用 `_acquire`/`_release` 代替无后缀 | ❌ 不省（x86 上无额外屏障可加） |
| **把 RMW 换成 `READ_ONCE`+`WRITE_ONCE`** | ✅ **省很多**（去掉 lock 指令） |
| **消除共享（per-CPU）** | ✅ **省最多**（去掉跨核迁移） |

> **结论**：在 x86 上，原子操作的优化方向**不是调内存序后缀**，而是
> **① 消除共享（per-CPU / 分片）② 把 RMW 降级为普通读写（单生产者场景）**。
> `_relaxed` 的真实价值在于：跨架构可移植性、表达设计意图、让 KCSAN 更准确。

</details>

**Q6.** 为什么 `refcount_t` 的饱和值选 `0xC0000000`（`INT_MIN/2`）？

<details><summary>答案</summary>

因为要让它**距 0 和 INT_MAX 的距离大致相等**，从而把"逃逸出饱和区"的难度最大化。

源码里的图（`include/linux/refcount.h:21-25`）：

```
                           INT_MAX     REFCOUNT_SATURATED   UINT_MAX
0                        (0x7fff_ffff)    (0xc000_0000)    (0xffff_ffff)
+--------------------------------+----------------+----------------+
                                  <---------- bad value! ---------->
```

**背景**：refcount 的饱和修复**是 racy 的**——
"检测到下溢"和"设为饱和值"之间，别的线程可能又改了值。
内核为了性能**故意不引入 cmpxchg 循环**，于是接受了这段窗口。

**为什么选在这个位置**：
- 从 0 往下减（下溢），要先跌到 `0xC0000000` 才算进入饱和区 —— 距离 `0x40000000`（约 10 亿）
- 从 INT_MAX 往上加（溢出），也要先涨到 `0xC0000000` —— 距离 `0x40000000`

两头距离相等，所以**无论从哪边逃逸，难度都一样大**（不能靠选一边来削弱）。

**量化论证**（注释原文）：

```
(UINT_MAX+1-REFCOUNT_SATURATED) / PID_MAX_LIMIT =
0x40000000 / 0x400000 = 0x100 = 256
```

即从饱和值回到正常值，需要连续约 **256 次**成功竞态命中。
考虑到需要精确控制调度时序并连续命中，注释的结论是
"there doesn't appear to be a practical avenue of attack"。

**一句话总结这个设计**：内核用"把故障模式从 UAF 降级为内存泄漏"换掉了
"每个引用计数操作都加 cmpxchg 循环"的性能代价 —— 这是内核里
**安全/性能取舍**最清晰的一个范例。

</details>

</details>
---
