# 1.3 AArch64 寄存器组

> 来源：§1.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AArch64 的通用寄存器 X0-X30、特殊寄存器（SP/ELR/SPSR）和 PSTATE 状态寄存器。

## 核心要点

| 寄存器 | 含义 |
|--------|------|
| Xn | 64 位完整寄存器 |
| Wn | 同一寄存器低 32 位；写 Wn → 高 32 位清零 |
| X0-X7 | 参数/返回值（AAPCS64） |
| X29 | FP 帧指针 |
| X30 | LR 返回地址 |
| XZR | 零寄存器，只读恒 0 |

特殊寄存器：SP_ELx（每 EL 独立栈）、ELR_ELx（异常返回 PC）、SPSR_ELx（异常时 PSTATE）。

PSTATE 关键位：
- **NZCV**：条件码（N=负、Z=零、C=进位/无符号溢出、V=有符号溢出）
- **DAIF**：中断屏蔽（D=调试、A=SError、**I=IRQ**、F=FIQ）

> 异常时硬件自动保存 PC→ELR、PSTATE→SPSR；**X0-X30 硬件不保存，软件必须手动压栈**。

## HFT 关联

寄存器使用直接影响性能：
- AAPCS64 调用约定：X0-X7 传参 → 关键路径函数参数不超过 8 个，避免栈传参
- X29/X30 的栈帧保存/恢复有内存访问开销 → 热路径函数用 `-fomit-frame-pointer`
- DAIF 的 I 位控制中断屏蔽 → HFT 关键路段用 `local_irq_disable()` 防止中断抖动
- XZR 零寄存器避免显式清零指令，编译器自动利用

## 自测题

1. 写 W0 后，X0 的高 32 位会保留吗？
<details><summary>答案</summary>
不会。写 W0 会自动清零 X0 的高 32 位。这是 AArch64 硬件行为，不同于 AArch32。
</details>

2. XZR 和 WZR 分别是什么？有什么用途？
<details><summary>答案</summary>
零寄存器，读永远为 0，写被丢弃。用途：比较（CMP Xn, XZR）、清零（MOV Xn, XZR）、替代不需要的目的寄存器。
</details>

3. 异常发生时，硬件自动保存哪些寄存器？哪些需要软件保存？
<details><summary>答案</summary>
硬件自动保存：PC→ELR_ELx、PSTATE→SPSR_ELx、SP 切到目标 EL 的 SP。软件必须保存：X0-X30 通用寄存器（硬件不自动保存）。
</details>

## 参考与延伸

- 原书 §1.3
- [NZCV 条件码专篇](../../NZCV.md)
- [Ch11 异常处理 · 现场保存](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
