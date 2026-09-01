## ⑥ 时间与页大小

#### jiffies 与 HZ

| 事实 | **`HZ`** 因架构/配置而异（100 / 250 / 1000 三档可选） |
|------|------------------------------------------------------|
| 错误 | `jiffies + 300` 当「3 秒」而不看 HZ |
| 正确 | **`msecs_to_jiffies(ms)`** · **`jiffies_to_msecs`** |

```c
unsigned long timeout = jiffies + 3 * HZ;              /* 可以，但别手写 */
unsigned long timeout = jiffies + msecs_to_jiffies(3000);  /* 更好：意图明确 */
unsigned long timeout = jiffies + secs_to_jiffies(3);      /* v5.9+ 有这个宏 */
```

#### 陷阱：`jiffies` 在 32 位架构上只有 32 位

```c
/* include/linux/jiffies.h:85 — v6.6 原文 */
extern u64 __cacheline_aligned_in_smp jiffies_64;
extern unsigned long volatile __cacheline_aligned_in_smp __jiffy_arch_data jiffies;
```

`jiffies` 的类型是 `unsigned long`——**32 位系统上就是 32 位**：

| HZ | 32 位 `jiffies` 回绕周期 |
|----|------------------------|
| 100 | ~497 天 |
| 250 | ~199 天 |
| **1000** | **~49.7 天** |

```c
/* include/linux/jiffies.h:74 — v6.6 原文注释 */
/*
 * The 64-bit value is not atomic on 32-bit systems - you MUST NOT read it
 * without sampling the sequence number in jiffies_lock.
 * get_jiffies_64() will do this for you as appropriate.
 */
```

| 规则 | 说明 |
|------|------|
| 32 位上直接读 `jiffies_64` | ❌ **非原子**（要读两个 32 位半字，中间可能进位） |
| 必须读 64 位时 | ✅ 用 **`get_jiffies_64()`**（内部用 seqlock 保护） |
| 比较时间 | ✅ 用 `time_after(a,b)` / `time_before()` 宏——**它们对回绕是安全的**（转成有符号差值比较） |

> **`time_after` 为什么对回绕安全？**
> ```c
> #define time_after(a,b) ((long)((b) - (a)) < 0)
> ```
> 把差值转成 `long`（有符号）再判正负：即使 `a` 刚回绕成 0、`b` 还是 `UINT_MAX`，
> `(long)(b - a)` 也是一个**很小的正数**。这是经典的"环形计数器比较"技巧，
> 和 TCP 序号、[Ch 6.3 kfifo 的 head/tail 回绕](../../chapter-06-kernel-data-structures/notes/section-6.3-队列.md)是**同一个数学**。

#### 现代内核的时间 API（HFT 该用这些）

| 需求 | 内核接口 | 用户态对应 |
|------|---------|-----------|
| **单调时间（纳秒）** | `ktime_get_ns()` / `ktime_get_mono_fast_ns()` | `clock_gettime(CLOCK_MONOTONIC)` |
| 实时钟（wall time） | `ktime_get_real_ns()` | `clock_gettime(CLOCK_REALTIME)` |
| **原始单调（不受 NTP 调整）** | `ktime_get_raw_ns()` | `CLOCK_MONOTONIC_RAW` |
| 高精度定时器 | `hrtimer_start()`（纳秒精度） | `timerfd` / `clock_nanosleep` |
| 调度延迟测量 | `sched_clock()`（架构相关，最快） | — |

> **`jiffies` 的精度是 1/HZ（1~10 ms），而 HFT 需要纳秒。**
> 所以 `jiffies` 只适合"粗粒度超时"（如"30 秒内重试一次"），
> **绝不能用于延迟测量**——那要用 `ktime_get_ns()` 或 TSC。

#### NO_HZ：idle CPU 上的 jiffies 不推进

现代内核默认开 **`CONFIG_NO_HZ_IDLE`**（动态时钟）：

```
旧模型：不管忙不忙，每个 CPU 每 1/HZ 秒收一次时钟中断 → jiffies++

NO_HZ：CPU 进入 idle 后**停掉周期时钟中断** → 省电
       代价：这颗 CPU 上的 jiffies 在 idle 期间**不推进**
```

| 影响 | 说明 |
|------|------|
| 用途受限 | 不能用 jiffies 做"跨 idle 的墙钟计时" |
| 正确做法 | 需要真实时间用 `ktime_get_*()`（基于 clocksource，不受 NO_HZ 影响） |
| HFT 相关 | 交易进程频繁唤醒的 CPU 基本不会进 idle，影响不大；但**监控脚本**要注意 |

→ **Ch 11** `HZ` · `time_after`

#### PAGE_SIZE

| 事实 | **不固定** — x86-64/ARM64 默认 **4KB**，ARM64 还可配 **16KB / 64KB** |
|------|--------------------------------------------------|
| 必须用 | **`PAGE_SIZE`** · **`PAGE_SHIFT`** |

| 架构 | 可配置页大小 |
|------|-------------|
| x86_64 | 4KB（+ 2MB / 1GB 大页） |
| **ARM64** | **4KB / 16KB / 64KB**（`CONFIG_ARM64_4K_PAGES` 等，构建时选择）+ 2MB/32MB/1GB 大页 |
| PowerPC | 4KB / 64KB |
| RISC-V | 4KB（+ Svpbmt 等扩展） |

```c
order = get_order(size);              /* 把字节数转成 alloc_pages 的 order */
pages = alloc_pages(GFP_KERNEL, order);
```

> **页大小差异的实际影响：**
> - **mmap 粒度**：映射的最小单位是一页；
> - **TLB 覆盖**：64KB 页用同样的 TLB 条目数能覆盖 16 倍的地址空间（大内存机器上有优势）；
> - **内存浪费**：64KB 页下，只写 100 字节也要占 64KB；
> - **HFT 大页**：`madvise(MADV_HUGEPAGE)` 或 hugetlbfs，减少 TLB miss（2MB 页在大数据集上收益显著）。

**HFT：** 用户态也一样——**永远用 `sysconf(_SC_PAGESIZE)` 或 `getpagesize()`，别写 4096。**

→ **Ch 12** 页分配 · **Ch 15** 映射 · [Ch 19.1 架构差异表](./section-19.1-可移植-OS-与-Linux-移植史.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** jiffies 和 HZ 的关系？为什么 HFT 不能用 jiffies 做精确计时？

<details><summary>答案</summary>

jiffies = 自启动以来时钟中断次数。HZ = 每秒时钟中断次数（默认 250 或 1000）。1 jiffy = 1/HZ 秒 = 1ms~4ms。HFT 需要纳秒级计时，jiffies 精度太低。用 `ktime_get_ns()` 或 `rdtsc()` 获取纳秒时间戳。TSC（Time Stamp Counter）是 CPU 硬件计数器，每个时钟周期递增，`rdtsc` ~10ns 精度。

</details>

**Q2.** PAGE_SIZE 在不同架构上有什么不同？

<details><summary>答案</summary>

x86/ARM64 默认 4KB。ARM64 支持 16KB/64KB（CONFIG_ARM64_64K_PAGES）。某些架构支持多种页大小。HFT 应避免硬编码 4096，用 PAGE_SIZE 宏。Huge Page 大小也随架构变化：x86 = 2MB/1GB，ARM64 = 2MB/32MB/512MB/1GB。跨架构移植时页大小差异影响 mmap 粒度和 TLB 效率。

</details>

**Q3.** 用 `jiffies` 测量一段代码跑了多久，为什么在 32 位 ARM 上过一段时间会算出荒谬的值？

<details><summary>答案</summary>

因为 **`jiffies` 在 32 位架构上只有 32 位**（`include/linux/jiffies.h:86`）：
```c
extern unsigned long volatile __cacheline_aligned_in_smp __jiffy_arch_data jiffies;
```
`unsigned long` 在 32 位平台 = 32 位。HZ=1000 时，**约 49.7 天回绕一次**。

回绕会带来两个后果：

**① 直接相减可能得到负数/巨大值**
```c
unsigned long t0 = jiffies;
do_work();
unsigned long dt = jiffies - t0;   /* 回绕后变成巨大的无符号数 */
```
如果 `t0` 接近 `UINT_MAX` 而结束时已回绕到很小的值，
无符号减法**仍然得到正确的差值**（模运算的性质），所以**这一步其实安全**。

**② 但直接比较、直接读 64 位就不安全了**
```c
u64 t = jiffies_64;    /* ❌ 32 位上这是两次 32 位读，非原子 */
                       /*    可能读到 "低位已进位、高位还没更新" 的中间态 */
```
源码注释说得很清楚（`jiffies.h:74`）：
> "The 64-bit value is not atomic on 32-bit systems - you MUST NOT read it
> without sampling the sequence number in **jiffies_lock**.
> **get_jiffies_64()** will do this for you as appropriate."

**正确做法：**
| 需求 | 用法 |
|------|------|
| 比较先后 | `time_after(a, b)` / `time_before(a, b)`（**回绕安全**） |
| 读 64 位 | `get_jiffies_64()`（内部用 seqlock） |
| 测耗时 | **不要用 jiffies** —— 用 `ktime_get_ns()` |
| 设超时 | `jiffies + msecs_to_jiffies(ms)` |

**更重要的是：jiffies 根本不该用来测耗时。** 它的精度是 1/HZ = 1~10 ms，
而 HFT 关心的是**微秒/纳秒级**。这段代码的正确写法是：

```c
u64 t0 = ktime_get_ns();
do_work();
u64 dt_ns = ktime_get_ns() - t0;      /* 纳秒，单调，不回绕（几百年） */
```

用户态对应：`clock_gettime(CLOCK_MONOTONIC, &ts)`，
且现代 glibc 通过 **vDSO** 把它变成**纯用户态调用**（不进内核，~20ns），
比 `rdtsc` 更可移植且不受 CPU 迁移/变频影响。

> **结论：** jiffies 是"粗粒度超时"的工具（"30 秒后重试"这种），
> 不是"计时"的工具。混淆这两者是嵌入式/驱动代码里的经典错误。

</details>

</details>
---
