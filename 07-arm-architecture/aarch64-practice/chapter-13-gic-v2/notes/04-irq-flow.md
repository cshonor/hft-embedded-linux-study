# §13.4 中断处理流程

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GICv2 的中断处理代码模式：读 IAR 获取中断号 → switch 分发到 ISR → 写 EOIR 结束。IAR 和 EOIR 必须配对使用。

## 核心要点

### 中断处理代码

```c
void gic_handle_irq(void) {
    // 1. 读 IAR 获取中断号
    uint32_t irq = GICC->IAR;

    // 2. 根据 irq 分发到 ISR
    switch (irq) {
        case 30:  // 通用定时器
            timer_isr();
            break;
        case 33:  // UART
            uart_isr();
            break;
        default:
            // 未知中断
            break;
    }

    // 3. 写 EOIR 结束中断
    GICC->EOIR = irq;
}
```

### IAR/EOIR 配对规则

| 操作 | 寄存器 | GIC 内部状态 |
|------|--------|-------------|
| 读 IAR | GICC_IAR | GIC 返回中断号，标记为 active（正在处理） |
| 写 EOIR | GICC_EOIR | GIC 释放优先级，允许同优先级中断 |

> **IAR 和 EOIR 必须配对**：读 IAR 后 GIC 认为该中断正在处理，直到写 EOIR。
> 不写 EOIR → 该优先级被占住，同优先级的中断不再送来。

### 中断号特殊值

| IAR 返回值 | 含义 |
|------------|------|
| 1023 | **Spurious interrupt**（无中断，通常由竞争条件引起） |

> 读到 1023 时不需要写 EOIR，直接返回即可。

## HFT 关联

中断处理流程中的 IAR 读取是延迟关键路径——从 IRQ 触发到读到 IAR 的延迟决定了中断响应速度。在 GICv2 上这是 MMIO 读操作（~50-100ns），GICv3 用系统寄存器 ICC_IAR1_EL1 更快。switch 分发应该将高频中断（如定时器）放在前面以减少比较次数。读到 spurious interrupt（1023）时要正确处理，否则可能误写 EOIR 导致 GIC 状态混乱。

## 自测题

1. **GICC_IAR 返回 1023 是什么意思？应该怎么处理？**

<details>
<summary>答案</summary>

1023 = **Spurious interrupt**（伪中断），表示没有真正待处理的中断。通常发生在中断竞争条件（多个 CPU 同时读 IAR，只有一个拿到真中断号，其他拿到 1023）。处理：**不写 EOIR**，直接返回。
</details>

2. **如果 ISR 中 case 30（定时器）处理时发生异常崩溃，EOIR 没有写入，会怎样？**

<details>
<summary>答案</summary>

GIC 认为中断 30 **仍在处理中**，优先级被占住。同优先级的中断不再送到 CPU。如果中断 30 是定时器（周期性触发），后续定时器中断也会被阻塞。系统实质上中断停摆。修复：在 ISR 中用 try-catch 或确保 EOIR 在任何路径下都被写入。
</details>

3. **IAR 读操作和 EOIR 写操作哪个更关键？为什么？**

<details>
<summary>答案</summary>

**两者都关键，但 EOIR 更容易遗漏**。读 IAR 是中断处理的自然第一步（不读拿不到中断号），但写 EOIR 在 ISR 末尾，容易被忘记（特别是 ISR 有多个 return 路径时）。不写 EOIR 的后果更严重——GIC 卡死，整个中断系统停摆。建议在 gic_handle_irq 的最外层写 EOIR，不在 ISR 内部写。
</details>

## 参考与延伸

- [§13.2 关键寄存器](02-gic-registers.md) — IAR/EOIR 的寄存器详解
- [§13.3 GIC 初始化流程](03-gic-init.md) — 初始化后才能处理中断
- [§13.8 易错点](08-pitfalls.md) — IAR/EOIR 不配对的陷阱
