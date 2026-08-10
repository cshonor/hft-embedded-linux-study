# §12.2 中断屏蔽（DAIF）

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

PSTATE 中的 DAIF 位控制 4 种异常的屏蔽：D（Debug）、A（SError）、I（IRQ）、F（FIQ）。进入异常时硬件自动设 DAIF 屏蔽中断，Linux 的 local_irq_disable/enable 底层操作 DAIF。

## 核心要点

### DAIF 位

| 位 | 含义 | 屏蔽的异常 |
|----|------|-----------|
| D | Debug 异常屏蔽 | 断点、单步调试 |
| A | SError（异步错误）屏蔽 | 系统总线错误 |
| **I** | **IRQ 屏蔽** ← 最常用 | 外部硬件中断 |
| F | FIQ 屏蔽 | 快速中断 |

### 操作指令

```asm
// 关中断（屏蔽 IRQ）
msr DAIFSet, #0xf       // 屏蔽全部（D+A+I+F）
// 或只屏蔽 IRQ
msr DAIFSet, #0x2       // 只设 I 位

// 开中断
msr DAIFClr, #0x2       // 清 I 位（允许 IRQ）
```

### 关键规则

| 场景 | DAIF 状态 |
|------|-----------|
| 进入异常时 | 硬件**自动设 I/F 位**（关中断） |
| ERET 返回时 | 硬件从 SPSR 恢复 DAIF（回到异常前状态） |
| Linux `local_irq_disable()` | 底层 = `msr DAIFSet, #0x2` |
| Linux `local_irq_enable()` | 底层 = `msr DAIFClr, #0x2` |

> **异常入口硬件自动关中断**：进入异常时 PSTATE 的 I/F 位被设为 1（自动关 IRQ 和 FIQ），防止中断嵌套。要嵌套需在 ISR 中手动开中断。

## HFT 关联

HFT 系统中，DAIF 控制是延迟确定性的核心。在交易关键路径上，通常 `DAIFSet #0xf` 屏蔽所有中断，确保交易逻辑不被打断。但屏蔽中断意味着网卡数据无法通过中断通知 CPU——需要用轮询模式。理解 DAIF 的硬件自动行为很重要：进入异常时硬件自动关 IRQ，所以 ISR 中默认不会嵌套中断，除非手动 `DAIFClr #0x2`。

## 自测题

1. **进入 IRQ 异常后，I 位的值是什么？为什么？**

<details>
<summary>答案</summary>

I 位被**自动设为 1**（屏蔽 IRQ）。这是硬件行为——进入异常时 PSTATE 的 I/F 位自动置 1，防止在处理当前中断时被新的 IRQ 打断（默认不嵌套）。
</details>

2. **`msr DAIFSet, #0x2` 和 `msr DAIFClr, #0x2` 分别做什么？**

<details>
<summary>答案</summary>

`DAIFSet, #0x2` = **设 I 位**（屏蔽 IRQ，关中断）。`DAIFClr, #0x2` = **清 I 位**（允许 IRQ，开中断）。0x2 = bit[1] = I 位。
</details>

3. **ERET 返回时 DAIF 会变成什么值？**

<details>
<summary>答案</summary>

ERET 从 **SPSR_ELx 恢复 PSTATE**，包括 DAIF。即 DAIF 回到**异常发生前的状态**。如果异常前 I=0（开中断），ERET 后 I=0；如果异常前 I=1（关中断），ERET 后 I=1。
</details>

## 参考与延伸

- [§12.1 中断处理全流程](01-interrupt-flow.md) — DAIF 在流程中的位置
- [§12.4 中断现场保存](04-context-save.md) — ISR 中如果需要嵌套中断
- [§12.7 易错点](07-pitfalls.md) — 中断嵌套的陷阱
