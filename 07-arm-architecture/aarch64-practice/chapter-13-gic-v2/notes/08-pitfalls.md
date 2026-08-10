# §13.8 易错点清单

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GICv2 的常见错误总结：IAR/EOIR 不配对、忘记设 PMR、中断号除法搞错、目标 CPU 没设、Pi5 用 GICv2 代码、电平触发不清中断源、spurious 误写 EOIR。每个错误都有明确的症状和修复方法。

## 核心要点

### 7 大易错点

| # | 易错点 | 后果 | 症状 | 修复 |
|---|--------|------|------|------|
| 1 | IAR/EOIR 不配对 | GIC 卡住，不再送中断 | 中断只触发一次 | 确保每个 IAR 都有对应 EOIR |
| 2 | 忘记设 PMR | PMR 默认 0，所有中断被屏蔽 | 中断完全不触发 | `GICC_PMR = 0xFF` |
| 3 | 中断号除法搞错 | ISENABLER 每 32 一组，算错组号 | 使能了错误的中断 | `ISENABLER[N/32]`，位 = `1<<(N%32)` |
| 4 | 目标 CPU 没设 | ITARGETSR 默认 0，无目标 | 中断不送到任何 CPU | 初始化时设 ITARGETSR |
| 5 | Pi5 用 GICv2 代码 | 寄存器地址和流程完全不同 | Pi5 中断不工作 | 改用 GICv3 接口 |
| 6 | 电平触发不清中断源 | 中断疯狂重入 | ISR 反复执行 | ISR 中先清中断源再写 EOIR |
| 7 | Spurious 误写 EOIR | GIC 状态混乱 | 后续中断异常 | 1023 不写 EOIR，直接返回 |

### 寄存器分组速查

| 寄存器 | 分组 | 索引 | 位/字节 | 常见错误 |
|--------|------|------|---------|----------|
| ISENABLER | 32 中断/组 | `[irq/32]` | `1 << (irq%32)` | 写成 `1<<irq`（超过 32 位） |
| IPRIORITYR | 1 中断/字节 | `[irq]` | 直接写字节 | 当成 32 位寄存器写 |
| ITARGETSR | 4 中断/组 | `[irq/4]` | `8*(irq%4)` 位域 | 忘记乘 8（位域不是位号） |
| ICFGR | 16 中断/组 | `[irq/16]` | `2*(irq%16)` 位域 | 忘记乘 2 |

### 调试技巧

| 症状 | 可能原因 | 检查方法 |
|------|----------|----------|
| 中断完全不触发 | PMR=0 / ITARGETSR=0 / ISENABLER 未设 | 打印 GICD_CTLR, GICC_CTLR, GICC_PMR, ITARGETSR 值 |
| 中断只触发一次 | EOIR 没写 / EOIR 写错值 | 在 EOIR 写入前后加打印 |
| 中断疯狂重入 | TVAL 没重设（电平触发） | 检查 ISR 第一行是否清中断源 |
| 中断号不对 | ISENABLER 除法搞错 | 打印 `irq/32`, `irq%32` 验证 |
| 1023 spurious 频繁 | 多核竞争 / 中断已清除 | 检查是否多核同时响应 |
| Pi5 中断不工作 | GICv2 代码跑在 GICv3 上 | 确认 GIC 版本和基址 |

### 常见代码错误示例

```c
// 错误 1: PMR 没设
void gic_init_wrong(void) {
    GICD->CTLR = 1;
    GICC->CTLR = 1;
    // 忘记 GICC->PMR = 0xFF;
    // → 中断完全不触发
}

// 错误 2: ISENABLER 位运算错误
void enable_irq_wrong(uint32_t irq) {
    GICD->ISENABLER[irq / 32] = (1 << irq);  // 错！应该是 1 << (irq%32)
    // 如果 irq=33, 1<<33 会溢出 32 位
}

// 错误 3: EOIR 在 spurious 时也写
void handle_irq_wrong(void) {
    uint32_t irq = GICC->IAR;
    // 没检查 1023
    GICC->EOIR = irq;  // 如果 irq=1023, EOIR 写了 1023 → GIC 状态混乱
}

// 错误 4: 电平触发先写 EOIR
void timer_isr_wrong(void) {
    GICC->EOIR = 30;  // 先写 EOIR！中断信号仍高
    // 重设 TVAL...
    // → GIC 立即再发中断 → 重入
}

// 正确版本
void gic_init_correct(void) {
    GICD->CTLR = 1;
    GICC->CTLR = 1;
    GICC->PMR = 0xFF;     // 必须！
    GICC->BPR = 0;
}

void enable_irq_correct(uint32_t irq) {
    GICD->ISENABLER[irq / 32] |= (1 << (irq % 32));  // 正确
}

void handle_irq_correct(void) {
    uint32_t irq = GICC->IAR;
    if (irq == 1023) return;  // spurious 不写 EOIR
    // ... ISR ...
    GICC->EOIR = irq;
}

void timer_isr_correct(void) {
    // 先清中断源（重设 TVAL）
    asm volatile("msr cntp_tval_el0, %0" :: "r"(tval));
    // 再处理逻辑
    handle_timer_event();
    // EOIR 在外层统一写
}
```

### GIC 自检函数

```c
// 启动后验证 GIC 是否正常工作
void gic_self_test(void) {
    // 触发 SGI 0（软件中断）
    // GICD_SGIR: bit[25:24]=目标CPU过滤, bit[3:0]=SGI号
    *(volatile uint32_t *)(GICD_BASE + 0xF00) = (1 << 24) | 0;

    // 等待中断到达
    uint32_t irq = GICC->IAR;
    if (irq == 0) {
        uart_puts("GIC self-test: PASS (SGI 0 received)\n");
        GICC->EOIR = 0;
    } else {
        uart_puts("GIC self-test: FAIL (got irq=");
        uart_hex(irq);
        uart_puts(")\n");
    }
}
```

## HFT 关联

GIC 配置错误在 HFT 系统中通常是灾难性的——中断不工作意味着定时器、网卡等关键外设全部失效。建议写一个 GIC 自检函数：初始化后主动触发一个 SGI（软件中断），验证 IAR 能读到正确中断号 + EOIR 写入后 GIC 状态正常。在 HFT 系统上线前，应该测试所有使用的中断号是否正确使能、优先级和目标 CPU 是否正确配置。

## 自测题

1. **GIC 初始化后中断完全不工作，列出 3 个最可能的原因。**

<details>
<summary>答案</summary>

1. **PMR=0**：默认屏蔽所有中断，必须设 `GICC_PMR=0xFF`
2. **ITARGETSR=0**：目标 CPU 默认为 0，中断不知道送给哪个核
3. **ISENABLER 未设**：对应中断号的使能位没设
4. （附加）GICD_CTLR 或 GICC_CTLR 未使能
</details>

2. **中断号 33 的 ISENABLER 和 IPRIORITYR 分别怎么索引？**

<details>
<summary>答案</summary>

- ISENABLER：`ISENABLER[33/32]` = `ISENABLER[1]`，位 = `1 << (33%32)` = `1 << 1` = bit1
- IPRIORITYR：`IPRIORITYR[33]`（每中断一字节，直接索引）

注意：33/32=1（第 1 组），不是第 33 组。
</details>

3. **Pi5 上运行原书 GICv2 代码会怎样？能正常工作吗？**

<details>
<summary>答案</summary>

**不能正常工作**。Pi5 使用 GIC-600（GICv3），GICv2 代码中的 MMIO 地址（GICD=0xFF841000, GICC=0xFF842000）在 Pi5 上不对应 GIC 寄存器，读写这些地址要么无响应要么访问到错误外设。必须改为 GICv3 接口：GICR 初始化 + ICC_*_EL1 系统寄存器。
</details>

4. **为什么 spurious interrupt (1023) 不能写 EOIR？写了会怎样？**

<details>
<summary>答案</summary>

Spurious interrupt 没有对应的中断号，GIC 内部没有将其标记为 Active。如果向 EOIR 写 1023，GIC 尝试结束一个不存在的中断 → 内部状态机错乱 → 后续中断处理异常（可能丢中断、送错误中断号、或 GIC 卡死）。

正确处理：检查 IAR 返回值，1023 时直接 return，不写 EOIR。
</details>

## 参考与延伸

- [§13.3 GIC 初始化流程](03-gic-init.md) — 避免初始化错误
- [§13.2 关键寄存器](02-gic-registers.md) — 寄存器分组规则
- [§13.4 中断处理流程](04-irq-flow.md) — IAR/EOIR 配对规则
- [§13.6 GICv2 vs GICv3](06-gicv2-vs-gicv3.md) — Pi5 适配详解
