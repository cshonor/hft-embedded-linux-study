# §21.9 易错点清单

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

Ch21 OS 话题实验中的常见错误和陷阱，涵盖上下文切换、栈管理、调度器、系统调用等关键环节。

## 核心要点

### 易错点清单

| # | 错误 | 后果 | 修复 |
|---|------|------|------|
| 1 | 上下文切换没保存 callee-saved | 新进程使用 prev 的寄存器值，数据损坏 | switch_to 中保存 X19-X28 |
| 2 | SP 没切换 | 两个进程用同一个栈，互相覆盖 | switch_to 中保存/恢复 SP |
| 3 | 0 号进程的 PC 设错 | 启动就跳飞 | PC = cpu_idle 地址 |
| 4 | 调度器在中断外调用 | BenOS 可以，Linux 需要 preempt_count | 理解协作式 vs 抢占式区别 |
| 5 | syscall 返回值没正确传回 X0 | 用户态拿到垃圾值 | 异常处理中将结果写入栈上 X0 位置 |
| 6 | 新进程栈未正确初始化 | RET 跳到垃圾地址崩溃 | do_fork 中栈顶放 fn 地址 |

### 详细分析

**陷阱 1：callee-saved 未保存**

```c
// 错误：只切了 SP，没保存寄存器
void bad_switch_to(prev, next) {
    prev->sp = current_sp;
    current_sp = next->sp;
    // X19-X28 仍然是 prev 的值！
    // next 进程会用 prev 的 callee-saved → 数据错误
}

// 正确：保存全部 callee-saved
void good_switch_to(prev, next) {
    // 保存 prev
    save_callee_saved(prev);   // X19-X28 + FP + LR
    prev->sp = current_sp;
    // 加载 next
    current_sp = next->sp;
    restore_callee_saved(next);
}
```

**陷阱 2：SP 未切换**

两个进程共享一个栈 → 互相覆盖局部变量 → 随机崩溃。必须每个进程有独立栈空间，switch_to 中切换 SP。

```c
// 错误症状：随机崩溃，调试极困难
// 两个进程的局部变量地址重叠
// process A: char buf[64];  // SP=0x8000
// process B: char buf[64];  // SP=0x8000 (同一个栈！)
// A 写 buf → B 的 buf 被覆盖

// 修复：确保每个进程有独立栈
void do_fork(void (*fn)(void)) {
    new->sp = (uint64_t)alloc_stack();  // 独立栈！
}
```

**陷阱 3：0 号进程 PC 设错**

```c
// 错误：PC 设为 0 或未初始化
task0->pc = 0;  // 启动后跳到 0 → 崩溃

// 正确
task0->pc = (uint64_t)cpu_idle;
```

**陷阱 5：syscall 返回值**

```c
// 错误：直接写 X0 寄存器
long sys_malloc(size_t size) {
    void *p = alloc(size);
    // return p;  ← 编译器设 X0=p，但 ERET 时从栈恢复 X0
    // 栈上 X0 还是旧值！
}

// 正确：写回栈上保存区
long sys_malloc(size_t size) {
    void *p = alloc(size);
    current_frame->regs[0] = (uint64_t)p;  // 写到栈上 X0 位置
    return 0;  // 返回 0 表示成功
}
```

**陷阱 6：新进程栈初始化**

```c
// 错误：栈未初始化，SP 指向随机内存
int do_fork(void (*fn)(void)) {
    new->sp = (uint64_t)alloc_stack();  // 栈内容随机！
    // 没有在栈顶放 fn 地址
    // switch_to RET → 跳到垃圾地址 → 崩溃
}

// 正确
int do_fork(void (*fn)(void)) {
    uint64_t *sp = (uint64_t *)alloc_stack();
    *sp = (uint64_t)fn;     // 栈顶放 fn
    new->sp = (uint64_t)sp;
}
```

### 调试技巧表

| 症状 | 可能原因 | 调试方法 |
|------|---------|---------|
| 随机崩溃/野指针 | SP 未切换，栈覆盖 | GDB 检查 switch_to 中 SP 值 |
| 进程数据莫名变化 | callee-saved 未保存 | GDB 检查 X19-X28 保存/恢复 |
| 启动就跳飞 | 0 号进程 PC 错误 | GDB 检查 task0->pc |
| syscall 返回垃圾值 | 返回值未写栈保存区 | GDB 检查栈帧 X0 位置 |
| 新进程立即崩溃 | 栈顶未放 fn | GDB 检查 new->sp 内容 |
| 调度后不返回 | LR 未正确保存 | GDB 检查 PCB 偏移 #88 |

### switch_to 正确性检查清单

```c
// HFT 内核调试：switch_to 正确性验证
void verify_switch_to(struct task_struct *prev,
                       struct task_struct *next) {
    // 1. 检查 PCB 偏移与汇编一致
    assert(offsetof(struct task_struct, cpu_context) == 0);
    assert(offsetof(struct task_struct, cpu_context) + 80 == 80);  // FP
    assert(offsetof(struct task_struct, cpu_context) + 88 == 88);  // LR
    assert(offsetof(struct task_struct, cpu_context) + 96 == 96);  // SP

    // 2. 检查 prev 保存后的值
    printf("prev X19=%lx X20=%lx\n",
           prev->cpu_context[0], prev->cpu_context[1]);
    printf("prev FP=%lx LR=%lx SP=%lx\n",
           prev->cpu_context[10], prev->cpu_context[11],
           prev->cpu_context[12]);

    // 3. 检查 next 的 SP 对齐
    if (next->cpu_context[12] & 0xF) {
        printf("ERROR: next SP not 16-aligned: %lx\n",
               next->cpu_context[12]);
    }

    // 4. 检查 next 的 LR 合法
    if (next->cpu_context[11] == 0) {
        printf("ERROR: next LR is NULL (new process?)\n");
    }
}
```

## HFT 关联

这些陷阱在 HFT 系统中虽然不会直接出现（因为不用手写 OS），但理解它们有助于调试内核相关问题。例如：交易系统偶发性数据损坏可能是 `isolcpus` 配置不当导致两个线程跑在同一核上产生 cache 干扰；syscall 返回值错误可能导致 DPDK 的 NIC 配置失败。

```c
// HFT 线程安全检查清单
void hft_safety_check(int pid) {
    // 1. 检查是否被频繁抢占
    FILE *f = fopen("/proc/%d/schedstat", "r");
    unsigned long run, wait, switches;
    fscanf(f, "%lu %lu %lu", &run, &wait, &switches);
    fclose(f);
    if (switches > 10) {
        printf("WARN: PID %d switched %lu times\n", pid, switches);
    }

    // 2. 检查是否绑核
    cpu_set_t mask;
    sched_getaffinity(pid, sizeof(mask), &mask);
    if (CPU_COUNT(&mask) > 1) {
        printf("WARN: PID %d not pinned to single CPU\n", pid);
    }

    // 3. 检查调度策略
    int policy = sched_getscheduler(pid);
    if (policy != SCHED_FIFO) {
        printf("WARN: PID %d not SCHED_FIFO (policy=%d)\n",
               pid, policy);
    }

    // 4. 检查内存锁定
    // (需要 /proc/%d/status 中的 VmLck > 0)
}
```

## 自测题

1. **为什么上下文切换只保存 callee-saved 而不保存 caller-saved？如果漏保存 X19 会怎样？**

<details>
<summary>答案</summary>

只保存 callee-saved 是因为 switch_to 本质是一次函数调用——caller-saved 寄存器在调用边界本就不保证保留，调用者（schedule()）如果需要会自己保存。如果漏保存 X19，next 进程恢复后会使用 prev 进程的 X19 值——如果 next 进程的函数之前把重要变量存在 X19 中（callee-saved 约定保证函数调用后 X19 不变），现在值被 prev 覆盖了，数据损坏。这种 bug 极难调试——只在进程切换后才出现，且表现随机。
</details>

2. **do_fork 中新进程栈顶放 fn 地址，如果忘了这一步会怎样？**

<details>
<summary>答案</summary>

新进程被调度后，switch_to 从 PCB 加载 LR 并 RET。如果 PCB 中的 LR/PC 字段未初始化（或为 0），RET 会跳到地址 0 → 立即触发对齐异常或取指异常 → 系统崩溃。正确做法是把 fn 地址放到新进程栈顶（模拟 LR），switch_to 从 PCB 恢复 LR 后 RET 跳到 fn 开始执行。
</details>

3. **syscall 返回值为什么不能直接写 X0 寄存器？**

<details>
<summary>答案</summary>

因为 SVC 异常入口已经将用户态的 X0 保存到**内核栈**上。EREST 恢复时会从栈上重新加载 X0 到寄存器——如果在内核中直接写 X0 寄存器，ERET 时会被栈上的旧值覆盖。正确做法是把返回值写到栈上 X0 的保存位置（或者异常处理代码中在 ERET 前从栈恢复寄存器时写入正确的值）。这是裸机开发中常见的"栈上保存区"陷阱——修改寄存器不够，必须修改栈上保存的副本。
</details>

## 参考与延伸

- [§21.5 上下文切换](05-context-switch.md) — switch_to 完整实现
- [§21.7 自定义系统调用](07-syscall.md) — SVC 异常处理流程
- [Ch07 A64 工程陷阱](../../chapter-07-a64-traps/notes/section-0-本章完整概述.md) — 汇编层面的常见陷阱
