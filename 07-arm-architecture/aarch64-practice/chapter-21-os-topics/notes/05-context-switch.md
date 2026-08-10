# §21.5 上下文切换 ⭐

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

上下文切换的核心是 `switch_to`：保存 prev 的 callee-saved 寄存器 + SP + LR 到 PCB，加载 next 的 callee-saved + SP + LR，最后 RET 跳到 next 的恢复点。

## 核心要点

### switch_to 汇编实现

```asm
; x0 = prev (旧进程 PCB), x1 = next (新进程 PCB)
switch_to:
    ; === 保存 prev 的上下文 ===
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    str x29,      [x0, #80]   ; FP
    str x30,      [x0, #88]   ; LR (= 切换返回点)
    mov x2, sp
    str x2,       [x0, #96]   ; SP

    ; === 加载 next 的上下文 ===
    ldp x19, x20, [x1, #0]
    ldp x21, x22, [x1, #16]
    ldp x23, x24, [x1, #32]
    ldp x25, x26, [x1, #48]
    ldp x27, x28, [x1, #64]
    ldr x29,      [x1, #80]   ; FP
    ldr x30,      [x1, #88]   ; LR → RET 后跳到 next 上次停的地方
    ldr x2,       [x1, #96]
    mov sp, x2                ; SP

    ret  ; → X30 = next 的恢复点
```

### 切换流程图

```
prev 进程                     next 进程
   │                             │
   │ 调用 schedule()              │ (上次在这里被切走)
   │ → switch_to(prev, next)      │
   │   保存 prev 寄存器到 PCB      │
   │   加载 next 寄存器 from PCB   │
   │   RET ──────────────────────→│ 恢复执行！
   │                              │
   │ (被挂起)                      │ 继续运行...
   │                              │
   │   ... 下次被调度 ...          │ 调用 schedule()
   │ ←──────────────── RET ───────│ switch_to(next, prev)
   │ 恢复执行！                    │ (被挂起)
```

> **核心：** 切换 SP + callee-saved + LR。RET 后自然跳到 next 上次被切走时的位置。

### 寄存器保存/恢复明细

| 操作 | 寄存器 | PCB 偏移 | 指令 | 周期 |
|------|--------|----------|------|------|
| 保存 X19-X20 | STP | #0 | stp x19,x20,[x0,#0] | 1-2 |
| 保存 X21-X22 | STP | #16 | stp x21,x22,[x0,#16] | 1-2 |
| 保存 X23-X24 | STP | #32 | stp x23,x24,[x0,#32] | 1-2 |
| 保存 X25-X26 | STP | #48 | stp x25,x26,[x0,#48] | 1-2 |
| 保存 X27-X28 | STP | #64 | stp x27,x28,[x0,#64] | 1-2 |
| 保存 FP | STR | #80 | str x29,[x0,#80] | 1 |
| 保存 LR | STR | #88 | str x30,[x0,#88] | 1 |
| 保存 SP | STR | #96 | str x2,[x0,#96] | 1 |
| 加载 X19-X28 | LDP×5 | #0-64 | ldp x19,x20,[x1,#0] | 1-2×5 |
| 加载 FP | LDR | #80 | ldr x29,[x1,#80] | 1 |
| 加载 LR | LDR | #88 | ldr x30,[x1,#88] | 1 |
| 加载 SP | LDR+MOV | #96 | ldr x2; mov sp,x2 | 2 |
| 跳转 | RET | — | ret | 2-3 |

**总计：~26 条指令，~25-35 周期（直接开销）**

### 直接开销 vs 间接开销

| 类型 | 内容 | 开销 | 可优化？ |
|------|------|------|---------|
| **直接** | 保存/恢复寄存器 + RET | ~25-35ns | 不可减（寄存器必须保存） |
| **直接** | TLB 切换（用户态进程） | ~100-200ns | ASID 减少刷新 |
| **间接** | cache cold miss | 1-10μs | 预热、绑核 |
| **间接** | TLB 重建 | 100-500ns | 大页减少 TLB 条目 |
| **间接** | branch predictor 预热 | 50-200ns | 不可避免 |
| **总计** | 典型上下文切换 | 2-5μs | — |

> **关键洞察：** 间接开销是直接开销的 100-1000 倍。HFT 优化的重点是减少切换频率，而非加速切换本身。

### 切换时机的完整分析

```
进程 A 运行中
    │
    ├── 1. 定时器中断 → 检查时间片 → 用完 → schedule()
    │      (抢占式切换，最常见)
    │
    ├── 2. 主动调用 schedule() / yield()
    │      (协作式切换，等IO/sleep)
    │
    ├── 3. 中断处理返回 → need_resched == true → schedule()
    │      (高优先级进程就绪)
    │
    ├── 4. 等待信号量/mutex → sleep → schedule()
    │      (阻塞式切换)
    │
    └── 5. 系统调用阻塞 (read/recv) → schedule()
           (IO 等待)
```

## HFT 关联

上下文切换是 HFT 最大的延迟抖动来源之一。一次 `switch_to` 的直接开销约 1-3μs（保存/恢复寄存器 + cache 预热），但**间接开销**（cache/TLB 污染）可能高达 10-50μs。

```c
// HFT 减少上下文切换的完整策略
void hft_setup_realtime(int cpu) {
    // 1. 绑核（独占一个核）
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    sched_setaffinity(0, sizeof(cpuset), &cpuset);

    // 2. 实时调度（最高优先级）
    struct sched_param param = { .sched_priority = 99 };
    sched_setscheduler(0, SCHED_FIFO, &param);

    // 3. 锁定内存（禁止换页）
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // 4. 设置栈预分配（避免缺页）
    volatile char stack_guard[64 * 1024];
    memset((void *)stack_guard, 0, sizeof(stack_guard));

    // 5. 关闭内核干扰（需 root）
    // cmdline: isolcpus=2 nohz_full=2 rcu_nocbs=2
}

// 测量上下文切换开销
static inline uint64_t cntvct(void) {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

void measure_switch_overhead() {
    uint64_t start, end;
    start = cntvct();
    sched_yield();  // 主动让出 → 触发切换
    end = cntvct();
    uint64_t cycles = end - start;
    double ns = (double)cycles / 
                (double)(cntfrq / 1000000);  // cntfrq = ticks/sec
    printf("Context switch: %lu cycles = %.0f ns\n", cycles, ns);
}
```

| HFT 优化手段 | 减少的开销 | 命令/API |
|-------------|-----------|---------|
| SCHED_FIFO | 抢占式切换 | sched_setscheduler |
| isolcpus | 定时器中断开销 | 内核启动参数 |
| nohz_full | 调度时钟中断 | 内核启动参数 |
| mlockall | 换页导致的切换 | mlockall |
| 绑核 | cache/TLB 污染 | sched_setaffinity |
| 预分配栈 | 缺页中断 | memset 触摸页面 |

## 自测题

1. **switch_to 保存 X30(LR) 而不是 PC，为什么？RET 后跳到哪里？**

<details>
<summary>答案</summary>

保存 LR(X30) 而不是 PC，因为 `switch_to` 是通过 `RET`（跳到 X30）来"恢复" next 进程的。next 进程上次被切走时，它正在 `switch_to` 内部执行 `RET`——此时 LR 的值就是 switch_to 的调用者（即 `schedule()` 中 switch_to 之后的指令地址）。保存这个 LR，下次恢复时 RET 就跳回 schedule() 中 switch_to 之后继续执行，好像 switch_to "返回"了一样。对于新进程，LR 是 fn 入口地址（do_fork 设置的）。
</details>

2. **switch_to 为什么要先保存 prev 再加载 next？能反过来吗？**

<details>
<summary>答案</summary>

**不能反过来**。如果先加载 next 的寄存器，会覆盖 prev 的 callee-saved 寄存器（X19-X28），此时 prev 的值还没保存到 PCB，就永久丢失了。必须先保存 prev 的寄存器到 prev 的 PCB，然后再从 next 的 PCB 加载。注意 SP 的切换顺序：先保存 prev 的 SP（用 `MOV x2, SP; STR x2`），再加载 next 的 SP（`LDR x2; MOV SP, x2`），因为保存 prev 寄存器时还需要用当前 SP（栈访问）。
</details>

3. **一次上下文切换的直接开销包括哪些？间接开销是什么？**

<details>
<summary>答案</summary>

**直接开销**：保存/恢复 11 个寄存器（X19-X29 + LR + SP）约 12 条 STP/STR + 12 条 LDP/LDR + 1 条 RET ≈ 25 条指令，约 20-50ns。**间接开销**：切换后 next 进程的栈和数据不在 cache 中，会产生大量 cache miss（cold cache），TLB 也需要重新填充。间接开销通常是直接开销的 100-1000 倍（10-50μs），是 HFT 延迟抖动的主要来源。
</details>

## 参考与延伸

- [§21.2 调用约定与栈帧](02-calling-convention.md) — callee-saved 寄存器约定
- [§21.3 进程控制块 PCB](03-pcb.md) — PCB 中保存的寄存器布局
- [§21.6 简易调度器](06-scheduler.md) — 调用 switch_to 的上下文
