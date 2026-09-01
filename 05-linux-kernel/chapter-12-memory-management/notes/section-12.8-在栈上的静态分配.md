## ⑧ 在栈上的静态分配

内核 **同样用 C 栈** 放局部变量，但栈 **极小、不可增长、溢出即灾难** — 规则比用户态 **硬得多**。

> **版本前提**：本节数值基于 **v6.6 源码实证**
> （`arch/x86/include/asm/page_64_types.h`、`arch/arm64/include/asm/memory.h`、
> `arch/Kconfig`、`lib/Kconfig.debug`、`kernel/trace/Kconfig`、`scripts/checkstack.pl`）。
> ⚠️ 书上"8KB 两页"的说法在 **x86_64/arm64 上已经不对了**（16KB），
> 且"溢出会破坏 `thread_info`"在启用了 `THREAD_INFO_IN_TASK` 的现代配置下也**不成立**。

#### 内核栈大小（v6.6 实证）

```c
/* arch/x86/include/asm/page_64_types.h:8-22 */
#ifdef CONFIG_KASAN
#define KASAN_STACK_ORDER 1
#else
#define KASAN_STACK_ORDER 0
#endif

#define THREAD_SIZE_ORDER	(2 + KASAN_STACK_ORDER)
#define THREAD_SIZE		(PAGE_SIZE << THREAD_SIZE_ORDER)

#define EXCEPTION_STACK_ORDER	(1 + KASAN_STACK_ORDER)
#define EXCEPTION_STKSZ	(PAGE_SIZE << EXCEPTION_STACK_ORDER)

#define IRQ_STACK_ORDER		(2 + KASAN_STACK_ORDER)
#define IRQ_STACK_SIZE		(PAGE_SIZE << IRQ_STACK_ORDER)
```

```c
/* arch/arm64/include/asm/memory.h:77-112 */
#define MIN_THREAD_SHIFT	(14 + KASAN_THREAD_SHIFT)   /* KASAN_THREAD_SHIFT = 1 with KASAN */
#if defined(CONFIG_VMAP_STACK) && (MIN_THREAD_SHIFT < PAGE_SHIFT)
#define THREAD_SHIFT		PAGE_SHIFT
#else
#define THREAD_SHIFT		MIN_THREAD_SHIFT
#endif
#define THREAD_SIZE		(UL(1) << THREAD_SHIFT)

#define IRQ_STACK_SIZE		THREAD_SIZE
#define OVERFLOW_STACK_SIZE	SZ_4K
```

| 配置 | 内核栈大小 | 依据 |
|------|-----------|------|
| **x86_64（4KB 页）** | **16 KB**（order 2） | `page_64_types.h:16` |
| **x86_64 + KASAN** | **32 KB**（order 3） | `KASAN_STACK_ORDER = 1` |
| **arm64（4KB 页）** | **16 KB**（shift 14） | `memory.h:83` |
| **arm64 + KASAN** | **32 KB**（shift 15） | `KASAN_THREAD_SHIFT = 1` |
| **传统 x86 32 位** | 8 KB（两页） | 书上说的年代 |
| **x86_64 中断栈** | **16 KB**（`IRQ_STACK_SIZE`，独立分配） | `page_64_types.h:22` |
| **x86_64 异常栈** | **8 KB**（`EXCEPTION_STKSZ`，NMI/DF/MCE 等） | `page_64_types.h:19` |
| **arm64 溢出栈** | **4 KB**（`OVERFLOW_STACK_SIZE`，专供栈溢出后打印） | `memory.h:114` |

#### ⚠️ 三个栈是分开的（修正"中断嵌套时同一栈"的说法）

```
进程内核栈（thread stack，16KB）
    ├── 系统调用 / 内核线程的函数调用链
    │
硬中断栈（x86_64：16KB，独立）        ← 硬中断**不**用进程栈
    ├── ISR 函数链
    │
异常栈（x86_64：8KB × N）             ← NMI / Double Fault / MCE 各有独立栈
    ├── 连"栈本身坏了"的情况都能处理

软中断：CONFIG_SOFTIRQ_ON_OWN_STACK = HAVE_SOFTIRQ_ON_OWN_STACK && !PREEMPT_RT
        （arch/Kconfig:986）—— RT 上软中断线程化，跑在自己的线程栈上
```

> **实践含义**：硬中断**不再**叠加到进程栈上（这是 x86_64 的做法），
> 所以"深调用链 + 来个中断就爆栈"的风险比 32 位年代**小很多**。
> 但**软中断/下半部**在部分架构上仍跑在被中断的栈上（或 RT 上跑线程栈），
> 且 **NMI 可以打断正在处理 NMI 的路径**——所以"栈要留余量"这条纪律没有过时。

#### 溢出保护：VMAP_STACK（默认 y）与 guard page

```c
/* arch/Kconfig:1237 */
config VMAP_STACK
	default y
	bool "Use a virtually-mapped stack"
	depends on HAVE_ARCH_VMAP_STACK
	depends on !KASAN || KASAN_HW_TAGS || KASAN_VMALLOC
	help
	  Enable this if you want the use virtually-mapped kernel stacks
	  with guard pages.  This causes kernel stack overflows to be
	  caught immediately rather than causing difficult-to-diagnose
	  corruption.
```

```
非 VMAP_STACK：                        VMAP_STACK（v6.6 默认）：
  [ task_struct 附近 ]                   vmalloc 区里一段虚拟地址
  [  内核栈 16KB    ]                    [ guard page ── 未映射 ]
  [ STACK_END_MAGIC ]                    [  内核栈 16KB        ]
       ↓ 溢出                            [ guard page ── 未映射 ]
  静默踩坏邻居（可能毫无症状）                 ↓ 溢出
                                       立即触发 page fault → 可诊断
```

arm64 还加了一层**廉价检测**（`memory.h:101-110` 注释逐字）：

```
By aligning VMAP'd stacks to 2 * THREAD_SIZE, we can detect overflow by
checking sp & (1 << THREAD_SHIFT), which we can do cheaply in the entry
assembly.
```

> 即：**把 vmap 栈按 2×栈大小对齐**，于是"判断当前 sp 是不是溢出到相邻区"
> 退化成一次 **按位与**——可以在**异常处理入口的汇编里**直接查。

#### `thread_info` 已经不在栈底了（修正书上的说法）

```c
/* include/linux/thread_info.h:17 —— v6.6 仍在，但 x86_64/arm64 都会选上 */
#ifdef CONFIG_THREAD_INFO_IN_TASK
#define current_thread_info() ((struct thread_info *)current)
#endif
```

> 书上（以及本笔记原稿）说"栈溢出会覆盖 `thread_info`"——
> 那是 `thread_info` 还放在**栈底**的年代。现代配置下
> **`thread_info` 是 `task_struct` 的第一个成员**（`current_thread_info()` 就是 `current`），
> 栈溢出**不会**直接破坏它。
> 溢出破坏的是栈**上方**（低地址）邻接着的东西——
> 在 `VMAP_STACK=y` 时是 **guard page**，所以表现为 **fault**；
> 在 `VMAP_STACK=n` 时才是静默踩内存。

#### 禁止模式

| 错误 | 后果 |
|------|------|
| **`char buf[65536]` 局部数组** | **栈溢出** — 触发 guard page fault（`VMAP_STACK`）或静默破坏 |
| **大 `struct` 值拷贝进栈** | 同样危险（且拷贝本身有开销） |
| **无限递归 / 不受控深递归** | 瞬间耗尽 |
| **`alloca` / VLA（变长数组）** | 等价于栈上大数组，且**大小运行时才确定** → 编译期检查完全失效（内核已全面清除 VLA 用法） |
| **深调用链 + 每层几百字节** | 单帧不超限，但**累计**会超。这是最隐蔽的一种 |

#### 正确替代

| 需求 | 做法 |
|------|------|
| **几 KB 临时缓冲** | **`kmalloc(..., GFP_KERNEL)`** — 进程上下文（见 [12.5](./section-12.5-kmalloc-与-kfree.md)） |
| **中断里小缓冲** | **静态 per-CPU 缓冲** 或 **预分配 pool**（见 [12.10](./section-12.10-每个-CPU-的分配.md)） |
| **固定类型高频** | **`kmem_cache_alloc`** |
| **编译期常量表** | **`static const`** 放 **.rodata** |
| **大块、只要求虚拟连续** | **`vmalloc`**（见 [12.6](./section-12.6-vmalloc.md)，注意不能用于中断/DMA） |

```c
/* 坏 */
void bad(void) {
    char tmp[8192];  /* 单帧就吃掉 16KB 栈的一半，还只是这一个函数 */
}

/* 好 */
void good(void) {
    char *tmp = kmalloc(8192, GFP_KERNEL);
    if (!tmp) return;
    /* ... */
    kfree(tmp);
}

/* 更好（热路径）：per-CPU 预分配，零分配零释放 */
DEFINE_PER_CPU(unsigned char[512], tmp_buf);
void hot(void) {
    unsigned char *tmp = this_cpu_ptr(tmp_buf);   /* 只有 512 字节，栈上一个指针 */
}
```

#### 怎么量：栈使用量的检测工具箱

| 手段 | 粒度 | 怎么用 |
|------|------|--------|
| **`CONFIG_FRAME_WARN`**（默认 **2048** @64bit / **1024** @32bit，`lib/Kconfig.debug:434`） | **单帧** | 编译期警告："stack frame larger than N bytes"。**只管单个函数**，管不了调用链累计 |
| **`scripts/checkstack.pl`** | **单帧** | `objdump -d vmlinux \| scripts/checkstack.pl [arch]` —— 列出栈用量最大的函数（v6.6 仓库内确有其脚本） |
| **`CONFIG_DEBUG_STACK_USAGE`** | **累计（历史最低水位）** | 记录每个任务**曾经**剩下的最少栈量，在 **sysrq-T / sysrq-P** 输出里显示。代价：创建进程变慢 |
| **`CONFIG_SCHED_STACK_END_CHECK`**（默认 **n**） | 溢出检测 | 在 `schedule()` 时检查栈底的 magic 是否被改写，被改就 **panic**（"corrupted region can no longer be trusted"） |
| **`CONFIG_STACK_TRACER`** | **累计（全系统峰值）** | ftrace 的栈追踪器，**挂钩每个函数调用**记录最大栈占用，结果看 **`/sys/kernel/tracing/stack_trace`** |
| **`/proc/<pid>/stack`** | 瞬时 | 看某个任务当前的**内核栈回溯**（需 `CONFIG_STACKTRACE`） |

```
分层理解（判据）：
  单帧 > FRAME_WARN(2048)        → 编译期就能抓（FRAME_WARN / checkstack.pl）
  单帧不超但调用链累计超          → 只能靠运行时（DEBUG_STACK_USAGE / STACK_TRACER）
  真溢出了                       → VMAP_STACK 的 guard page 立刻 fault（最好情况）
```

#### 与用户栈对比

| | 用户栈 | 内核栈 |
|--|--------|--------|
| 默认大小 | **~8MB**（可调 `ulimit`） | **16KB 量级**（x86_64/arm64） |
| 溢出 | SIGSEGV（常） | **guard page fault** 或 **破坏内核** — 难查 |
| 可增长 | ✅ 缺页异常里自动扩 | ❌ 内核自己的缺页处理也要用栈，无法自举 |
| 大数组 | 仍不推荐 | **绝对禁止** |

**HFT：** 用户态 **策略栈** 也不放大数组 — **`thread_local` ring + mmap** 放堆/映射区。内核 **NAPI** 处理函数 **栈帧要浅** — 深调用链 + 局部变量 = **隐性 latency**（cache miss + 栈 touch）。

> **HFT 补充（栈为什么影响延迟）**：
> ① **栈的"冷启动"成本**：内核栈是 **per-task** 的，每次上下文切换到新任务，
> 栈顶附近几乎肯定 **不在 L1/L2** —— 深的调用链意味着**每层都要触碰新的栈页**（cache miss）。
> 浅调用链 + 紧凑栈帧 = 更少的 cache miss；
> ② **`CONFIG_FRAME_WARN` 是免费的静态检查**：把大缓冲挪出栈，
> 同时也就顺手消除了上面这笔 cache 成本，一举两得；
> ③ **绑核 + 单线程模型下，栈是"热"的**：同一个任务反复在同一核上跑，
> 栈顶区域稳定留在 L1——这也是 HFT 里"每核一个线程、避免迁移"的收益之一
> （与 [12.10 per-CPU](./section-12.10-每个-CPU-的分配.md) 是同一个道理）；
> ④ **量化方法**：开 `CONFIG_STACK_TRACER` 跑真实负载，读
> `/sys/kernel/tracing/stack_trace` 看**峰值调用链**；
> 用 `CONFIG_DEBUG_STACK_USAGE` + **sysrq-T** 看**每个任务的历史最低水位**，
> 比拍脑袋安全得多。

→ Ch 2 内核栈 · [Ch 7 中断栈](../../chapter-07-interrupts) · [Ch 12.5 kmalloc](./section-12.5-kmalloc-与-kfree.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核栈有多大？为什么不能递归调用？

<details><summary>答案</summary>

x86_64 内核栈通常 8KB（或 16KB with CONFIG_THREAD_INFO_IN_TASK）。8KB 栈意味着函数调用链不能太深、不能有大的局部数组。递归会迅速耗尽栈 → stack overflow → oops/panic。内核代码规则：避免递归、局部数组 < 1KB、大缓冲用 kmalloc。

> **按 v6.6 修订**：现在的数字是**确定的 16KB，不是 8KB**：
> - x86_64：`THREAD_SIZE_ORDER = 2 + KASAN_STACK_ORDER` → `THREAD_SIZE = PAGE_SIZE << 2` = **16KB**（开 KASAN 时 order 3 = 32KB）；
> - arm64：`MIN_THREAD_SHIFT = 14 + KASAN_THREAD_SHIFT` → **16KB**（开 KASAN 时 32KB）。
>
> 另外 8KB 是**32 位 x86** 的年代。而且"或 16KB with CONFIG_THREAD_INFO_IN_TASK"这个条件说反了——
> `THREAD_INFO_IN_TASK` 管的是 **`thread_info` 放在哪里**（`task_struct` 内 vs 栈底），
> **和栈大小无关**。它带来的是另一条修正：**栈溢出不再会直接破坏 `thread_info`**。

</details>

**Q2.** 为什么内核栈不能自动增长？

<details><summary>答案</summary>

用户态栈可以自动扩展（page fault handler 检测到栈生长 → 分配新页）。内核态没有这个机制：page fault handler 本身也用内核栈，如果栈溢出时再触发 page fault 会无限递归。所以内核栈溢出直接 oops。8KB 是硬限制，开发者必须小心。

> **按 v6.6 补充：溢出后的表现取决于 `CONFIG_VMAP_STACK`。**
> 这个选项在 v6.6 是 **default y**（依赖 `HAVE_ARCH_VMAP_STACK`），
> 它的帮助文本直说目的是 **"causes kernel stack overflows to be caught immediately rather than
> causing difficult-to-diagnose corruption"**：
> 栈被分配在 **vmalloc 区**并**两侧带 guard page**，
> 所以溢出**立刻触发一次 page fault**（可诊断），而不是静默踩坏邻近内存。
>
> arm64 还额外把 vmap 栈 **按 2×THREAD_SIZE 对齐**，
> 于是"是否溢出"可以用 **`sp & (1 << THREAD_SHIFT)`** 一次按位与判断——
> 在**异常处理入口的汇编里**就能查（`memory.h:101` 注释）。
>
> 关掉 VMAP_STACK 时，兜底的是 `CONFIG_SCHED_STACK_END_CHECK`（**默认 n**）：
> 在 `schedule()` 里检查栈底 magic，**被改写就 panic**——
> 注意这是"事后检测"（等调度到才发现），不如 guard page 即时。

</details>

**Q3.** `CONFIG_FRAME_WARN` 报了警告，是不是就说明我的调用链一定不会爆栈？

<details><summary>答案</summary>

**不是——它只看"单个函数的栈帧"，不看调用链的累计值。**

`lib/Kconfig.debug:434` 的定义是 `int "Warn for stack frames larger than"`，
默认 **2048（64 位）/ 1024（32 位）**（KASAN 32 位 1280、PARISC 2048 等有特例）。
它传给编译器的是 `-Wframe-larger-than=`，所以：

| 情况 | FRAME_WARN 能抓吗 |
|------|------------------|
| 单个函数里 `char buf[4096]` | ✅ 能抓（编译期） |
| 20 个函数各用 800 字节，累计 16KB | ❌ **抓不到** |
| 递归深度取决于运行时输入 | ❌ 抓不到 |

要量"累计"必须上运行时手段：
- **`CONFIG_STACK_TRACER`**：ftrace 栈追踪器，挂钩每个函数调用记录**全系统最大栈占用**，
  结果在 **`/sys/kernel/tracing/stack_trace`**；
- **`CONFIG_DEBUG_STACK_USAGE`**：记录**每个任务**历史最低剩余栈，通过 **sysrq-T / sysrq-P** 输出；
- **`scripts/checkstack.pl`**：对 `objdump -d vmlinux` 做静态分析，列出栈帧最大的函数（仍是单帧粒度，但能全量排序）。

</details>

**Q4.** 硬中断处理函数用的栈，和系统调用路径用的栈是同一块吗？

<details><summary>答案</summary>

**在 x86_64 上不是**——这是本笔记原稿"中断嵌套时同一栈"需要更新的地方。

v6.6 实证（`arch/x86/include/asm/page_64_types.h`）：
- **`IRQ_STACK_SIZE = PAGE_SIZE << (2 + KASAN_STACK_ORDER)` = 16KB**，硬中断跑在**独立的中断栈**上；
- **`EXCEPTION_STKSZ = PAGE_SIZE << (1 + KASAN_STACK_ORDER)` = 8KB**，
  **NMI / Double Fault / Machine Check** 各有**专属异常栈**（连"栈本身坏了"都能继续处理）；
- 进程自己的 **`THREAD_SIZE` = 16KB** 栈只承载系统调用与内核线程的调用链。

软中断的位置由 `arch/Kconfig:986` 决定：
**`config SOFTIRQ_ON_OWN_STACK` = `HAVE_SOFTIRQ_ON_OWN_STACK && !PREEMPT_RT`**
——非 RT 上可以有独立的软中断栈，而 **RT 上软中断被线程化，跑在各自的线程栈上**。

> 结论：硬中断**不再**叠加到进程栈上，所以"深调用链 + 来个中断就爆栈"的风险比 32 位年代小得多。
> 但 NMI 可以打断 NMI 处理、软中断/下半部在部分架构上仍用被中断的栈，
> 所以**留栈余量**这条纪律依然成立。

</details>

**Q5.** 我在中断处理里需要一个 2KB 的临时缓冲，有哪些做法？各自代价是什么？

<details><summary>答案</summary>

| 做法 | 可行？ | 代价 / 风险 |
|------|--------|------------|
| **局部数组 `char buf[2048]`** | ⚠️ 编译能过 | 单帧就吃掉 16KB 栈的 1/8，且**超过 `FRAME_WARN`(2048) 的边界**；若 ISR 里还调用其他函数，累计风险陡增。**不推荐** |
| **中断里 `kmalloc(..., GFP_ATOMIC)`** | ✅ 能跑 | **会失败**（必须判 NULL），且 `GFP_ATOMIC` 走紧急储备；**热路径上每次分配都是尾延迟来源**（见 [12.5](./section-12.5-kmalloc-与-kfree.md)）。**不推荐用于每包路径** |
| **per-CPU 静态缓冲** | ✅ **推荐** | 零分配、零释放、本 CPU 独占无锁。注意两点：① 访问期间**不能被抢占到别的核**（用 `this_cpu_ptr()` 或 `preempt_disable()`）；② 若中断可能**嵌套**进来（如 NMI 打断普通中断），需要**双份缓冲**或禁中断 |
| **预分配 pool + 无锁 ring** | ✅ 推荐（大数据块） | 启动时分配好，运行时只取/还。最贴近 HFT 的做法 |
| **`kmem_cache_alloc`** | ✅ 固定类型时 | 比 `kfree()` 路径短（少一次 folio 反查），见 [12.7](./section-12.7-Slab-层.md) |

> 判据一句话：**中断里不要分配内存，要"预先分配好、运行时零分配"**。
> 需要多大的缓冲，就把它变成**静态/per-CPU 的容量**，而不是运行时的分配请求。

</details>

</details>
---
