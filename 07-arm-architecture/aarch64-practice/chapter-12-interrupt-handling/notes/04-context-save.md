# §12.4 中断现场保存/恢复

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

IRQ 中断入口的完整寄存器保存代码：31 个通用寄存器（X0-X30）用 STP 压栈，调用 C 处理函数后用 LDP 恢复，最后 ERET 返回。

## 完整保存/恢复代码

```asm
// IRQ 入口（向量表跳来）
irq_entry:
    // === 保存现场 ===
    sub sp, sp, #272          // 预留栈空间
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

    // 保存 ELR 和 SPSR（供 do_irq 使用）
    mrs x0, ELR_EL1
    mrs x1, SPSR_EL1
    stp x0, x1, [sp, #256]

    // 调用 C 处理函数
    mov x0, sp                 // 传 pt_regs 指针
    bl  do_irq

    // === 恢复 ELR 和 SPSR ===
    ldp x0, x1, [sp, #256]
    msr ELR_EL1, x0
    msr SPSR_EL1, x1

    // === 恢复现场 ===
    ldp x0,  x1,  [sp, #0]
    ldp x2,  x3,  [sp, #16]
    ldp x4,  x5,  [sp, #32]
    ldp x6,  x7,  [sp, #48]
    ldp x8,  x9,  [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldp x30, xzr, [sp, #240]

    add sp, sp, #272           // 释放栈空间
    eret                       // 异常返回
```

## 保存布局

```
SP → ┌──────────────┐  sp+0
     │ X0           │
     │ X1           │  sp+8
     │ X2           │  sp+16
     │ ...          │
     │ X28          │  sp+224
     │ X29 (FP)     │  sp+232
     │ X30 (LR)     │  sp+240
     │ (padding)    │  sp+248
     │ ELR_EL1      │  sp+256
     │ SPSR_EL1     │  sp+264
     └──────────────┘  sp+272
```

## 栈空间计算

| 项目 | 大小 | 说明 |
|------|------|------|
| X0-X30 (31 个) | 248 字节 | 31 × 8 |
| ELR + SPSR | 16 字节 | 异常返回信息 |
| 对齐填充 | 8 字节 | AAPCS64 16B 对齐 |
| **总计** | **272 字节** | |

## STP vs STR

| 指令 | 每次存 | 保存 31 个寄存器需要 | 对齐 |
|------|-------|-------------------|------|
| STP | 2 个（16 字节） | 16 条（+1 条 padding） | 自然 16B 对齐 |
| STR | 1 个（8 字节） | 31 条 | 需手动对齐 |

STP 优势：指令数减半 → 中断延迟更低。A64 专门设计 LDP/STP 双寄存器操作优化此类场景。

## X29（FP）和 X30（LR）的处理

```asm
// 方式1：X29 和 X30 分开存
stp x28, x29, [sp, #224]    // X28+X29
stp x30, xzr, [sp, #240]    // X30+padding

// 方式2：X29 和 X30 一起存（便于栈回溯）
stp x28, xzr, [sp, #224]    // X28+padding
stp x29, x30, [sp, #240]    // X29(FP)+X30(LR)
```

方式2 便于 GDB 栈回溯（FP→LR 链），Linux 内核用此方式。

## C 处理函数接口

```c
// C 函数接收 pt_regs 指针
struct pt_regs {
    u64 regs[31];    // X0-X30
    u64 sp;
    u64 pc;           // ELR
    u64 pstate;       // SPSR
};

void do_irq(struct pt_regs *regs) {
    // 读 GIC 中断号
    u32 irq = gic_read_iar();

    // 根据中断号调用 handler
    if (irq < NR_IRQS && irq_handlers[irq])
        irq_handlers[irq](regs);

    // 写 GIC EOIR
    gic_write_eoir(irq);
}
```

## HFT 关联

这段保存/恢复代码是中断延迟的主要软件开销：16 条 STP + 16 条 LDP = 32 条访存指令，在 A76 上约 30-50ns（如果栈在 L1 cache）。这是不可压缩的——即使 ISR 什么都不做，进出中断的 overhead 也至少 60-100ns。HFT 系统如果中断频率很高（如每微秒一次），这个开销占比显著。解决方案：用轮询替代中断，或用 ARMv8 的 SDEI（Software Delegated Exception Interface）简化保存。

## 自测题

1. **为什么要保存 X0-X30 共 31 个寄存器？硬件不自动保存吗？**
<details><summary>答案</summary>
硬件**不自动保存通用寄存器**。硬件只保存 ELR（PC）和 SPSR（PSTATE）。ISR（C 函数 do_irq）可能使用任何通用寄存器，如果不保存，被中断的代码的寄存器值会被覆盖，返回后行为错误。保存和恢复是 AAPCS64 调用约定的要求——被调用者需保存 callee-saved 寄存器（X19-X28），但中断是异步的，不知道会打断哪个调用链，所以全部保存。
</details>

2. **`stp x30, xzr, [sp, #240]` 中的 xzr 有什么作用？**
<details><summary>答案</summary>
X30（LR）需要保存，但 STP 需要两个寄存器。用 **xzr（零寄存器）** 作为占位——写入 xzr 等于写 0，不占额外空间。这样栈布局整齐，每个 STP 写 16 字节。也可以用 `stp x29, x30` 保存 FP+LR 用于栈回溯。
</details>

3. **为什么保存代码用 STP 而不是 STR？**
<details><summary>答案</summary>
STP（Store Pair）一次存两个寄存器（16 字节），比 STR 效率更高：31 个寄存器只需 16 条 STP（最后一条存 X30+padding），而 STR 需要 31 条。STP 减少指令数 → 减少中断延迟。STP 还自然保证 16 字节对齐，符合 AAPCS 栈对齐要求。
</details>

4. **为什么要保存 ELR 和 SPSR 到栈上？ERET 不是自动恢复吗？**
<details><summary>答案</summary>
如果支持中断嵌套（ISR 中开中断），新的中断会覆盖 ELR/SPSR（硬件只有一对）。保存到栈上可以在嵌套返回时恢复。即使不嵌套，保存 ELR/SPSR 也有调试价值——C 函数可以读取被中断的 PC 和 PSTATE 做分析。
</details>

5. **保存/恢复代码中 `sub sp, sp, #272` 和 `add sp, sp, #272` 能否用 `stp`/`ldp` 的前缀偏移替代？**
<details><summary>答案</summary>
可以。用 `stp x0, x1, [sp, #-16]!`（前缀偏移，SP 先减再存）可以省去 `sub sp` 指令。恢复用 `ldp x0, x1, [sp], #16`（后缀偏移，先取再加 SP）。但需要精确计算每步偏移，且最后一条的 SP 恢复值必须正确。Linux 内核用这种方式减少指令数。
</details>

## 参考与延伸

- [§12.1 中断处理全流程](01-interrupt-flow.md) — 保存/恢复在整个流程中的位置
- [Ch11 §11.4 硬件保存+软件保存](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md) — 硬件保存 vs 软件保存的区别
- [§12.7 易错点](07-pitfalls.md) — 保存/恢复中的常见错误
