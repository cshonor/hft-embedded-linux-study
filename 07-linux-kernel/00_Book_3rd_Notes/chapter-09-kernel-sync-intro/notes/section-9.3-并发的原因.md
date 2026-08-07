## ③ 并发的原因 · Causes of Concurrency

内核既要处理 **真并发**，也要处理 **伪并发**：

| 类型 | 说明 |
|------|------|
| **真并发（true concurrency）** | 多 CPU **同时** 执行 |
| **伪并发（pseudo-concurrency）** | 单 CPU 上 **交替** 执行 — 仍可能竞态 |

#### Linux 内核五类并发源

| # | 来源 | 说明 | 章节 |
|---|------|------|------|
| 1 | **中断** | 随时异步插入 | **Ch 7** |
| 2 | **软中断 / tasklet** | 中断返回后异步 | **Ch 8** |
| 3 | **内核抢占** | 内核态可被换下 | **Ch 4** |
| 4 | **睡眠** | 显式或缺页等导致 **进程切换** | **Ch 3–5** |
| 5 | **SMP** | 多处理器真并行 | **Ch 1** |

```
        ┌─ IRQ 上半部 ─────────────┐
        ├─ softirq / tasklet ─────┤
同一数据 ◄├─ 另一 CPU 上的进程 ────┼──► 必须同步
        ├─ 本 CPU 抢占后的另一任务 ─┤
        └─ sleep 唤醒后的竞争者 ────┘
```

**内核开发者：** 写每一行访问共享数据时，问：**五种来源里谁会同时碰到这里？**

→ [01 Day 14 临界区](../../../../05-os-from-scratch/thirty-days-os/day-14-keyboard/)

### 常见陷阱

1. 以为只有多 CPU 才有并发——中断、抢占、softirq 都是并发来源
2. 忽略中断引起的并发——进程在修改数据时被中断，中断处理函数也修改同一数据
3. 以为 preempt_disable() 能防止所有并发——只防抢占，不防中断

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核中并发的来源有哪些？

<details><summary>答案</summary>

① SMP：多 CPU 同时执行。② 中断：hard IRQ 打断进程/softirq。③ softirq：softirq 打断进程上下文。④ 抢占：CONFIG_PREEMPT 时进程可被另一进程抢占。⑤ 信号：某些操作可被信号中断。⑥ 线程化：同一进程的多个内核线程。分析并发：问「这段代码是否可能被另一个执行路径同时进入？」如果是 → 需要同步。

</details>

**Q2.** preempt_disable() 能防止所有并发吗？

<details><summary>答案</summary>

不能。preempt_disable() 只防止本 CPU 上的抢占（内核态），不防：① 其他 CPU 上的并行执行（SMP）。② 中断（hard IRQ 仍可触发）。③ softirq（仍可执行）。全面防止：preempt_disable() + local_irq_disable() + spinlock（防 SMP）。或直接 spin_lock_irqsave()（一步到位）。

</details>

**Q3.** HFT 用户态的并发来源和内核有什么不同？

<details><summary>答案</summary>

用户态并发来源：① 多线程（pthread）。② 信号处理函数。③ atexit/fork handlers。④ 多进程共享内存。用户态不能禁中断/禁抢占（需要 root + 内核模块），所以用：① `std::atomic` 无锁同步。② `std::mutex`（futex）。③ `pthread_cancel` 屏蔽。④ 共享内存 + 原子操作。HFT 热路径只用无锁设计。

</details>

</details>

---
