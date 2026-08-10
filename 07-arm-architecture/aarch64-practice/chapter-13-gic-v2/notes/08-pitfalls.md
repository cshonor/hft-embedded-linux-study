# §13.8 易错点清单

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GICv2 的 5 个常见错误：IAR/EOIR 不配对、忘记设 PMR、中断号除法搞错、目标 CPU 没设、Pi5 用 GICv2 代码。

## 核心要点

| # | 易错点 | 后果 | 修复 |
|---|--------|------|------|
| 1 | IAR/EOIR 不配对 | GIC 卡住，不再送中断 | 确保每个 IAR 都有对应 EOIR |
| 2 | 忘记设 PMR | PMR 默认 0，所有中断被掩蔽 | `GICC_PMR = 0xFF` |
| 3 | 中断号除法搞错 | ISENABLER 每 32 个一组，算错组号 | `ISENABLER[N/32]`，位 = `1<<(N%32)` |
| 4 | 目标 CPU 没设 | ITARGETSR 默认 0，中断不知道送给哪个核 | 初始化时设 ITARGETSR |
| 5 | Pi5 用 GICv2 代码 | 寄存器地址和流程完全不同 | 改用 GICv3 接口 |

### 寄存器分组速查

| 寄存器 | 分组 | 索引 | 位/字节 |
|--------|------|------|---------|
| ISENABLER | 32 中断/组 | `[irq/32]` | `1 << (irq%32)` |
| IPRIORITYR | 1 中断/字节 | `[irq]` | 直接写字节 |
| ITARGETSR | 4 中断/组 | `[irq/4]` | `8*(irq%4)` 位域 |
| ICFGR | 16 中断/组 | `[irq/16]` | `2*(irq%16)` 位域 |

### 调试技巧

- 中断完全不触发 → 检查 PMR、ITARGETSR、ISENABLER
- 中断只触发一次 → 检查 EOIR 是否写入
- 中断号不对 → 验证 ISENABLER/IPRIORITYR 的除法
- Pi5 中断不工作 → 确认 GIC 版本，检查是否误用 GICv2 地址

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

## 参考与延伸

- [§13.3 GIC 初始化流程](03-gic-init.md) — 避免初始化错误
- [§13.2 关键寄存器](02-gic-registers.md) — 寄存器分组规则
- [§13.6 GICv2 vs GICv3](06-gicv2-vs-gicv3.md) — Pi5 适配详解
