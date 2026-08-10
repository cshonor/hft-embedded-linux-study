# §11.3 异常向量表（VBAR）

> **来源：** [Ch11 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

每个 EL 有自己的向量基址寄存器（VBAR_ELx），指向一张 16 表项的向量表。每项 128 字节，对应 4 种异常类型 × 4 种来源场景。异常发生时硬件自动跳到 VBAR + 偏移处执行。

## 向量表结构

### 16 项 × 128B = 2048B

| 偏移范围 | 来源场景 | 异常类型 | 用途 |
|---------|----------|---------|------|
| 0x000 | 当前 EL SP0 | 同步 | 当前 EL 用 SP_EL0 时的同步异常 |
| 0x080 | 当前 EL SP0 | IRQ | 当前 EL 用 SP_EL0 时的 IRQ |
| 0x100 | 当前 EL SP0 | FIQ | 当前 EL 用 SP_EL0 时的 FIQ |
| 0x180 | 当前 EL SP0 | SError | 当前 EL 用 SP_EL0 时的 SError |
| 0x200 | 当前 EL SPx | 同步 | 当前 EL 用 SP_ELx 时的同步异常 |
| 0x280 | 当前 EL SPx | IRQ | **内核态 IRQ 最常用** |
| 0x300 | 当前 EL SPx | FIQ | 内核态 FIQ |
| 0x380 | 当前 EL SPx | SError | 内核态 SError |
| 0x400 | 低 EL → 当前 EL AArch64 | 同步 | **SVC 系统调用最常用** |
| 0x480 | 低 EL → 当前 EL AArch64 | IRQ | 用户态 → 内核态 IRQ |
| 0x500 | 低 EL → 当前 EL AArch64 | FIQ | 用户态 → 内核态 FIQ |
| 0x580 | 低 EL → 当前 EL AArch64 | SError | 用户态 → 内核态 SError |
| 0x600 | 低 EL → 当前 EL AArch32 | 同步 | AArch32 用户态 → EL1 |
| 0x680 | 低 EL → 当前 EL AArch32 | IRQ | AArch32 用户态 IRQ |
| 0x700 | 低 EL → 当前 EL AArch32 | FIQ | AArch32 用户态 FIQ |
| 0x780 | 低 EL → 当前 EL AArch32 | SError | AArch32 用户态 SError |

### 偏移计算规则

```
偏移 = 来源场景偏移 + 异常类型偏移

来源场景偏移：
  SP0     → 0x000
  SPx     → 0x200
  低EL A64 → 0x400
  低EL A32 → 0x600

异常类型偏移：
  Sync    → 0x000
  IRQ     → 0x080
  FIQ     → 0x100
  SError  → 0x180

示例：用户态 SVC（低EL A64 + Sync）= 0x400 + 0x000 = 0x400
示例：内核态 IRQ（SPx + IRQ）    = 0x200 + 0x080 = 0x280
```

### 为什么每项 128 字节？

每项 128 字节 = 32 条 A64 指令（每条 4 字节）。这足够在向量入口内放：
- 保存寄存器的 STP 指令
- 跳转到实际处理函数的 B 指令

如果入口代码超过 128 字节，需要用 B 跳转到外部处理函数。

## 设置向量表

```asm
// 在 EL1 设置向量表
adrp x0, vector_table           // 获取向量表页地址
add  x0, x0, #:lo12:vector_table // 加页内偏移
msr   VBAR_EL1, x0              // 写入向量基址寄存器
isb                              // 指令同步屏障（确保 VBAR 生效）
```

### 向量表定义（GNU as）

```asm
// 向量表定义
.align 11       // 2048 字节对齐（16 × 128 = 2048）
vector_table:
    // --- 当前 EL SP0 (0x000-0x180) ---
    .align 7    // 128 字节对齐
    b   sync_handler_sp0          // 0x000
    .align 7
    b   irq_handler_sp0           // 0x080
    .align 7
    b   fiq_handler_sp0           // 0x100
    .align 7
    b   serror_handler_sp0        // 0x180

    // --- 当前 EL SPx (0x200-0x380) ---
    .align 7
    b   sync_handler_spx          // 0x200
    .align 7
    b   irq_handler_spx           // 0x280  ← 内核态 IRQ
    .align 7
    b   fiq_handler_spx           // 0x300
    .align 7
    b   serror_handler_spx        // 0x380

    // --- 低 EL → 当前 EL AArch64 (0x400-0x580) ---
    .align 7
    b   sync_handler_el0_a64      // 0x400  ← SVC 系统调用
    .align 7
    b   irq_handler_el0_a64       // 0x480  ← 用户态 IRQ
    .align 7
    b   fiq_handler_el0_a64       // 0x500
    .align 7
    b   serror_handler_el0_a64    // 0x580

    // --- 低 EL → 当前 EL AArch32 (0x600-0x780) ---
    .align 7
    b   sync_handler_el0_a32      // 0x600
    .align 7
    b   irq_handler_el0_a32       // 0x680
    .align 7
    b   fiq_handler_el0_a32       // 0x700
    .align 7
    b   serror_handler_el0_a32    // 0x780
```

## 各 EL 的 VBAR 寄存器

| 寄存器 | 适用 EL | 用途 |
|--------|--------|------|
| VBAR_EL1 | EL1 | 内核异常向量 |
| VBAR_EL2 | EL2 | Hypervisor 异常向量 |
| VBAR_EL3 | EL3 | Secure Monitor 异常向量 |

> **注意**：EL0 没有 VBAR——EL0 不能处理异常，异常总是升到 EL1+。

## 对齐要求

| 对齐 | 值 | 原因 |
|------|----|------|
| 向量表起始 | 2048B（`.align 11`） | VBAR 低 11 位必须为 0 |
| 每个表项 | 128B（`.align 7`） | 硬件按 128B 步进计算偏移 |

`.align n` 在 GNU as (AArch64) 中 = 2^n 字节对齐：
- `.align 11` = 2^11 = 2048 字节
- `.align 7` = 2^7 = 128 字节

## HFT 关联

向量表是中断响应延迟的第一环。向量表放在 cache 热区域可以减少跳转延迟。HFT 系统中，IRQ 向量入口（offset 0x280 或 0x480）的代码应尽可能短——通常只做保存现场 + 跳转到 C 处理函数。在 QEMU 上做裸金属开发时，正确设置 VBAR 是第一步，否则异常发生后跳到随机地址导致死机。

## 自测题

1. **向量表有多少项？每项多大？总共需要多少字节？为什么需要 2048 字节对齐？**
<details><summary>答案</summary>
**16 项**（4 种异常类型 × 4 种来源场景），每项 **128 字节**，总共 **2048 字节**。2048 字节对齐（`.align 11`）是 VBAR 寄存器的硬件要求——硬件用 `VBAR + offset` 计算跳转地址，VBAR 的低 11 位必须为 0（因为 offset 最大 0x780，需要 11 位地址线）。
</details>

2. **EL0 用户态程序执行 SVC 指令时，硬件跳到 VBAR_EL1 的哪个偏移？**
<details><summary>答案</summary>
跳到 **VBAR_EL1 + 0x400**（低 EL → 当前 EL AArch64，同步异常）。SVC 是同步异常，从 EL0（低 EL）进入 EL1（当前 EL），且 EL0 是 AArch64 模式。偏移 = 0x400（来源场景）+ 0x000（同步）= 0x400。
</details>

3. **向量表的前 4 项（SP0）和第二个 4 项（SPx）有什么区别？什么时候用到 SP0？**
<details><summary>答案</summary>
前 4 项用 **SP_EL0**，第二个 4 项用 **SP_ELx**（当前 EL 的 SP）。SP0 场景用于异常发生在当前 EL 但 PSTATE.SP=0（选择 SP_EL0）的情况。Linux 内核线程有时用 SP_EL0 做临时栈。通常 IRQ 从用户态进入内核用第三组（0x400+），内核态发生 IRQ 用第二组（0x200+）。
</details>

4. **为什么每个向量表项是 128 字节而不是 4 字节（一条指令）？**
<details><summary>答案</summary>
128 字节 = 32 条 A64 指令，允许在向量入口内直接放保存寄存器的代码（STP 指令），而不需要先跳转再保存。减少一次跳转 = 减少中断延迟。如果 4 字节只能放一条 B 跳转指令，每次异常入口都要多一次分支跳转。
</details>

5. **`.align 11` 在 AArch64 GNU as 中表示对齐多少字节？**
<details><summary>答案</summary>
在 AArch64 GNU as 中，`.align n` 表示 **2^n 字节对齐**。`.align 11` = 2^11 = 2048 字节对齐。注意：在 x86 GNU as 中 `.align n` 是 n 字节对齐（不是 2^n），架构不同含义不同。AArch64 统一用 2^n。
</details>

## 参考与延伸

- [§11.4 硬件保存+软件保存](04-hw-sw-save.md) — 跳到向量入口后做什么
- [§11.2 异常等级切换](02-el-switch.md) — 4 种来源场景的由来
- [Ch12 §12.4 中断现场保存](../../chapter-12-interrupt-handling/notes/section-0-本章完整概述.md) — IRQ 向量入口的完整保存代码
