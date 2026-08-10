# §21.6 简易调度器

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

BenOS 的简易轮转调度器：维护一个进程循环链表，定时器中断触发 schedule()，schedule 调用 switch_to 切换到下一个进程。

## 核心要点

### 调度器数据结构

```c
struct task_struct *current;      // 当前运行进程
struct task_struct *run_queue;    // 运行队列（循环链表）

// 每个 task 的 next 指向下一个 task
// task0 → task1 → task2 → ... → task0 (循环)
```

### schedule() 实现

```c
void schedule(void) {
    struct task_struct *prev = current;
    current = current->next;   // 轮转调度：下一个
    switch_to(prev, current);  // 切换上下文
}

// 定时器中断触发调度
void timer_irq_handler(void) {
    // 清中断
    // ...
    schedule();  // 切换到下一个进程
}
```

### 调度时机

| 触发方式 | 场景 | 说明 |
|----------|------|------|
| 定时器中断 | 抢占式调度 | 最常见，时间片到 |
| 主动调用 | 协作式调度 | 进程调用 schedule() 主动让出 |
| 中断返回 | 内核态→用户态 | Linux 的 preempt_count 检查 |

> BenOS 简化版：只在定时器中断中调度。真正的 Linux 调度器远比这复杂（CFS/EEVDF 红黑树、优先级、负载均衡）。

## HFT 关联

HFT 系统极力避免被调度器抢占。策略：`SCHED_FIFO` 实时优先级 + `isolcpus` 隔离核 + `nohz_full` 关闭调度时钟中断。但理解调度器原理对诊断延迟尖峰至关重要：`/proc/<pid>/schedstat` 记录进程在 CPU 上运行的时间、等待时间和时间片次数；`perf sched` 可以可视化调度行为。HFT 交易线程的理想状态是 se.sum_exec_runtime 持续增长（一直在运行），nr_switches 接近 0（几乎不被切换）。

## 自测题

1. **BenOS 的调度策略是什么？有什么缺点？**

<details>
<summary>答案</summary>

BenOS 使用**轮转调度**（Round-Robin）：每次调度选 current->next，每个进程公平获得一个时间片。缺点：(1) 没有优先级——所有进程平等，无法让重要进程更频繁运行；(2) 没有 sleep/wake 机制—— sleeping 的进程也在链表中轮转，浪费时间片；(3) 链表遍历 O(1) 但没有负载感知。Linux 的 CFS/EEVDF 用红黑树按虚拟运行时间排序，选出"最该运行的"进程。
</details>

2. **为什么定时器中断中调用 schedule() 是安全的？**

<details>
<summary>答案</summary>

因为定时器中断发生时，当前进程一定在**内核态**（中断处理在内核态执行），此时内核数据结构（调度队列、PCB）可以安全访问。如果在用户态直接调用 schedule()，需要先通过 SVC 进入内核态。此外，中断处理中已经保存了被中断进程的上下文（异常入口自动保存 X0-X30 + SPSR + ELR），schedule() 只需在此基础上再做 switch_to 即可。注意：BenOS 简化了，真正的 Linux 还要检查 `preempt_count == 0` 才允许调度。
</details>

3. **如果 run_queue 中只有一个进程（0号进程），schedule() 会怎样？**

<details>
<summary>答案</summary>

`current->next` 指向自己（只有一个进程的循环链表），所以 `schedule()` 会调用 `switch_to(task0, task0)`——保存 task0 的寄存器到 PCB，然后又从同一个 PCB 加载回来，RET 跳回 schedule() 之后继续执行。虽然做了一次"无用的切换"，但逻辑正确。0 号进程的 cpu_idle() 中的 WFE 会被定时器中断唤醒，然后调度器发现没有其他进程，切回自身继续 WFE。Linux 中这种情况通过 `need_resched` 标志避免不必要的调度。
</details>

## 参考与延伸

- [§21.4 0 号进程与 do_fork](04-do-fork.md) — 进程如何加入调度队列
- [§21.5 上下文切换](05-context-switch.md) — switch_to 实现
- [Ch12 中断处理](../../chapter-12-interrupt-handling/notes/section-0-本章完整概述.md) — 定时器中断处理流程
