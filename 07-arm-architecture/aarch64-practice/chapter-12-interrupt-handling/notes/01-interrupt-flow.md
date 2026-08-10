# §12.1 中断处理全流程

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

从硬件产生中断信号到 ERET 返回的完整 IRQ 处理流程：中断源 → GIC 仲裁 → CPU IRQ 线 → 硬件保存 → 向量表跳转 → 软件保存 → ISR → EOIR → 恢复 → ERET。

## 核心要点

### 完整流程

```
硬件产生中断信号
  → GIC（中断控制器）仲裁、优先级
  → CPU IRQ 线拉高
  → 当前指令执行完后，CPU 响应
  → 硬件保存 ELR+SPSR，切到 EL1（如果来自 EL0）
  → 跳到 VBAR + 0x280（当前EL SPx IRQ）
  → 软件保存 X0-X30
  → 读 GIC 中断号 → 调用 ISR
  → ISR 处理 → 写 GIC EOIR（End of Interrupt）
  → 恢复 X0-X30
  → ERET
```

### 关键阶段

| 阶段 | 执行者 | 操作 |
|------|--------|------|
| 中断信号 | 硬件外设 | 产生电平/边沿信号 |
| 仲裁路由 | GIC | 优先级排序、路由到目标 CPU |
| CPU 响应 | CPU 硬件 | 当前指令完成后响应 IRQ |
| 硬件保存 | CPU 硬件 | ELR+SPSR 自动保存，切 SP |
| 向量跳转 | CPU 硬件 | PC = VBAR + offset |
| 软件保存 | 软件 | STP X0-X30 到栈 |
| 中断确认 | 软件 | 读 GIC IAR 获取中断号 |
| ISR 处理 | 软件 | 调用对应中断处理函数 |
| 结束中断 | 软件 | 写 GIC EOIR |
| 恢复返回 | 软件 | LDP + ERET |

## HFT 关联

这个完整流程的每一步都贡献延迟。从硬件产生中断到 ISR 开始执行，总延迟 = GIC 仲裁延迟（~50-100ns）+ CPU 响应延迟（当前指令完成，可能 1-10ns）+ 硬件保存（~10ns）+ 向量跳转（~5ns）+ 软件保存（~30-50ns）。总计约 100-200ns。HFT 系统如果用中断接收网卡数据，这个延迟是基础开销，加上 ISR 实际处理时间才是完整中断延迟。

## 自测题

1. **CPU 什么时候响应 IRQ 中断？是立即响应吗？**

<details>
<summary>答案</summary>

**不是立即响应**。CPU 在**当前指令执行完后**才响应 IRQ（如果 PSTATE.I 位为 0）。如果当前指令执行时间较长（如 LDP 多个寄存器），中断响应会有额外延迟。
</details>

2. **GIC EOIR 的作用是什么？如果不写会怎样？**

<details>
<summary>答案</summary>

EOIR（End of Interrupt Register）通知 GIC 中断处理完毕。不写 EOIR → GIC 认为该中断**仍在处理中**，同优先级的中断不再送到 CPU，中断系统实质上卡死。
</details>

3. **从 EL0 用户态发生 IRQ 时，硬件跳到 VBAR_EL1 的哪个偏移？**

<details>
<summary>答案</summary>

跳到 **VBAR_EL1 + 0x480**（低 EL → 当前 EL AArch64，IRQ）。因为中断来自 EL0（低 EL）→ EL1（当前 EL），AArch64 模式，异常类型是 IRQ。
</details>

## 参考与延伸

- [§12.2 中断屏蔽](02-daif.md) — PSTATE.I 位控制是否响应 IRQ
- [§12.4 中断现场保存](04-context-save.md) — 软件保存 X0-X30 的详细代码
- [Ch13 GIC-V2](../../chapter-13-gic-v2/notes/section-0-本章完整概述.md) — GIC 仲裁和 EOIR 的详细原理
