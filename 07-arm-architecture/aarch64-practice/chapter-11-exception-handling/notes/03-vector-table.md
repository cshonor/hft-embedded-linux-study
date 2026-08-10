# §11.3 异常向量表（VBAR）

> **来源：** [Ch11 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

每个 EL 有自己的向量基址寄存器（VBAR_ELx），指向一张 16 表项的向量表。每项 128 字节，对应 4 种异常类型 × 4 种来源场景。异常发生时硬件自动跳到 VBAR + 偏移处执行。

## 核心要点

### 向量表结构（16 项 × 128B = 2048B）

| 偏移 | 来源场景 | 异常类型 |
|------|----------|----------|
| 0x000-0x180 | 当前 EL SP0 | 同步/IRQ/FIQ/SError |
| 0x200-0x380 | 当前 EL SPx | 同步/IRQ/FIQ/SError |
| 0x400-0x580 | 低 EL → 当前 EL AArch64 | 同步/IRQ/FIQ/SError |
| 0x600-0x780 | 低 EL → 当前 EL AArch32 | 同步/IRQ/FIQ/SError |

> 16 项 = 4 种异常类型（同步/IRQ/FIQ/SError）× 4 种来源场景。每项 128B 可放 32 条指令（足够跳转到处理函数）。

### 设置向量表

```asm
// 在 EL1 设置向量表
adrp x0, vector_table
add  x0, x0, #:lo12:vector_table
msr   VBAR_EL1, x0
isb

// 向量表定义
.align 11       // 2048 字节对齐（16 × 128 = 2048）
vector_table:
    .align 7    // 128 字节对齐
    b   sync_handler_sp0
    .align 7
    b   irq_handler_sp0
    // ... 共 16 项
```

### 关键对齐要求

| 对齐 | 值 | 原因 |
|------|----|------|
| 向量表起始 | 2048B（`.align 11`） | VBAR 硬件要求 |
| 每个表项 | 128B（`.align 7`） | 硬件跳转偏移计算 |

## HFT 关联

向量表是中断响应延迟的第一环。向量表放在 cache 热区域可以减少跳转延迟。HFT 系统中，IRQ 向量入口（offset 0x280 或 0x480）的代码应尽可能短——通常只做保存现场 + 跳转到 C 处理函数。在 QEMU 上做裸金属开发时，正确设置 VBAR 是第一步，否则异常发生后跳到随机地址导致死机。

## 自测题

1. **向量表有多少项？每项多大？总共需要多少字节？为什么需要 2048 字节对齐？**

<details>
<summary>答案</summary>

**16 项**，每项 **128 字节**，总共 **2048 字节**。2048 字节对齐（`.align 11`）是 VBAR 寄存器的硬件要求——硬件用 `VBAR + offset` 计算跳转地址，VBAR 的低 11 位必须为 0。
</details>

2. **EL0 用户态程序执行 SVC 指令时，硬件跳到 VBAR_EL1 的哪个偏移？**

<details>
<summary>答案</summary>

跳到 **VBAR_EL1 + 0x400**（低 EL → 当前 EL AArch64，同步异常）。因为 SVC 是同步异常，且从 EL0（低 EL）进入 EL1（当前 EL），假设是 AArch64 模式。
</details>

3. **向量表的前 4 项（SP0）和第二个 4 项（SPx）有什么区别？什么时候用到 SP0？**

<details>
<summary>答案</summary>

前 4 项用 **SP_EL0**，第二个 4 项用 **SP_ELx**（当前 EL 的 SP）。SP0 场景用于异常发生在当前 EL 但选择使用 SP_EL0 的情况（Linux 内核线程有时用 SP_EL0 做临时栈）。通常 IRQ 从用户态进入内核用第三组（0x400+），内核态发生 IRQ 用第二组（0x200+）。
</details>

## 参考与延伸

- [§11.4 硬件保存+软件保存](04-hw-sw-save.md) — 跳到向量入口后做什么
- [§11.2 异常等级切换](02-el-switch.md) — 4 种来源场景的由来
- [Ch12 §12.4 中断现场保存](../../chapter-12-interrupt-handling/notes/section-0-本章完整概述.md) — IRQ 向量入口的完整保存代码
