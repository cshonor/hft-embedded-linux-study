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
```

> **核心：** 切换 SP + callee-saved + LR。RET 后自然跳到 next 上次被切走时的位置。

### 关键细节

| 操作 | 寄存器 | PCB 偏移 |
|------|--------|----------|
| 保存 X19-X20 | STP | #0 |
| 保存 X21-X22 | STP | #16 |
| ... | ... | ... |
| 保存 X27-X28 | STP | #64 |
| 保存 FP (X29) | STR | #80 |
| 保存 LR (X30) | STR | #88 |
| 保存 SP | STR | #96 |

## HFT 关联

上下文切换是 HFT 最大的延迟抖动来源之一。一次 `switch_to` 的直接开销约 1-3μs（保存/恢复寄存器 + cache 预热），但**间接开销**（cache/TLB 污染）可能高达 10-50μs。HFT 策略：(1) `SCHED_FIFO` 实时调度 + `isolcpus` 隔离核，减少被抢占；(2) `mlockall` 锁定内存，避免换页导致的上下文切换；(3) 绑核 + NOHZ_FULL 减少定时器中断。测量切换开销可用 `perf sched` 或自定义测量：在 schedule 前后读 `cntvct_el0` 计数器。

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
