## 2. 调度策略与进程抢占

---

### 一、可抢占与 `TIF_NEED_RESCHED`

Linux 进程 **可抢占（Preemptable）**。以下情况触发重新调度：

| 条件 | 说明 |
|------|------|
| 更高优先级进程就绪 | 动态优先级 **高于** 当前运行进程 |
| 时间片耗尽 | **Time quantum** 用完 |
| 标志位 | 内核设置 **`TIF_NEED_RESCHED`**，在适当时机调用 `schedule()` |

→ 内核抢占：[Ch 5](../../chapter-05-kernel-synchronization/notes/section-2-内核抢占.md) · 中断返回检查：[Ch 4](../../chapter-04-interrupts-and-exceptions/notes/section-8-中断返回.md)

---

### 二、普通进程（Conventional）

| 概念 | 范围 / 说明 |
|------|-------------|
| **静态优先级** | **100–139**（**数值越小越高**） |
| 继承 | 新进程继承父进程静态优先级 |
| 修改 | `nice()` 等 syscall |
| **基本时间片** | 由静态优先级决定 — 优先级越高，时间片 **越长** |
| **平均睡眠时间** | 调度器统计，用于识别 **交互式进程** |
| **交互式 Bonus** | 睡眠越久，最多 **+5** 优先级奖励 |
| **动态优先级** | 静态优先级 − Bonus → **实际调度用** |

目标：I/O 密集型交互进程唤醒后 **快速响应**。

---

### 三、实时进程（Real-Time）

| 要点 | 说明 |
|------|------|
| 实时优先级 | **1–99**（高于普通进程） |
| 调度规则 | **总是**先跑实时进程，直到阻塞或完成 |
| **SCHED_FIFO** | 先进先出，无时间片轮转 |
| **SCHED_RR** | Round Robin，带时间片 |

→ HFT 热路径常用 **`SCHED_FIFO`** + 绑核 → [14 HFT](../../../14-hft-engineering/) · [Ch 6 syscall](./section-6-调度相关系统调用.md)

### 常见陷阱

1. 混淆 SCHED_FIFO 和 SCHED_RR——FIFO 无时间片（跑到阻塞/被更高优先级抢占），RR 有时间片（轮转）
2. 以为 SCHED_DEADLINE 是 CFS 的一部分——DEADLINE 是独立的调度类（EDF 算法），优先级最高
3. 在 RT 策略下以为 nice 值还有效——RT 策略不看 nice，看 rt_priority（1-99）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** SCHED_OTHER / SCHED_FIFO / SCHED_RR / SCHED_DEADLINE 四个策略的区别？

<details><summary>答案</summary>

OTHER：普通分时，CFS/EEVDF 调度，nice [-20,19] 影响权重。FIFO：实时，同优先级内 FIFO，无时间片，跑到阻塞/被更高优先级抢占。RR：实时，同优先级内轮转，有时间片（默认 100ms）。DEADLINE：基于 EDF（Earliest Deadline First），需要指定 runtime/deadline/period，优先级最高。

</details>

**Q2.** `rt_runtime_us` 和 `rt_period_us` 是什么？为什么需要？

<details><summary>答案</summary>

RT 线程（FIFO/RR）可以无限占用 CPU，导致系统无响应。`rt_runtime_us`（默认 950000us）和 `rt_period_us`（默认 1000000us）限制：每 `rt_period_us` 时间窗口内，RT 线程最多跑 `rt_runtime_us`。超过后 RT 线程被节流（throttled），让 CFS 运行。可通过 `/proc/sys/kernel/sched_rt_runtime_us` 调整。HFT 设 `-1` 禁用节流（需要确保 RT 线程不会 bug）。

</details>

**Q3.** HFT 用 SCHED_FIFO 时常见的延迟来源有哪些？

<details><summary>答案</summary>

① 中断（hard IRQ）仍可打断 RT 线程——用 `isolcpus` + 中断重定向。② softirq（ksoftirqd）——用 `nohz_full`。③ RT 线程间的锁竞争——用无锁设计。④ 内存分配（page fault）——`mlockall` + 预分配。⑤ CPU 频率调节——`cpufreq=governor performance` 锁定最高频率。⑥ SMT/超线程——`nosmt` 禁用。

</details>

</details>

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 调度器数据结构](./section-3-调度器数据结构.md)
> ↔ [LKD Ch04 §4.6 实时调度策略](../../../05-linux-kernel/chapter-04-process-scheduling/notes/section-4.6-实时调度策略.md)
> ↔ [LKD Ch04 §4.5 抢占与上下文切换](../../../05-linux-kernel/chapter-04-process-scheduling/notes/section-4.5-抢占与上下文切换.md)
> ↔ [LKD Ch04 §4.2 调度策略](../../../05-linux-kernel/chapter-04-process-scheduling/notes/section-4.2-调度策略.md)
