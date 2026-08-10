# 1.5-1.6 必背小结与思考题

> 来源：§1.5-1.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 必背 6 条

1. 只有 **Cortex-A** 有 AArch64；M/R 没有
2. **EL0-EL3** 替代 v7 的 7 种模式
3. X=64 位，W=低 32 且写清高位；XZR≡0
4. PSTATE：**NZCV** + **DAIF**（I=关 IRQ）
5. 异常：硬件存 ELR/SPSR；**通用寄存器软件保存**
6. **ERET**：异常返回，切回原 EL

## 综合自测题

1. 画出 EL0-EL3 的权限层级，标注 Linux 各组件对应的 EL。
<details><summary>答案</summary>
EL0(用户态, 用户程序) < EL1(内核态, Linux内核) < EL2(虚拟化, Hypervisor) < EL3(安全监控, Secure Monitor/TrustZone)
</details>

2. 以下代码执行后 X0 的值是什么？
```asm
mov x0, #0xFFFFFFFFFFFFFFFF
mov w0, #0x12345678
```
<details><summary>答案</summary>
X0 = 0x0000000012345678。写 W0 会清零高 32 位。
</details>

3. 中断屏蔽位 DAIF 中，哪个位控制 IRQ？在内核中如何关中断？
<details><summary>答案</summary>
I 位控制 IRQ。内核关中断用 `msr daifset, #2`（置 I 位）或 C 代码 `local_irq_disable()`。
</details>

4. 为什么 AArch64 抛弃了 ARMv7 的 R0-R15 命名？
<details><summary>答案</summary>
v7 的 R0-R15 中 R13=SP、R14=LR、R15=PC 混用了通用和特殊寄存器。v8 把 SP/LR/PC 独立出来（SP/X30/PC），X0-X30 纯通用，设计更清晰。
</details>

## 参考与延伸

- 原书 §1.5-1.6
- [Ch11 异常处理](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
- [AArch64 命名](../../AARCH64-NAMING.md)
