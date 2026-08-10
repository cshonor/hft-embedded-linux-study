# §21.3 进程控制块（PCB）

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

进程控制块（PCB）是操作系统中每个进程的核心数据结构，保存进程的运行上下文、状态、PID 等信息。本节讲解 BenOS 简易 PCB 的设计和最小字段集。

## 核心要点

### BenOS 简易 PCB

```c
struct task_struct {
    uint64_t cpu_context[8];  // 保存 X19-X28（callee-saved）
    uint64_t sp;              // 保存 SP
    uint64_t pc;              // 保存 PC（恢复点）
    int pid;                  // 进程 ID
    int state;                // RUNNING / SLEEPING / ZOMBIE
    struct task_struct *next; // 调度链表
};
```

### 关键设计决策

| 字段 | 为什么保存 | 备注 |
|------|-----------|------|
| cpu_context (X19-X28) | callee-saved，切换时必须保存 | 10 个寄存器 = 80 字节 |
| sp | 每个进程有独立栈 | 切换 SP 即可切栈 |
| pc | 恢复执行点 | 实际通过 LR(X30) 恢复 |
| state | 调度器判断是否可运行 | RUNNING/SLEEPING/ZOMBIE |
| next | 调度链表指针 | 循环链表实现轮转 |

> **核心洞察：** 上下文切换只需保存 **callee-saved 寄存器**（X19-X28）+ SP + PC(LR)。caller-saved（X0-X18）调用者自己保存，切换时不需要管。

### Linux 的 task_struct

Linux 的 PCB 远比 BenOS 复杂（数千字段），但核心上下文保存在 `thread_struct cpu_context`，原理相同。

## HFT 关联

HFT 系统通常采用线程绑核 + 用户态调度，避免内核上下文切换开销。但理解 PCB 结构对调试至关重要：当交易线程被抢占时，内核将 callee-saved 寄存器保存到 `task_struct->cpu_context`，通过 `crash` 工具或 `/proc/<pid>/stack` 可以查看被抢占时的寄存器状态。HFT 线程的 `sched_setaffinity` 绑核 + `SCHED_FIFO` 实时调度可以减少被抢占次数，但不能完全消除（中断仍可抢占）。

## 自测题

1. **PCB 中为什么要保存 SP 而不是保存整个栈？**

<details>
<summary>答案</summary>

因为每个进程有自己的**独立栈空间**，切换 SP 指针即可切换到另一个栈——不需要复制栈内容。SP 指向该进程上次被切走时的栈顶，恢复 SP 后就能继续使用该进程的栈。保存整个栈会浪费大量内存（每个进程栈通常 8MB），且复制开销巨大。SP 只是一个指针（8 字节），保存它就等于保存了整个栈的"位置"。
</details>

2. **PCB 保存 PC 还是 LR？两者在上下文切换中的关系？**

<details>
<summary>答案</summary>

BenOS 的 PCB 字段叫 `pc`，但实际保存的是 **LR(X30)**——即进程被切走时的返回地址。上下文切换的 `switch_to` 汇编中：保存 `STR X30, [prev, #88]`（存 LR 到 PCB），恢复时 `LDR X30, [next, #88]`（从 PCB 加载 LR），然后 `RET`（跳到 X30）。所以 PCB 中的"PC"实际是上次 switch_to 的返回点，而不是被中断时的 PC——但对于协作式调度器（主动调用 schedule()），两者等价。
</details>

3. **为什么 PCB 不保存 X0-X18（caller-saved 寄存器）？**

<details>
<summary>答案</summary>

因为 `schedule()` → `switch_to()` 本身就是一次**函数调用**。在函数调用边界，caller-saved 寄存器的值本来就不保证保留——调用者如果需要这些值，会在调用前自己保存。所以当进程调用 `schedule()` 时，它已经遵守了调用约定（caller-saved 寄存器要么不需要保留、要么已保存在自己的栈上）。切换时只需保存 callee-saved 寄存器即可正确恢复。
</details>

## 参考与延伸

- [§21.2 调用约定与栈帧](02-calling-convention.md) — caller-saved vs callee-saved
- [§21.4 0 号进程与 do_fork](04-do-fork.md) — 创建新 PCB 的过程
- [§21.5 上下文切换](05-context-switch.md) — switch_to 汇编实现
