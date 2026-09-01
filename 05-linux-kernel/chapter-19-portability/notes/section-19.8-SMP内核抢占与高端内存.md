## ⑧ SMP、内核抢占与高端内存

可移植 ≠ 只换 CPU — 还要兼容 **内核配置**。

同一份驱动源码，会被编译进**上千种配置组合**的内核：单核路由器、128 核服务器、跑 RT 的工控机、32 位老 ARM。
LKD 这一节的核心主张只有一句：

> **原则：按「最坏情况」写** — 单核 UP、关抢占、无 HIGHMEM 的「侥幸」代码 **迟早炸**。

| 配置维度 | 编写时必须做的假设 | 违反后症状 |
|----------|-------------------|-----------|
| **SMP** | **始终**可能真并发 → **锁 / per-CPU / READ_ONCE** | 只在多核上偶发的数据损坏 |
| **内核抢占** | 临界区随时可能被插 → **短临界区**，`preempt_disable()` 仅当真有理由 | 只在抢占内核上出现的竞态 |
| **HIGHMEM** | 页可能**没有**内核线性映射 → 用 **kmap 家族**取临时映射 | 只在 32 位大内存机器上崩溃 |

**三个维度的共同点**：出错的都是"在我的机器上跑得好好的"——
因为它们全是**配置相关**的，而你的开发机只有一种配置。

---

## 一、SMP：把"可能并发"当既定事实

### 1.1 `CONFIG_SMP` 到底改变了什么

不是"多了几个 CPU"这么简单，它会在**编译期**改掉一批基础设施的语义：

| 机制 | `CONFIG_SMP=y` | `CONFIG_SMP=n` | 出处 |
|------|----------------|----------------|------|
| `smp_mb()` | 真屏障指令（`mfence` / `dmb ish`） | **退化为 `barrier()`**（只剩编译器屏障） | `include/asm-generic/barrier.h:99 / :113` |
| `smp_processor_id()` | 读 per-CPU 的 cpu 号 | 常量 0 | — |
| per-CPU 变量 | 每 CPU 一份副本 | 只有一份 | — |
| `spinlock` | 真自旋 | 退化（UP 上不可能有第二个持有者） | — |

> 这就是 [19.7 处理器排序](./section-19.7-处理器排序.md) 里 `smp_mb()` 在 UP 上"消失"的同一条逻辑：
> **内核用编译期开关为不存在的场景免单**。你写驱动时不需要关心，但你**依赖**它们的语义。

### 1.2 三条写法规律

| 场景 | 正确写法 | 错误写法 |
|------|---------|---------|
| 共享计数器 | `this_cpu_inc()` / per-CPU 聚合 | `counter++`（撕裂 + 缓存行乒乓） |
| 共享链表 | `spin_lock` / `rcu` + `list_for_each_entry_rcu()` | 裸 `list_add` |
| 只被读一次的指针/标志 | `READ_ONCE()` / `WRITE_ONCE()` | 直接读写（可能被编译器优化成重复读/合并写） |

### 1.3 让内核替你验：三把"配置无关的尺子"

SMP 类 bug 之所以难，是因为**跑不出来**。内核提供了三个开关，把概率性竞态变成必现告警：

| 开关 | 抓什么 | 代价 |
|------|--------|------|
| `CONFIG_PROVE_LOCKING`（lockdep） | 锁序反转、错误的加锁上下文（在中断里拿非 irq-safe 锁） | 启动/首轮运行时开销，生产一般不 |
| `CONFIG_DEBUG_PREEMPT` | 在原子上下文里调用可睡眠函数 | 小 |
| `CONFIG_DEBUG_ATOMIC_SLEEP` | `might_sleep()` 违规（如持有自旋锁时 `kmalloc(GFP_KERNEL)`） | 小 |
| KCSAN（`CONFIG_KCSAN`） | **数据竞争**（并发访问同一地址且无同步） | 很大（10x+），只用于测试 |

> **`might_sleep()` / `might_resched()`** 是给"按最坏情况写"准备的主动自检宏：
> 把它们放在任何**不该睡眠**的函数开头，`CONFIG_DEBUG_ATOMIC_SLEEP` 开着时一旦被抢占上下文调用立刻 `WARN`。
> 这是把"假设"变成"断言"的标准做法。

---

## 二、内核抢占：LKD 时代 3 档，现在是 4 档 + 运行时可切

### 2.1 v6.6 实证：`choice` 里就是四档

```c
/* kernel/Kconfig.preempt —— v6.6 原文（行号为该文件内行号） */
choice
	prompt "Preemption Model"
	default PREEMPT_NONE                       /* :16 ← 默认是"不抢占" */

config PREEMPT_NONE                            /* :18 */
	bool "No Forced Preemption (Server)"
	help
	  ... the traditional Linux preemption model ...
	  throughput ...  server, scientific, and similar workloads

config PREEMPT_VOLUNTARY                       /* :32 */
	bool "Voluntary Kernel Preemption (Desktop)"
	depends on !ARCH_NO_PREEMPT

config PREEMPT                                 /* :51 */
	bool "Preemptible Kernel (Low-Latency Desktop)"
	depends on !ARCH_NO_PREEMPT
	help
	  ... makes all kernel code (that is not executing in a
	  critical section) preemptible ... at the cost of slightly
	  lower throughput ...                     /* :60-62 原文 */

config PREEMPT_RT                              /* :70 */
	bool "Fully Preemptible Kernel (Real-Time)"
	depends on EXPERT && ARCH_SUPPORTS_RT       /* :72 ← 门槛在这里 */
	select PREEMPTION
	help
	  This option turns the kernel into a real-time kernel by replacing
	  various locking primitives (spinlocks, rwlocks, etc.) with
	  preemptible priority-inheritance aware variants, enforcing
	  interrupt threading and introducing mechanisms to break up long
	  non-preemptible sections. This makes the kernel, except for very
	  low level and critical code paths (entry code, scheduler, low
	  level interrupt handling) fully preemptible ...   /* :75-80 原文 */

endchoice                                      /* :87 */
```

> **⚠️ 更正一个常见误解**：`config PREEMPT_RT` 这个 Kconfig 选项**早就在主线里**（v6.6 就有，见 :70），
> 并不是 v6.12 才"出现"。v6.12 的意义是 **RT 最后一批基础设施合入、`ARCH_SUPPORTS_RT` 铺开**，
> 从此 RT 不再是"树外补丁 + 少数架构"，而是主线一等公民。
> 换句话说：**选项早就存在，能用才是 v6.12 的事**。

### 2.2 四档语义对照

| 档位 | 内核态能否被抢占 | 关键差异 | 典型用途 |
|------|-----------------|---------|---------|
| `PREEMPT_NONE` | 只在**返回用户态 / 显式调度点** | 吞吐最高，延迟最差 | 服务器 / HPC（**多数发行版 server 内核默认值**） |
| `PREEMPT_VOLUNTARY` | 额外在 `might_resched()` 埋点上可让出 | 折中 | 桌面发行版（Fedora/Ubuntu desktop 常用） |
| `PREEMPT` | **任意非临界区**可被抢占（"Low-Latency Desktop"） | 延迟低，吞吐略降 | 桌面 / 软实时 / 音视频 |
| `PREEMPT_RT` | **几乎全部**（含大部分临界区） | 自旋锁变成**可抢占的 PI-aware mutex**，中断**线程化** | 硬实时 / 工控 / 低延迟交易 |

**`PREEMPT_RT` 做了三件结构性的事**（help 原文点名）：

1. **spinlock / rwlock → 可抢占、带优先级继承（priority inheritance）的变体**
   → 直接消灭"优先级反转"，但也意味着**自旋锁里不能睡眠"的旧规则不再靠"禁止抢占"来保证**。
2. **中断线程化**（interrupt threading）→ 大部分 ISR 变成可调度的内核线程，能被实时任务抢占。
3. **打散长临界区**（break up long non-preemptible sections）→ 把原本一大段不可抢占的代码切开。

> 例外（help 原文）：**entry code、scheduler、底层中断处理**仍然不可抢占 —— 这三处是"实时性的地板"。

### 2.3 v6.13 的结构性变化：RT 独立 + 新增 LAZY

```c
/* kernel/Kconfig.preempt —— v6.13 原文 */
config ARCH_HAS_PREEMPT_LAZY                   /* :14 新增：架构能力宏 */
choice
config PREEMPT_NONE                            /* :21 */
	depends on !PREEMPT_RT                     /* :23 ← 新增依赖 */
config PREEMPT_VOLUNTARY                       /* :36 */
	depends on !PREEMPT_RT                     /* :39 ← 新增依赖 */
config PREEMPT
config PREEMPT_LAZY                            /* :75 新增第四档 */
	bool "Scheduler controlled preemption model"
	depends on ARCH_HAS_PREEMPT_LAZY
	help
	  ... fundamentally similar to full preemption, but is less
	  eager to preempt SCHED_NORMAL tasks in an attempt to
	  reduce lock holder preemption and recover some of the
	  performance gains seen from using Voluntary preemption.
endchoice                                      /* :87 */
config PREEMPT_RT                              /* :89 ← 移出 choice！ */
	depends on EXPERT && ARCH_SUPPORTS_RT && !COMPILE_TEST
config PREEMPT_DYNAMIC                         /* :113 */
	depends on HAVE_PREEMPT_DYNAMIC             /* ← v6.6 这里是 && !PREEMPT_RT */
```

三个变化各自的意义：

| 变化 | 意义 |
|------|------|
| `PREEMPT_RT` **移出 choice** | RT 从"四选一的一档"变成**正交开关**：可以和 dynamic 共存 |
| `PREEMPT_DYNAMIC` 去掉 `!PREEMPT_RT` | 同一个内核二进制可以**运行时**在 RT / 非 RT 之间切 |
| `PREEMPT_LAZY` 新增 | 折中：对 `SCHED_NORMAL` 任务**不那么急着抢**（减少"抢到锁的持有者"被抢，即 lock holder preemption），只为 RT 任务抢 → 既拿 RT 延迟又尽量保吞吐 |

> **LAZY 的思想值得单独记**：真正的延迟杀手往往不是"抢占不及时"，而是"抢占太及时"——
> 你刚把一个持有锁的任务换下，另一个任务上来就撞在这把锁上空转。
> 所以**调度器驱动**的抢占（知道"现在该不该抢"）优于"见缝就抢"。

### 2.4 `PREEMPT_DYNAMIC`：一个二进制，三种行为

```c
/* kernel/Kconfig.preempt —— v6.6 :96 */
config PREEMPT_DYNAMIC
	bool "Preemption behaviour defined on boot"
	depends on HAVE_PREEMPT_DYNAMIC && !PREEMPT_RT
	select JUMP_LABEL if HAVE_PREEMPT_DYNAMIC_KEY
	default y if HAVE_PREEMPT_DYNAMIC_CALL
	help
	  ... allows to define the preemption model on the kernel
	  command line parameter ...
	  The runtime overhead is negligible with HAVE_STATIC_CALL_INLINE
	  enabled ...                               /* :104-110 原文 */
```

- 启动参数：**`preempt=none` / `preempt=voluntary` / `preempt=full`**
- 实现靠 **static call / jump label**：抢占点被编译成"间接调用桩"，启动时二进制改写成一个 nop 或一条 jump
  → help 原文说开销 "negligible"，这正是"零成本抽象"在抢占模型上的应用。
- 主打用户是**发行版**："同一个内核包同时服务 Server 和 Desktop"。

### 2.5 对驱动作者的三条硬约束

| 约束 | 说明 | 违反后果 |
|------|------|---------|
| **临界区要短** | 抢占内核上，持锁时间长 → 直接变成别任务的延迟 | RT 上延迟尖刺 / `lockdep` 报警 |
| **`preempt_disable()` 不是万能锤** | 它只保证"不被同 CPU 上的任务抢占"，**不防中断、不防其他 CPU** | 以为关了抢占就安全 → 仍被 IRQ/其他核并发 |
| **`in_atomic()` 只用于断言** | 它读 `preempt_count()`；在 `PREEMPT_NONE` 上恒为 0，用它做**逻辑分支**会写出配置相关的代码 | 代码行为随内核配置变化 |

> 配套自检宏：`might_sleep()`（会睡？）、`might_resched()`（会让出？）、`WARN_ON_ONCE(in_atomic())`。

---

## 三、HIGHMEM：一段需要知道、但 64 位上已经作废的历史

### 3.1 为什么历史上必须有 HIGHMEM

32 位内核的虚拟地址空间只有 **4GB**，还要按 `VMSPLIT_3G`（用户 3G / 内核 1G）切分
（`arch/x86/Kconfig:1401` `choice "Memory split"`）。
内核那 **1GB** 里还要塞下：

- **直接映射（lowmem）**：全部物理内存的线性映射 —— 这部分是 `virt_to_phys()` / `page_address()` 成立的前提
- vmalloc 区、kmap 区、模块区、fixmap…

1GB 扣掉其他区域后，能直接映射的物理内存大约只剩 **~896MB**。
**超过 896MB 的物理页就叫 highmem（高端内存）—— 它们没有固定的内核虚拟地址**，想访问必须先"临时映射"。

### 3.2 v6.6 实证：HIGHMEM 是纯 32 位现象

```c
/* arch/x86/Kconfig —— v6.6 */
config X86_32                                  /* :10 */
	def_bool y
	depends on !64BIT
	select KMAP_LOCAL                          /* :19 ← 只有 32 位才 select */
	...
choice
	prompt "High Memory Support"
config HIGHMEM4G                               /* :1385 */
	bool "4GB"
	help
	  Select this if you have a 32-bit processor and between 1 and 4
	  gigabytes of physical RAM.
config HIGHMEM64G                              /* :1391 */
	bool "64GB"
	select X86_PAE                             /* :1394 64G 需要 PAE */
endchoice                                      /* :1399 */
```

而 **`arch/arm64/Kconfig` 里根本没有 `config HIGHMEM`**（v6.6 全文搜不到），只有虚拟地址宽度：

```c
config ARM64_VA_BITS                           /* :1322 */
	int
	default 48 if ARM64_VA_BITS_48             /* 另有 36/39/42/47/52 可选 */
```

**为什么 64 位不需要 HIGHMEM**：

| 架构 | 虚拟地址宽度 | 地址空间 | 结论 |
|------|-------------|---------|------|
| x86_64（4 级页表） | 48 bit | **256 TB** | ≫ 任何现有物理内存 |
| x86_64（5 级，`CONFIG_X86_5LEVEL`，x86/Kconfig:1459） | 57 bit | **128 PB**（help 原文） | 更不可能用完 |
| ARM64（VA_BITS=48，默认） | 48 bit | 256 TB | 同上 |

> 地址空间是"容器"，物理内存是"内容"。容器比内容大好几个数量级时，**"装不下"这个问题根本不存在**，
> 于是"临时映射"这一整套机制（槽位、全局锁、kmap 区）连同它的复杂度一起消失。
> 这就是架构演进**删除整类问题**的例子——不是优化了 HIGHMEM，而是让它变得没有意义。

### 3.3 API 断崖：`kmap_atomic()` 已被明确废弃

LKD 教的是 `kmap()` + `kmap_atomic()`。v6.6 的 `include/linux/highmem.h` 里，**`kmap_atomic` 的注释已经写明不要再用**：

```c
/* include/linux/highmem.h —— v6.6 原文 */

/* ① 老式 kmap：会阻塞                                            :37 */
static inline void *kmap(struct page *page);
 * For highmem pages on 32bit systems this can be slow as the mapping space
 * is limited and protected by a global lock. In case that there is no
 * mapping slot available the function blocks until a slot is released via
 * kunmap().                                                    /* :32-35 */

/* ② kmap_local_page：现代首选                                     :96 */
 * On CONFIG_HIGHMEM=n kernels and for low memory pages this returns the
 * virtual address of the direct mapping. Only real highmem pages are
 * temporarily mapped.                                          /* :85-87 */
 * While kmap_local_page() is significantly faster than kmap() for the
 * highmem case it comes with restrictions about the pointer validity. :89-90
 * On HIGHMEM enabled systems mapping a highmem page has the side effect of
 * disabling migration in order to keep the virtual address stable across
 * preemption. No caller of kmap_local_page() can rely on this side
 * effect.                                                      /* :92-94 */
static inline void *kmap_local_page(struct page *page);
static inline void *kmap_local_folio(struct folio *folio, size_t offset);  /* :132 */

/* ③ kmap_atomic：已废弃                                          :179 */
 * In fact a wrapper around kmap_local_page() which also disables pagefaults
 * and, depending on PREEMPT_RT configuration, also CPU migration and
 * preemption. Therefore users should not count on the latter two side
 * effects.                                                     /* :140-143 */
 * Do not use in new code. Use kmap_local_page() instead.        /* :146 ★ */
```

三代 API 对照：

| API | 是否可睡眠 | 副作用 | 现代建议 |
|-----|-----------|--------|---------|
| `kmap()` / `kunmap()` | **可以睡**（槽位耗尽时阻塞等全局锁，:32-35） | 全局锁竞争，慢 | 仍可用于进程上下文，但优先 `kmap_local_page()` |
| `kmap_atomic()` | 不可睡 | **禁用页错误**；依 `PREEMPT_RT` 配置还会禁迁移和抢占（:140-142） | ❌ **别在新代码里用**（:146） |
| `kmap_local_page()` / `kunmap_local()` | 不可睡 | HIGHMEM 上会顺带禁迁移，**但调用者不得依赖**（:94 原文 "No caller ... can rely on this side effect"） | ✅ **新代码用这个** |

要点：

- `kmap_local_page()` 在 64 位 / lowmem 页上**直接返回直接映射地址，零开销**（:85-87），所以它现在是**无条件安全**的写法，不需要 `#ifdef CONFIG_HIGHMEM`。
- 它的代价是**指针有效期只到 `kunmap_local()`**（且不能跨调度点传递）——这恰恰是"按最坏情况写"的约束，比 `kmap()` 的"全局锁 + 可以一直拿着"更严格，因此也更快。
- 配套：`memcpy_to_page()` / `memcpy_from_page()` / `memset_page()`（highmem.h :404 附近已全部用 `kmap_local_page` 实现）→ **能用这些就别自己 kmap**。

### 3.4 写代码时的唯一纪律

> **不要假设页有内核线性映射，但也不要以为必须 kmap。**
> 正确做法：一律走 `kmap_local_page()`（或更好的 `memcpy_{to,from}_page()`），
> 让它在 64 位上退化成 `page_address()`、在 32 位 highmem 上退化成临时映射。

反面教材（LKD 时代常见、现在仍能在老驱动里看到）：

```c
/* ❌ 错：假设低端页一定有线性地址，直接算偏移 */
char *p = page_address(page);        /* highmem 页返回 NULL！ */
p[0] = 'x';                          /* 空指针解引用 */

/* ✅ 对 */
char *p = kmap_local_page(page);
p[0] = 'x';
kunmap_local(p);
```

---

## 四、"按最坏情况写"的四条具体纪律

LKD 只给了原则。落到日常编码，可以拆成四条可检查的规则：

| # | 纪律 | 具体做法 | 违反后在哪炸 |
|---|------|---------|-------------|
| 1 | **并发当作既定事实** | 共享数据必有锁 / per-CPU / `READ_ONCE`；开 `lockdep` + `KCSAN` 跑一遍 | SMP |
| 2 | **临界区可随时被插** | 短临界区；`preempt_disable()` 只在真的需要时；`might_sleep()` 标记不可睡函数 | `CONFIG_PREEMPT*` |
| 3 | **不假设线性映射** | 统一走 `kmap_local_page()` / `memcpy_*_page()` | 32 位 + 大内存 |
| 4 | **用断言替假设** | `might_sleep()`、`WARN_ON_ONCE(in_atomic())`、`lockdep_assert_held()` | 让 bug 从"偶发"变"必现 WARN" |

> 四条的共同思路：**把"我的机器上没问题"换成"内核替我验证过"**。
> 可移植代码的成本不在写的时候，而在**能不能用工具证明它对**。

---

## 五、HFT 视角

**用户态同样面临这三条**，只是换了名字：

| 内核侧 | 用户态对应 | HFT 中的具体手段 |
|--------|-----------|-----------------|
| SMP 并发 | `std::atomic` / `std::memory_order` | SPSC 环形队列 + acquire/release（见 [19.7](./section-19.7-处理器排序.md)） |
| 伪共享 | 缓存行对齐 | `alignas(64)` + 热冷分离（见 [19.4](./section-19.4-数据对齐和结构体填充.md)） |
| 抢占延迟 | **核隔离 + 实时优先级** | `isolcpus` + `SCHED_FIFO` + 无锁忙轮询 |
| 内存映射 | `mlockall(MCL_CURRENT\|MCL_FUTURE)` | 杜绝缺页（见 [16.2](../../chapter-16-page-cache/notes/section-16.2-缓存回收与双链表策略.md)） |

### 关于"HFT 该不该上 PREEMPT_RT"—— 一个需要拆开的判断

流传的说法是"HFT 都用 RT 内核"，**这只对一半**。真实的选择是：

| 方案 | 调度延迟 | 吞吐 | 适合 |
|------|---------|------|------|
| 通用内核 + **核隔离**（`isolcpus` / `nohz_full`）+ `SCHED_FIFO` 忙轮询 | **已可做到 < 10μs** 抖动 | **最高** | **主流做法**：交易线程独占核，根本不进内核、不被调度 |
| `PREEMPT_RT` | 更确定的**最坏情况**延迟 | 损失（自旋锁变 mutex、中断线程化有额外上下文切换） | 需要**硬保证**的场景（风控看门狗、外设在内核态有实时要求） |

**关键洞察**：如果交易线程已经**独占一个隔离核、全程用户态忙轮询、内存全 mlock**，
那么内核抢占模型的差异**对它几乎不可见**——因为它压根不进内核，也没有别的任务跟它抢这个核。
PREEMPT_RT 真正改善的是**尾巴**（偶发的、由内核活动引入的长延迟），而不是平均值。
所以正确的决策顺序是：

1. 先把**架构**做对（核隔离 + 忙轮询 + mlock + 无锁 + 预分配），这一步的收益远大于换内核；
2. 若仍有无法解释的**尾延迟尖刺**（用 `perf` / `ftrace` 的 `wakeup_rt` tracer 定位到是内核临界区），再考虑 RT；
3. 上 RT 前先量吞吐代价——RT 把自旋锁换成 PI mutex 后，**内核网络栈**这段路径可能变慢。

> 一句话：**核隔离解决的是"没人跟我抢"，RT 解决的是"内核不让我等"。
> 前者是免费的（配置一下就行），后者是有代价的。先把免费的拿满。**

### 可移植性纪律在 HFT 代码里的落地

- 时间戳：不要 `rdtsc`（x86 专用）→ 用 `clock_gettime(CLOCK_MONOTONIC)`（vDSO 加速，见 [19.6](./section-19.6-时间与页大小.md)）
- 字节序：不要直接 `memcpy` 网络报文到结构体 → 用 `__le32` + `le32_to_cpu`（sparse 能抓错，见 [19.5](./section-19.5-字节序.md)）
- 整数宽度：不要 `long`（LP64/LLP64 分歧）→ 用 `int64_t` / `uintptr_t`（见 [19.2](./section-19.2-字长和数据类型.md)）
- 无锁：不要依赖"x86 上跑过" → 用 acquire/release + 在 ARM64 上同步压测（见 [19.7](./section-19.7-处理器排序.md)）

---

### 章节回溯

| 主题 | 章节 |
|------|------|
| SMP / 锁 / 无锁 | **Ch 9–10** |
| 抢占与调度 | **Ch 4**（进程调度）· 本节补充 v6.6/v6.13 演进 |
| HIGHMEM / 内存 zone | **Ch 12**（内存管理）|
| 内存屏障 | **[19.7 处理器排序](./section-19.7-处理器排序.md)** |
| 页缓存中的 kmap 使用者 | [16.4 缓冲区高速缓存](../../chapter-16-page-cache/notes/section-16.4-缓冲区高速缓存.md) |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `CONFIG_PREEMPT_NONE`、`PREEMPT_VOLUNTARY`、`PREEMPT`、`PREEMPT_RT` 四档有什么区别？对 HFT 该怎么选？

<details><summary>答案</summary>

**四档语义**（v6.6 `kernel/Kconfig.preempt` 实证 `:18/:32/:51/:70`，`endchoice` 在 `:87`，`default PREEMPT_NONE` 在 `:16`）：

| 档位 | 内核态何时可被抢占 | 代价 | 典型用途 |
|------|------------------|------|---------|
| `PREEMPT_NONE` | 只在返回用户态 / 显式调度点 | 延迟最差，吞吐最高 | 服务器 / HPC（多数发行版 server 默认） |
| `PREEMPT_VOLUNTARY` | 额外在 `might_resched()` 埋点 | 折中 | 桌面发行版 |
| `PREEMPT` | **任意非临界区**（"Low-Latency Desktop"） | 吞吐略降（help 原文 "slightly lower throughput"） | 桌面 / 软实时 |
| `PREEMPT_RT` | **几乎全部**（自旋锁 → 可抢占 PI mutex；中断线程化） | 吞吐损失最大 | 硬实时 / 工控 |

> **⚠️ 更正**：`config PREEMPT_RT` 在 **v6.6 主线 Kconfig 里就存在**（:70，但 `depends on EXPERT && ARCH_SUPPORTS_RT`）。
> **v6.12** 的意义是 RT 基础设施**完全合入**、架构支持铺开，从此不需要树外补丁。
> 所以准确说法是"选项早有，v6.12 起才真正可用"，而不是"v6.12 才加入"。
> 另外 **v6.13** 又做了两处结构性调整：`PREEMPT_RT` 移出 `choice`（:89）、新增 `PREEMPT_LAZY` 档（:75）。

**HFT 怎么选**（这一步常被简化成"上 RT"，其实应该分层看）：

1. **先做架构层，收益最大且免费**：
   核隔离（`isolcpus` + `nohz_full`）+ `SCHED_FIFO` + 用户态忙轮询 + `mlockall(MCL_CURRENT|MCL_FUTURE)` + 内存预分配 + 无锁队列。
   做到这一步，交易线程**根本不进内核、也不与任何任务共享核**，调度延迟抖动已经可以压到 10μs 量级。
2. **再做内核层**：如果 `ftrace` 的 `wakeup_rt` / `perf sched latency` 显示尖刺来自**内核临界区**，才考虑 RT。
3. **RT 的代价要量**：自旋锁变成 PI mutex、中断线程化都会增加上下文切换，
   **内核网络栈路径可能变慢**——所以"上 RT"不是免费的胜利，是"用吞吐换最坏情况延迟的确定性"。

一句话：**核隔离解决"没人跟我抢"，RT 解决"内核不让我等"。前者免费，后者有代价，先把免费的拿满。**

另外还有一个**不用重编内核**的选项：`CONFIG_PREEMPT_DYNAMIC`（:96）允许用启动参数
`preempt=none|voluntary|full` 在**同一个二进制**上切换，且因 static call / jump label 实现，
help 原文说开销 "negligible"（:110）—— 这正是发行版"一个内核包服务多种场景"的做法，也适合做 A/B 对比测试。

</details>

**Q2.** 老驱动里有 `kmap_atomic()`，能直接改成 `kmap_local_page()` 吗？为什么 `kmap_local_page()` 在 64 位上是零开销的？

<details><summary>答案</summary>

**能改，而且应该改**——`include/linux/highmem.h` 的注释已经明说（v6.6 `:146`）：

> *"Do not use in new code. Use kmap_local_page() instead."*

**两者的关键差别在副作用，不在返回值**：

| | `kmap_atomic()` | `kmap_local_page()` |
|---|---|---|
| 实现 | 是 `kmap_local_page()` 的包装（`:140` 原文 "In fact a wrapper around kmap_local_page()"） | 本体 |
| 禁用页错误 | 是 | 否 |
| 禁用迁移 / 抢占 | **依 `PREEMPT_RT` 配置而定**（`:141-142`），注释明确说 *"users should not count on the latter two side effects"* | HIGHMEM 上会顺带禁迁移，但注释同样警告 *"No caller of kmap_local_page() can rely on this side effect"*（`:94`） |
| 指针有效期 | 到 `kunmap_atomic()` | 到 `kunmap_local()`，**不能跨调度点** |

**迁移步骤**：
1. `kmap_atomic(page)` → `kmap_local_page(page)`，`kunmap_atomic(addr)` → `kunmap_local(addr)`；
2. 检查这段代码**是否原本偷偷依赖了"禁止抢占/禁止页错误"**——比如中间调了会睡的函数、或者跨越了很长的临界区。
   如果依赖了，得显式补 `preempt_disable()`/`pagefault_disable()`（但要重新审视这么长的临界区是否合理）。
3. 能用 `memcpy_to_page()` / `memcpy_from_page()` / `memset_page()` 就别自己 kmap —— 它们内部已经全部改用 `kmap_local_page()` 了（highmem.h `:404` 附近），语义更清楚。

**为什么 64 位上零开销**（`:85-87` 原文）：

> *"On CONFIG_HIGHMEM=n kernels and for low memory pages this returns the virtual address of the direct mapping. Only real highmem pages are temporarily mapped."*

即：在 `CONFIG_HIGHMEM=n`（**所有 64 位架构**都是，见下）或页本身是 lowmem 时，
它**直接返回直接映射区的地址**，就是一次加法，不建立任何页表项、不占 kmap 槽位、不碰全局锁。
只有真正的 highmem 页才走临时映射路径。

**为什么 64 位没有 HIGHMEM**：`arch/x86/Kconfig` 的 `HIGHMEM4G`(:1385)/`HIGHMEM64G`(:1391) 挂在 32 位的
`config X86_32`(:10) 下；而 `arch/arm64/Kconfig` **根本没有 `config HIGHMEM`**（只有 `ARM64_VA_BITS`，:1322，默认 48）。
原因是地址空间够大：x86_64 四级页表 256TB、五级（`CONFIG_X86_5LEVEL`，:1459）128PB，
都远远超过任何机器的物理内存——**"装不下"这个前提不存在，整套临时映射机制连同它的复杂度一起消失**。

</details>

**Q3.** 团队说"我们的驱动在 4 核 x86 开发机上跑了一年没事，可以发布"。从 SMP / 抢占 / HIGHMEM 三个维度各举一个"开发机配置掩盖了 bug"的具体例子，并说明怎么提前发现。

<details><summary>答案</summary>

这三个维度的共同点是：**bug 只在特定配置下存在，而开发机只有一种配置**。

| 维度 | 开发机掩盖了什么 | 具体例子 | 怎么提前发现 |
|------|----------------|---------|-------------|
| **SMP** | 开发机虽然 4 核，但**压测没跑满并发窗口**；或 `CONFIG_SMP` 与生产不同 | 两个 CPU 同时进入 `probe` 里的初始化路径，`if (!drv->init_done) { setup(); drv->init_done = 1; }` 无锁 → 偶发重复初始化 / 半初始化 | 开 **`CONFIG_PROVE_LOCKING`（lockdep）** 跑一遍；更强的上 **`CONFIG_KCSAN`**（数据竞争检测器），它能抓"并发访问同一地址且无同步"，哪怕竞态窗口从未真的命中 |
| **抢占** | 开发机是 `PREEMPT_NONE`（服务器默认 :16），生产是 `PREEMPT_RT` 或 `PREEMPT` | 驱动持有自旋锁期间调用了一个**会睡**的函数（如 `kmalloc(GFP_KERNEL)` / `msleep()` / 用户拷贝）。`PREEMPT_NONE` 下临界区本来就不被抢，"自旋锁里不能睡"这条**恰好没被违反**；换到抢占内核立刻 `BUG: scheduling while atomic` | 开 **`CONFIG_DEBUG_ATOMIC_SLEEP`** —— 它让 `might_sleep()` 在原子上下文显式 `WARN`，把"碰巧没事"变成"必现告警"。同时在每个**不该睡眠**的函数开头主动放 `might_sleep()` / `lockdep_assert_held()` |
| **HIGHMEM** | 开发机是 64 位（`CONFIG_HIGHMEM=n`），生产是 32 位 + 2GB 内存 | `page_address(page)` **对 highmem 页返回 NULL**，直接解引用 → 空指针崩溃。64 位上所有页都有线性映射，这个错误路径**永远不会被执行** | ① 一律用 `kmap_local_page()`（64 位上退化成取直接映射地址，零开销，:85-87），**不要**用 `page_address()` 除非你确定页来自 `GFP_DMA`/lowmem；② 在 CI 里**加一个 32 位 + `CONFIG_HIGHMEM64G` 的交叉编译 + QEMU 跑测**配置 |

**方法论**（对应本节"按最坏情况写"）：

- **别用"跑过"证明正确，用工具证明**：lockdep / KCSAN / `DEBUG_ATOMIC_SLEEP` / `DEBUG_PREEMPT` 这四个开关的组合，
  能把三个维度的概率性 bug 全部变成**必现 WARN**。
- **CI 至少覆盖三种配置**：`allmodconfig`(SMP+PREEMPT) / `PREEMPT_RT` / 32 位 `HIGHMEM64G`。
  不需要都跑性能，只要能启动 + 跑冒烟测试，就能拦住绝大多数"配置相关"的崩溃。
- **主动埋断言**：`might_sleep()`、`WARN_ON_ONCE(in_atomic())`、`lockdep_assert_held(&x)`。
  断言是零成本的自检——它在正确配置下什么都不做，在错误配置下立刻喊。

> 最后呼应 [19.1](./section-19.1-可移植-OS-与-Linux-移植史.md) 的教训：
> 可移植性不是"代码能在多个平台上编译过"，而是**"代码不依赖某个特定配置的偶然性质"**。
> 编译过 ≠ 正确，跑过 ≠ 可移植。

</details>

</details>
---
