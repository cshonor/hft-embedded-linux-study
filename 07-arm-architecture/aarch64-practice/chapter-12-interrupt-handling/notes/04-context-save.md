# §12.4 中断现场保存/恢复

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

IRQ 中断入口的完整寄存器保存代码：31 个通用寄存器（X0-X30）用 STP 压栈，调用 C 处理函数后用 LDP 恢复，最后 ERET 返回。

## 核心要点

### 完整保存/恢复代码

```asm
// IRQ 入口（向量表跳来）
irq_entry:
    sub sp, sp, #272          // 预留空间
    stp x0,  x1,  [sp, #0]    // 保存 x0-x1
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    stp x30, xzr, [sp, #240]  // x30(LR) + padding

    // 调用 C 处理函数
    bl  do_irq

    // 恢复
    ldp x0,  x1,  [sp, #0]
    // ... 恢复 x0-x30
    ldp x28, x29, [sp, #224]
    ldp x30, xzr, [sp, #240]
    add sp, sp, #272

    eret
```

### 保存布局

| 栈偏移 | 内容 |
|--------|------|
| [sp, #0] | X0, X1 |
| [sp, #16] | X2, X3 |
| ... | ... |
| [sp, #224] | X28, X29 |
| [sp, #240] | X30 (LR), padding |
| [sp, #248-271] | 对齐填充 |

> 也可以用 `stp x29, x30, [sp, #240]` 保存 FP+LR，取决于是否需要调试栈回溯。

## HFT 关联

这段保存/恢复代码是中断延迟的主要软件开销：16 条 STP + 16 条 LDP = 32 条访存指令，在 A76 上约 30-50ns（如果栈在 L1 cache）。这是不可压缩的——即使 ISR 什么都不做，进出中断的 overhead 也至少 60-100ns。HFT 系统如果中断频率很高（如每微秒一次），这个开销占比显著。解决方案：用轮询替代中断，或用 ARMv8 的 SDEI（Software Delegated Exception Interface）简化保存。

## 自测题

1. **为什么要保存 X0-X30 共 31 个寄存器？硬件不自动保存吗？**

<details>
<summary>答案</summary>

硬件**不自动保存通用寄存器**。硬件只保存 ELR（PC）和 SPSR（PSTATE）。ISR（C 函数 do_irq）可能使用任何通用寄存器，如果不保存，被中断的代码的寄存器值会被覆盖，返回后行为错误。
</details>

2. **`stp x30, xzr, [sp, #240]` 中的 xzr 有什么作用？**

<details>
<summary>答案</summary>

X30（LR）需要保存，但 STP 需要两个寄存器。用 **xzr（零寄存器）** 作为占位——写入 xzr 等于写 0，不占额外空间。这样栈布局整齐，每个 STP 写 16 字节。也可以用 `stp x29, x30` 保存 FP+LR 用于栈回溯。
</details>

3. **为什么保存代码用 STP 而不是 STR？**

<details>
<summary>答案</summary>

STP（Store Pair）一次存两个寄存器（16 字节），比 STR 效率更高：31 个寄存器只需 16 条 STP（最后一条存 X30+padding），而 STR 需要 31 条。STP 还自然保证 16 字节对齐，符合 AAPCS 栈对齐要求。
</details>

## 参考与延伸

- [§12.1 中断处理全流程](01-interrupt-flow.md) — 保存/恢复在整个流程中的位置
- [Ch11 §11.4 硬件保存+软件保存](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md) — 硬件保存 vs 软件保存的区别
- [§12.7 易错点](07-pitfalls.md) — 保存/恢复中的常见错误
