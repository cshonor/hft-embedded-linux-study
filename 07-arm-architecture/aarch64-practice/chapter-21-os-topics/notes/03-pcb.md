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
    uint64_t pc;              // 保存 PC（恢复点，实际存 LR）
    int pid;                  // 进程 ID
    int state;                // RUNNING / SLEEPING / ZOMBIE
    struct task_struct *next; // 调度链表
};
```

### 关键设计决策

| 字段 | 为什么保存 | 大小 | 备注 |
|------|-----------|------|------|
| cpu_context (X19-X28) | callee-saved，切换时必须保存 | 10×8=80 字节 | 5 对 STP/LDP |
| sp | 每个进程有独立栈 | 8 字节 | 切换 SP 即切栈 |
| pc (实际是 LR) | 恢复执行点 | 8 字节 | switch_to RET 用 |
| state | 调度器判断是否可运行 | 4 字节 | RUNNING/SLEEPING/ZOMBIE |
| pid | 标识进程 | 4 字节 | 分配唯一值 |
| next | 调度链表指针 | 8 字节 | 循环链表 |

> **核心洞察：** 上下文切换只需保存 **callee-saved 寄存器**（X19-X28）+ SP + PC(LR)。caller-saved（X0-X18）调用者自己保存，切换时不需要管。

### PCB 内存布局

```
PCB 起始地址 (task_struct*)
┌─────────────────────────┐ 偏移
│ X19                     │ #0
│ X20                     │ #8
│ X21                     │ #16
│ X22                     │ #24
│ X23                     │ #32
│ X24                     │ #40
│ X25                     │ #48
│ X26                     │ #56
│ X27                     │ #64
│ X28                     │ #72
│ FP (X29)                │ #80
│ LR (X30)                │ #88
│ SP                      │ #96
├─────────────────────────┤
│ pid                     │ #104
│ state                   │ #108
│ next                    │ #112
└─────────────────────────┘
```

> 注意：`cpu_context[8]` 看起来只存 8 个寄存器，但实际代码中 switch_to 手动保存 X19-X28（10 个）+ FP + LR + SP，共 13 个 64 位值。

### 进程状态机

```
                    do_fork
                       │
                       ▼
                 ┌──────────┐
          ┌─────│ RUNNING  │←──── schedule() 选中
          │      └──────────┘
          │            │
          │    主动 yield / 时间片到
          │            │
          │            ▼
          │      ┌──────────┐
          │      │ READY    │──→ schedule() 选中 ──→ RUNNING
          │      └──────────┘
          │
  exit()  │      ┌──────────┐
          └─────│ ZOMBIE   │──→ 父进程回收
                 └──────────┘

  BenOS 简化版只有 RUNNING 状态（没有 READY/SLEEPING）
```

### BenOS vs Linux PCB 对比

| 维度 | BenOS | Linux task_struct |
|------|-------|-------------------|
| 大小 | ~120 字节 | ~9000 字节 |
| 寄存器保存 | X19-X28 + FP + LR + SP | 同 + FPSIMD + TLS + SCTLR 等 |
| 调度信息 | next 指针（循环链表） | se (sched_entity)：vruntime/nice/cpus_allowed |
| 内存管理 | 无（单一地址空间） | mm_struct（页表/ASID/VMA） |
| 文件系统 | 无 | files_struct（fd 表） |
| 信号 | 无 | sighand_struct |
| 命名空间 | 无 | nsproxy |
| 凭证 | 无 | cred（uid/gid/capabilities） |

### Linux 的 cpu_context

```c
// Linux AArch64 的 task_struct 中上下文保存结构
struct cpu_context {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;   // X29
    unsigned long sp;   // 切换后的 SP
    unsigned long pc;   // 切换返回点（LR）
};

// 在 task_struct->thread.cpu_context
// switch_to 汇编读写的偏移与这个结构完全对应
```

## HFT 关联

HFT 系统通常采用线程绑核 + 用户态调度，避免内核上下文切换开销。但理解 PCB 结构对调试至关重要：当交易线程被抢占时，内核将 callee-saved 寄存器保存到 `task_struct->cpu_context`，通过 `crash` 工具或 `/proc/<pid>/stack` 可以查看被抢占时的寄存器状态。

```c
// HFT 线程状态监控
void hft_monitor_thread(int pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/schedstat", pid);
    FILE *f = fopen(path, "r");
    unsigned long run_time, wait_time, nr_switches;
    fscanf(f, "%lu %lu %lu", &run_time, &wait_time, &nr_switches);
    fclose(f);

    // HFT 理想状态：
    // run_time 持续增长（一直在运行）
    // wait_time ≈ 0（几乎不等待）
    // nr_switches 接近 0（几乎不被切换）
    if (nr_switches > 10) {
        printf("WARNING: thread %d switched %lu times!\n",
               pid, nr_switches);
    }
}

// HFT 绑核 + 实时调度
void hft_pin_thread(int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    sched_setaffinity(0, sizeof(cpuset), &cpuset);

    struct sched_param param = { .sched_priority = 99 };
    sched_setscheduler(0, SCHED_FIFO, &param);  // 最高实时优先级
}
```

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

4. **BenOS PCB 和 Linux task_struct 的核心区别是什么？为什么 Linux 那么大？**

<details>
<summary>答案</summary>

BenOS PCB 只有 ~120 字节（寄存器 + PID + state + next），而 Linux task_struct 约 9000 字节。Linux 多出的部分包括：mm_struct（地址空间/页表/ASID）、files_struct（文件描述符表）、sighand_struct（信号处理）、cred（UID/GID/权限）、sched_entity（调度信息：虚拟运行时间/nice/亲和性）、nsproxy（命名空间）等。Linux 是完整 OS，需要支持多地址空间、文件系统、信号、权限隔离；BenOS 是教学 OS，只有单地址空间和轮转调度。
</details>

## 参考与延伸

- [§21.2 调用约定与栈帧](02-calling-convention.md) — caller-saved vs callee-saved
- [§21.4 0 号进程与 do_fork](04-do-fork.md) — 创建新 PCB 的过程
- [§21.5 上下文切换](05-context-switch.md) — switch_to 汇编实现
