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
    timer_clear_irq();
    // 切换到下一个进程
    schedule();
}
```

### 调度时机

| 触发方式 | 场景 | 说明 |
|----------|------|------|
| 定时器中断 | 抢占式调度 | 最常见，时间片到 |
| 主动调用 | 协作式调度 | 进程调用 schedule() 主动让出 |
| 中断返回 | 内核态→用户态 | Linux 的 preempt_count 检查 |
| 阻塞操作 | sleep/IO等待 | 进程主动阻塞，调度器选下一个 |

> BenOS 简化版：只在定时器中断中调度。真正的 Linux 调度器远比这复杂（CFS/EEVDF 红黑树、优先级、负载均衡）。

### BenOS vs Linux 调度器对比

| 维度 | BenOS | Linux CFS/EEVDF |
|------|-------|-----------------|
| 数据结构 | 循环链表 | 红黑树（按 vruntime 排序） |
| 选择算法 | next 指针 O(1) | 最左节点 O(log N) |
| 时间片 | 固定（每中断切一次） | 动态（按 nice/权重计算） |
| 优先级 | 无 | nice -20~+19，实时 0~99 |
| 负载均衡 | 无 | runqueue 负载迁移 |
| 抢占 | 仅中断时 | 内核态可抢占 (CONFIG_PREEMPT) |
| 睡眠/唤醒 | 无 | wait_queue / swait |
| 多核 | 无 | per-CPU runqueue + 负载均衡 |

### Linux 调度类层次

```
调度器核心 (schedule())
    │
    ├── stop_sched_class (最高优先级，迁移用)
    ├── dl_sched_class (Deadline，实时任务)
    ├── rt_sched_class (SCHED_FIFO/RR，实时)
    │   └── 优先级 0-99，同优先级轮转
    ├── fair_sched_class (CFS/EEVDF，普通进程)
    │   └── 红黑树按 vruntime 排序
    └── idle_sched_class (空闲，0 号进程)
        └── 无可运行进程时执行
```

### CFS 核心概念

```c
// Linux CFS 调度实体
struct sched_entity {
    struct load_weight load;      // 权重（nice 值映射）
    unsigned int on_rq;           // 是否在运行队列
    u64 exec_start;               // 本次开始执行时间
    u64 sum_exec_runtime;         // 总执行时间
    u64 vruntime;                 // 虚拟运行时间（核心）
    u64 prev_sum_exec_runtime;    // 上次切走时的执行时间
};

// vruntime 增长率 = 实际运行时间 / 权重
// nice=0 的进程权重=1024，vruntime 增长 = 实际时间
// nice=-10 的进程权重=9537，vruntime 增长慢 → 更频繁被选中
// nice=+10 的进程权重=110，vruntime 增长快 → 较少被选中

// 调度器选 vruntime 最小的（红黑树最左节点）
```

## HFT 关联

HFT 系统极力避免被调度器抢占。策略：`SCHED_FIFO` 实时优先级 + `isolcpus` 隔离核 + `nohz_full` 关闭调度时钟中断。但理解调度器原理对诊断延迟尖峰至关重要。

```c
// HFT 调度延迟诊断
void hft_sched_diagnosis(int pid) {
    // 1. 检查调度统计
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/schedstat", pid);
    FILE *f = fopen(path, "r");
    unsigned long run_ns, wait_ns, switches;
    fscanf(f, "%lu %lu %lu", &run_ns, &wait_ns, &switches);
    fclose(f);

    printf("PID %d: run=%lu ns, wait=%lu ns, switches=%lu\n",
           pid, run_ns, wait_ns, switches);

    // HFT 理想值：
    // switches < 10（几乎不被切换）
    // wait_ns / run_ns < 0.01（等待时间极短）
    // 如果 switches > 100 → 检查 isolcpus/SCHED_FIFO 配置

    // 2. 检查被抢占次数
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "voluntary_ctxt_switches") ||
            strstr(line, "nonvoluntary_ctxt_switches")) {
            printf("  %s", line);
            // voluntary = 主动让出（sleep/IO）
            // nonvoluntary = 被抢占 → HFT 要最小化
        }
    }
    fclose(f);
}

// HFT 线程理想配置
void hft_optimal_sched_config(int cpu) {
    // 1. 内核启动参数（/boot/cmdline.txt 或 grub）
    // isolcpus=2,3 nohz_full=2,3 rcu_nocbs=2,3
    // 2. 线程配置
    cpu_set_t cs; CPU_ZERO(&cs); CPU_SET(cpu, &cs);
    sched_setaffinity(0, sizeof(cs), &cs);
    struct sched_param sp = { .sched_priority = 99 };
    sched_setscheduler(0, SCHED_FIFO, &sp);
    // 3. 避免内核活动
    mlockall(MCL_CURRENT | MCL_FUTURE);
}
```

| 调度参数 | HFT 推荐值 | 效果 |
|----------|-----------|------|
| SCHED_FIFO | priority 99 | 最高优先级，不被普通进程抢占 |
| isolcpus | 隔离交易核 | 不运行其他内核线程 |
| nohz_full | 交易核 | 关闭调度时钟中断 |
| rcu_nocbs | 交易核 | RCU 回调迁移到其他核 |
| irqaffinity | 非交易核 | 硬中断路由到其他核 |
| mlockall | 全部 | 禁止换页 |

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
