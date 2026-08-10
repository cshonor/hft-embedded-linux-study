# §13.4 中断处理流程

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GICv2 的中断处理代码模式：读 IAR 获取中断号 → switch 分发到 ISR → 写 EOIR 结束。IAR 和 EOIR 必须配对使用。还涉及 spurious interrupt（伪中断）处理和多核竞争场景。

## 核心要点

### 中断处理代码

```c
// GICv2 中断处理入口
void gic_handle_irq(void) {
    uint32_t irq;

    // 1. 读 IAR 获取中断号（原子操作）
    //    GIC 同时将此中断标记为 Active
    irq = GICC->IAR;

    // 2. 检查 spurious interrupt
    if (irq == 1023) {
        // 伪中断：没有真正待处理的中断
        // 不写 EOIR，直接返回
        return;
    }

    // 3. 根据 irq 分发到 ISR
    switch (irq) {
        case 0:    // SGI 0（核间通信）
            ipi_handler(irq);
            break;
        case 30:   // 通用定时器 PPI
            timer_isr();
            break;
        case 33:   // UART SPI
            uart_isr();
            break;
        case 34:   // GPIO SPI
            gpio_isr();
            break;
        default:
            // 未知中断：记录日志，仍需写 EOIR
            unknown_irq_log(irq);
            break;
    }

    // 4. 写 EOIR 结束中断（释放优先级）
    //    必须在所有路径都执行，包括 default
    GICC->EOIR = irq;
}
```

### IAR/EOIR 配对规则

| 操作 | 寄存器 | GIC 内部状态变化 |
|------|--------|-------------------|
| 读 IAR | GICC_IAR | GIC 返回中断号，中断状态 Pending → Active |
| 写 EOIR | GICC_EOIR | GIC 释放优先级，中断状态 Active → Inactive（或 Active+Pending → Pending） |

> **IAR 和 EOIR 必须配对**：读 IAR 后 GIC 认为该中断正在处理，直到写 EOIR。
> 不写 EOIR → 该优先级被占住，同优先级的中断不再送来。

### IAR/EOIR 使用模式对比

| 模式 | 代码 | 优点 | 缺点 |
|------|------|------|------|
| **外层统一 EOIR** | `irq=IAR; isr(irq); EOIR=irq;` | EOIR 不会遗漏 | ISR 不能提前 return |
| **ISR 内部 EOIR** | `irq=IAR; isr(irq){...EOIR=irq;}` | ISR 灵活 | 多 return 路径易遗漏 EOIR |

> 推荐：**外层统一 EOIR** 模式——在 gic_handle_irq 最外层写 EOIR，不在 ISR 内部写。避免 ISR 有多个 return 路径时遗漏。

### 中断号特殊值

| IAR 返回值 | 含义 | 处理方式 |
|------------|------|----------|
| 0-1010 | 有效中断号 | 分发到 ISR，处理后写 EOIR |
| 1023 | **Spurious interrupt**（伪中断） | **不写 EOIR**，直接返回 |

### Spurious Interrupt 原因

```
场景：多核系统中断竞争
CPU0 和 CPU1 同时响应同一个中断
CPU0 先读 IAR → 拿到真实中断号 (如 33)
CPU1 后读 IAR → 拿到 1023 (spurious)

原因：GICD 的 pending 中断已被 CPU0 取走，CPU1 无中断可取
处理：CPU1 不写 EOIR，直接返回
```

| 原因 | 说明 |
|------|------|
| 多核竞争 | 两个 CPU 同时响应同一中断，一个拿到真中断号，另一个拿到 1023 |
| 中断已清除 | 中断源在 IAR 读取前已清除 pending 状态 |
| 优先级变化 | PMR 被修改导致中断不再符合送达条件 |

### 中断处理时序

```
时间线：
T0: 外设触发中断信号 → GICD 标记 pending
T1: GICD 仲裁优先级 → 选最高优先级 → 送 GICC
T2: GICC 判断 PMR → 符合 → 拉 IRQ 线通知 CPU
T3: CPU 响应 IRQ 异常 → 保存现场 → 跳到向量表
T4: 中断处理入口 → 读 IAR → 获取中断号 (T2-T4 = 中断确认延迟)
T5: switch 分发 → 执行 ISR (T4-T5 = 分发延迟)
T6: ISR 执行完毕 → 写 EOIR (T5-T6 = ISR 执行时间)
T7: 恢复现场 → 返回被中断代码 (T6-T7 = 恢复开销)
```

## HFT 关联

中断处理流程中的 IAR 读取是延迟关键路径——从 IRQ 触发到读到 IAR 的延迟决定了中断响应速度。在 GICv2 上这是 MMIO 读操作（~50-100ns），GICv3 用系统寄存器 ICC_IAR1_EL1 更快。switch 分发应该将高频中断（如定时器）放在前面以减少比较次数。读到 spurious interrupt（1023）时要正确处理，否则可能误写 EOIR 导致 GIC 状态混乱。

在 HFT 中建议用函数指针数组替代 switch 分发，O(1) 查找：

```c
// 函数指针数组（比 switch 更快）
typedef void (*irq_handler_t)(uint32_t irq);
static irq_handler_t irq_handlers[1024] = {
    [30] = timer_isr,
    [33] = uart_isr,
    [34] = gpio_isr,
};

void gic_handle_irq_fast(void) {
    uint32_t irq = GICC->IAR;
    if (irq < 1024 && irq_handlers[irq]) {
        irq_handlers[irq](irq);
    }
    GICC->EOIR = irq;
}
```

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

3. **IAR 读操作和 EOIR 写操作哪个更关键？推荐在哪里写 EOIR？**

<details>
<summary>答案</summary>

**两者都关键，但 EOIR 更容易遗漏**。读 IAR 是中断处理的自然第一步（不读拿不到中断号），但写 EOIR 在 ISR 末尾，容易被忘记（特别是 ISR 有多个 return 路径时）。不写 EOIR 的后果更严重——GIC 卡死，整个中断系统停摆。建议在 gic_handle_irq 的**最外层**写 EOIR，不在 ISR 内部写。
</details>

4. **用函数指针数组替代 switch 有什么优势？有什么限制？**

<details>
<summary>答案</summary>

优势：**O(1) 查找**——直接索引到 ISR，无需逐个 case 比较。对于高频中断（如定时器）减少分发延迟。

限制：数组大小固定（最多 1024 项），占用内存。未注册的 IRQ 号对应 NULL 指针需要检查。如果中断号分布稀疏（如只用 30、33、1000），数组浪费空间。在 HFT 中中断号通常集中在低号区，数组方案很高效。
</details>

## 参考与延伸

- [§13.2 关键寄存器](02-gic-registers.md) — IAR/EOIR 的寄存器详解
- [§13.3 GIC 初始化流程](03-gic-init.md) — 初始化后才能处理中断
- [§13.8 易错点](08-pitfalls.md) — IAR/EOIR 不配对的陷阱
- [Ch12 §12.4 上下文保存](../../chapter-12-interrupt-handling/notes/04-context-save.md) — 中断入口的现场保存代码
