## 2. 内核抢占 (Kernel Preemption)

> Linux 2.6 起：**内核态** 也可被更高优先级任务抢占

---

### 一、是什么

以前：进程进入内核态后，除非主动 `schedule()` 或从中断返回，否则 **一直占着 CPU**。

2.6 引入 **内核抢占**：在内核态运行时，若更高优先级进程就绪或异步事件要求，**仍可被抢占**。

---

### 二、动机

| 目标 | 说明 |
|------|------|
| **降低调度延迟 (Dispatch Latency)** | 交互式、**实时**任务更快得到 CPU |
| 更公平 | 长内核路径不能饿死高优先级任务 |

→ 与 [Ch 7 调度](../../chapter-07-process-scheduling.md) 紧密相关

---

### 三、不允许抢占的情况

| 场景 | 原因 |
|------|------|
| 正在执行 **ISR**（中断服务例程） | 中断上下文不可睡眠/不可随意抢占 |
| 禁用了 **可延迟函数** | softirq 等关键区 |
| **显式禁用内核抢占** | `preempt_disable()` 等 |

持 **自旋锁** 期间通常也禁止抢占 — 否则另一 CPU/路径可能死锁。

→ 中断上下文规则：[Ch 4](../../chapter-04-interrupts-and-exceptions/notes/section-7-可延迟函数与工作队列.md)

---

### 四、HFT 关联

- `PREEMPT_RT` / 低延迟内核：抢占模型与 **spinlock → mutex** 等改造相关  
- 热路径应 **短临界区 + 少持锁** — 抢占只解决「等太久」，不消除锁竞争

### 常见陷阱

1. 以为内核不可被抢占——`CONFIG_PREEMPT=y` 内核允许在大部分内核代码中抢占（除持锁区域）
2. 混淆 `preempt_disable()` 和 `local_irq_disable()`——前者只防抢占，后者还防中断
3. 在 RT 内核（PREEMPT_RT）上以为 spinlock 还是非抢占的——RT 内核的 spinlock 会变成可睡眠的 rt_spinlock

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `CONFIG_PREEMPT_NONE`/`VOLUNTARY`/`FULL`/`RT` 四种抢占模型有什么区别？

<details><summary>答案</summary>

NONE：内核中不可抢占（服务器默认，吞吐优先）。VOLUNTARY：在 `might_sleep()` 点自愿让出（桌面）。FULL：除持锁/中断上下文外可抢占（低延迟桌面/嵌入式）。RT：几乎所有内核代码可抢占，spinlock 变成可睡眠 mutex（工业实时）。HFT 通常用 FULL + `isolcpus`，或 RT + `SCHED_FIFO`。

</details>

**Q2.** `preempt_disable()` 后能调用 `schedule()` 吗？

<details><summary>答案</summary>

不能。`preempt_disable()` 递增 `preempt_count`，`schedule()` 检查 `preempt_count == 0` 才允许调度。违反会触发 `BUG: scheduling while atomic` panic。如果需要在不可抢占区域让出 CPU，用 `preempt_enable_no_resched()` + `schedule()` + `preempt_disable()` 手动管理。

</details>

**Q3.** PREEMPT_RT 内核对 HFT 有什么影响？

<details><summary>答案</summary>

RT 内核把 spinlock 改成可睡眠的 `rt_spinlock`（基于 mutex），中断线程化，`local_irq_disable()` 用 `migrate_disable()` 替代。好处：确定性延迟（最大抢占延迟有界）。坏处：① spinlock 开销增大（从 ~20ns 到 ~200ns）。② 吞吐下降 ~10-30%。HFT 通常评估后选择 FULL（非 RT）+ 手动优化，而非直接用 RT。

</details>

</details>

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 基础同步原语](./section-3-基础同步原语.md)
