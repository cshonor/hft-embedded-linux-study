# §21.4 0 号进程与 do_fork ⭐

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

0 号进程（idle process）是内核启动后的第一个"进程"，当没有其他进程可运行时回到它。do_fork 创建新进程：分配 PCB、分配栈、设置初始 SP 和 PC、加入调度队列。

## 核心要点

### 0 号进程（idle process）

```c
// 0 号进程：内核启动后第一个"进程"
struct task_struct *task0;
task0->pid = 0;
task0->state = RUNNING;
task0->pc = (uint64_t)cpu_idle;  // 死循环

void cpu_idle(void) {
    while (1) {
        wfe;  // 低功耗等待事件
    }
}
```

> 0 号进程永不退出，没有可运行进程时调度器切回它。WFE 让 CPU 进入低功耗状态等待中断唤醒。

### do_fork 流程

```c
int do_fork(void (*fn)(void)) {
    struct task_struct *new = alloc_task();
    new->pid = next_pid++;
    new->state = RUNNING;

    // 设置栈（给新进程分配栈空间）
    uint64_t *sp = alloc_stack();
    *sp-- = (uint64_t)fn;     // 栈顶放 fn 地址（ret 后跳到 fn）
    new->sp = (uint64_t)sp;

    // 加入调度队列
    enqueue_task(new);
    return new->pid;
}
```

### 新进程的"巧妙"栈设计

| 步骤 | 内容 | 目的 |
|------|------|------|
| 1 | 分配新栈 | 每个进程独立栈空间 |
| 2 | 栈顶放 fn 地址 | switch_to 的 RET 跳到 fn |
| 3 | SP 指向栈顶-8 | 模拟"函数调用后准备返回"的状态 |

> 新进程从未执行过，没有"上次被切走的位置"。把 fn 放在栈顶模拟"LR = fn"，switch_to 恢复 LR 后 RET 自然跳到 fn。

## HFT 关联

HFT 系统中 `fork()` 几乎不使用（fork + exec 的 COW 开销太大），而是用 `pthread_create` 或 `clone()` 共享地址空间。但 do_fork 的栈设计思路对理解协程（coroutine）有启发：协程库（如 Boost.Context）创建协程时也是在栈顶放入口函数地址，然后通过 `switch_to` 类似的汇编切换。HFT 的用户态调度器（如 cooperative scheduler）也用类似技术。

## 自测题

1. **新进程的栈顶为什么放 fn 的地址？**

<details>
<summary>答案</summary>

因为 switch_to 的最后一条指令是 `RET`（跳到 X30/LR）。对于新进程，它从未执行过，没有"上次被切走的返回点"。把 fn 放在栈顶（模拟 LR 的位置），switch_to 从 PCB 恢复 LR 时读到 fn 地址，RET 后自然跳到 fn 开始执行。这是一种"伪造返回地址"的技巧——让新进程的首次调度看起来像是从 switch_to "返回"到 fn。
</details>

2. **0 号进程为什么用 WFE 而不是 while(1)?**

<details>
<summary>答案</summary>

`WFE`（Wait For Event）让 CPU 进入**低功耗等待状态**，直到有事件（如中断）唤醒。纯 `while(1)` 会让 CPU 全速空转，浪费功耗和热量。WFE 在等待中断时几乎不消耗功耗——当定时器中断到来时，CPU 自动唤醒（SEV 由中断触发），调度器可以切换到新就绪的进程。这在嵌入式和服务器场景中都很有意义。
</details>

3. **do_fork 分配的栈和父进程的栈有什么关系？**

<details>
<summary>答案</summary>

BenOS 的 do_fork 分配的是**全新栈**，不复制父进程栈内容。这与 Linux 的 `fork()` 不同——Linux fork 会 COW（Copy-On-Write）复制父进程的整个地址空间（包括栈）。BenOS 的 do_fork 更像 Linux 的 `kernel_thread()`——创建一个内核线程，从指定函数开始执行，没有父进程栈数据的继承。新进程从 fn 开始独立执行。
</details>

## 参考与延伸

- [§21.3 进程控制块 PCB](03-pcb.md) — PCB 结构定义
- [§21.5 上下文切换](05-context-switch.md) — switch_to 如何恢复新进程
- [§21.6 简易调度器](06-scheduler.md) — do_fork 后如何加入调度队列
