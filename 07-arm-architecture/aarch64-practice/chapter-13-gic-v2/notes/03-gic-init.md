# §13.3 GIC 初始化流程

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GICv2 的完整初始化流程：使能 Distributor → 使能 CPU Interface → 设 PMR → 使能特定中断 → 设目标 CPU → 设优先级。

## 核心要点

### 初始化代码

```c
void gic_init(void) {
    // 1. 使能 Distributor
    GICD->CTLR = 1;

    // 2. 使能 CPU Interface
    GICC->CTLR = 1;

    // 3. 设置优先级掩码（允许所有优先级）
    GICC->PMR = 0xFF;

    // 4. 使能特定中断（如定时器 IRQ=30）
    GICD->ISENABLER[30/32] |= (1 << (30 % 32));

    // 5. 设置中断目标 CPU（CPU0）
    GICD->ITARGETSR[30/4] |= (1 << (8 * (30 % 4) + 0));  // bit0=CPU0

    // 6. 设置优先级
    GICD->IPRIORITYR[30] = 0xA0;  // 优先级 0xA0
}
```

### 初始化步骤

| 步骤 | 寄存器 | 作用 | 必须性 |
|------|--------|------|--------|
| 1 | GICD_CTLR=1 | 全局使能 Distributor | 必须 |
| 2 | GICC_CTLR=1 | 使能 CPU Interface | 必须 |
| 3 | GICC_PMR=0xFF | 允许所有优先级 | **必须**（默认 0 屏蔽一切） |
| 4 | GICD_ISENABLER | 使能特定中断号 | 按需 |
| 5 | GICD_ITARGETSR | 设置目标 CPU | **必须**（默认 0 = 无目标） |
| 6 | GICD_IPRIORITYR | 设置优先级 | 可选（有默认值） |

> **最常见的初始化失败**：忘记步骤 3（PMR=0）或步骤 5（ITARGETSR=0），导致中断无法到达 CPU。

## HFT 关联

GIC 初始化是裸金属 HFT 系统的第一步——没有正确初始化的 GIC，任何外设中断都无法工作。在 HFT 多核系统中，步骤 5（ITARGETSR）特别重要：将定时器中断绑定到交易核，将网卡中断绑定到管理核，实现中断隔离。步骤 6（优先级）可以用来确保交易核的定时器中断优先级高于其他中断。初始化代码应尽量简短，在系统启动早期完成。

## 自测题

1. **GIC 初始化时如果忘记设置 GICC_PMR，会发生什么？**

<details>
<summary>答案</summary>

PMR 默认值 = 0，会**屏蔽所有中断**——没有中断能到达 CPU。必须设置 `GICC_PMR = 0xFF`（允许所有优先级）或适当的掩码值。这是最常见的 GIC 初始化错误。
</details>

2. **如何将中断号 30 路由到 CPU0？写出 ITARGETSR 的设置。**

<details>
<summary>答案</summary>

```c
// ITARGETSR 每 4 个中断一组，每个中断占 8 位
// bit0 = CPU0, bit1 = CPU1, ...
GICD->ITARGETSR[30/4] |= (1 << (8 * (30 % 4) + 0));
// 30/4 = 7（第 7 组），30%4 = 2（第 2 个 8 位槽）
// 即 ITARGETSR[7] 的 bit[16:17] 区域，bit16=CPU0
```

如果 ITARGETSR 默认为 0，中断不知道送给哪个 CPU，永远不会触发。
</details>

3. **GICD_CTLR 和 GICC_CTLR 分别使能什么？能只使能一个吗？**

<details>
<summary>答案</summary>

- **GICD_CTLR**：使能 Distributor（全局中断分发功能）
- **GICC_CTLR**：使能当前核的 CPU Interface（中断送达当前核）

**两个都必须使能**。只使能 GICD → 中断在 Distributor 中处理但不会送到 CPU Interface。只使能 GICC → 没有中断从 Distributor 送来。两者缺一不可。
</details>

## 参考与延伸

- [§13.2 关键寄存器](02-gic-registers.md) — 初始化涉及的寄存器详解
- [§13.4 中断处理流程](04-irq-flow.md) — 初始化后如何处理中断
- [§13.8 易错点](08-pitfalls.md) — 初始化常见错误
