# §11.4 硬件保存 + 软件保存

> **来源：** [Ch11 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

异常发生时硬件自动保存 ELR（PC）和 SPSR（PSTATE），但 X0-X30 通用寄存器必须软件手动保存。本节展示完整的保存/恢复代码模式，以及 ERET 返回指令。

## 硬件自动保存

异常发生时，CPU 硬件原子地完成以下操作：

| 步骤 | 操作 | 说明 |
|------|------|------|
| 1 | PSTATE → SPSR_ELx | 保存当前处理器状态（NZCV、DAIF、EL 等） |
| 2 | 返回地址 → ELR_ELx | 保存 PC（同步=触发指令；异步=下一条指令） |
| 3 | 设置 PSTATE | 屏蔽 DAIF、切换到目标 EL |
| 4 | SP 切换 | 切到目标 EL 的 SP（SP_ELx） |
| 5 | PC ← VBAR_ELx + offset | 跳转到异常向量表对应入口 |

### ELR_ELx 的值

| 异常类型 | ELR 保存的地址 | 含义 |
|---------|--------------|------|
| 同步异常（SVC/页错误） | 触发异常的指令地址 | ERET 后重新执行该指令 |
| IRQ/FIQ（异步） | 未执行的下一条指令 | ERET 后继续执行 |
| SError（异步） | 架构定义 | 可能是当前或下一条 |

## 软件必须保存

**X0-X30 通用寄存器硬件不自动保存！** 必须在异常处理入口手动压栈。

### 完整的 IRQ 保存/恢复代码

```asm
irq_handler_spx:                     // VBAR + 0x280
    // === 保存现场 ===
    sub sp, sp, #272                 // 分配栈空间
    stp x0,  x1,  [sp, #0]          // 保存 X0-X1
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
    stp x30, xzr, [sp, #240]        // X30=LR, 另一个槽不用

    // 保存 ELR 和 SPSR（用于调试/嵌套异常）
    mrs x0, ELR_EL1
    mrs x1, SPSR_EL1
    stp x0, x1, [sp, #256]

    // === 调用 C 处理函数 ===
    mov x0, sp                       // 传 pt_regs 指针
    bl  do_irq                       // C 函数处理中断

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

    add sp, sp, #272                 // 释放栈空间
    eret                             // 异常返回：ELR→PC, SPSR→PSTATE
```

## 保存空间计算

| 项目 | 大小 | 说明 |
|------|------|------|
| X0-X30 = 31 个寄存器 | 31 × 8 = 248 字节 | 通用寄存器 |
| ELR + SPSR | 2 × 8 = 16 字节 | 异常返回信息 |
| 对齐填充 | 8 字节 | AAPCS64 16 字节对齐 |
| **总计** | **272 字节** | |

### 栈布局

```
SP → ┌──────────────┐  sp+0
     │ X0           │
     │ X1           │  sp+8
     │ ...          │
     │ X30 (LR)     │  sp+240
     │ (padding)    │  sp+248
     │ ELR_EL1      │  sp+256
     │ SPSR_EL1     │  sp+264
     └──────────────┘  sp+272
```

## ERET 指令

```asm
eret
```

| 操作 | 说明 |
|------|------|
| SPSR_ELx → PSTATE | 恢复处理器状态（NZCV、DAIF、EL） |
| ELR_ELx → PC | 恢复返回地址 |
| EL 切换 | 从当前 EL 切回异常前的 EL |

**ERET 是原子操作**——PC 和 PSTATE 同时恢复，不会被中断打断。这是从高 EL 回到低 EL 的唯一方式。

## 硬件保存 vs 软件保存对比

| 保存项 | 硬件自动 | 软件手动 | 恢复方式 |
|--------|---------|---------|---------|
| PC | ✓ → ELR_ELx | - | ERET 自动 |
| PSTATE | ✓ → SPSR_ELx | - | ERET 自动 |
| SP | ✓ 自动切换 | 需恢复 | 软件 ADD SP |
| X0-X30 | ✗ | ✓ STP 压栈 | 软件将 LDP 恢复 |

## HFT 关联

保存/恢复 31 个寄存器需要约 16 条 STP + 16 条 LDP = 32 条访存指令，在 A76 上约 30-50ns。这是中断延迟的下限——即使 ISR 什么都不做，进出异常的 overhead 也无法消除。HFT 系统如果用中断驱动，这段保存/恢复代码是性能关键路径，应确保栈在 L1 cache 中。某些 HFT 方案用轮询（polling）替代中断，就是为了避免这段开销。

## 自测题

1. **异常发生时硬件自动保存了哪些寄存器？哪些需要软件保存？**
<details><summary>答案</summary>
硬件自动保存：**ELR_ELx**（PC 值）和 **SPSR_ELx**（PSTATE 值），并自动切换 SP。软件必须保存：**X0-X30**（31 个通用寄存器），因为硬件不自动保存通用寄存器，ISR 可能覆盖它们。还需要保存 ELR/SPSR（如果支持嵌套异常）。
</details>

2. **为什么保存空间是 272 字节而不是 248 字节（31×8）？**
<details><summary>答案</summary>
31 个寄存器 × 8 字节 = 248 字节。额外 24 字节：16 字节保存 ELR + SPSR（用于调试或嵌套异常恢复），8 字节对齐填充（AAPCS64 要求 SP 16 字节对齐）。272 = 248 + 16 + 8，保证 SP 对齐。
</details>

3. **ERET 之前如果忘记恢复 SP 会怎样？**
<details><summary>答案</summary>
ERET 恢复 PC 和 PSTATE，但**不恢复 SP**。如果处理中改了 SP（如 `sub sp, sp, #272` 分配栈空间）但忘记 `add sp, sp, #272` 恢复，ERET 后 SP 指向错误地址（偏移了 272 字节）。后续函数调用会在错误栈位置压栈，覆盖数据或栈溢出。
</details>

4. **同步异常和 IRQ 异常的 ELR 值有什么区别？**
<details><summary>答案</summary>
同步异常（如 SVC/页错误）：ELR = **触发异常的指令地址**，ERET 后重新执行该指令。IRQ/FIQ（异步）：ELR = **未执行的下一条指令地址**，ERET 后继续执行被打断的代码。这是因为同步异常需要处理触发指令（如 SVC 需要执行系统调用），异步异常不影响当前指令。
</details>

5. **为什么保存寄存器用 STP 而不是 STR？**
<details><summary>答案</summary>
STP（Store Pair）一次存两个寄存器，比 STR（Store Single）效率高一倍。保存 31 个寄存器：STP 需要 16 条指令（最后一个配 XZR），STR 需要 31 条。STP 减少指令数量 → 减少中断延迟。A64 专门设计了 LDP/STP 双寄存器操作来优化这类场景。
</details>

## 参考与延伸

- [§11.3 异常向量表](03-vector-table.md) — 保存代码在哪个入口执行
- [§11.5 异常综合征](05-esr.md) — 保存完现场后如何判断异常原因
- [Ch12 §12.4 中断现场保存](../../chapter-12-interrupt-handling/notes/section-0-本章完整概述.md) — 更完整的 IRQ 保存/恢复代码
