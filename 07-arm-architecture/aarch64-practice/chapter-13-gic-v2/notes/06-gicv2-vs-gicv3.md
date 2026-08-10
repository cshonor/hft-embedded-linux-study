# §13.6 GICv2 vs GICv3 对照

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GICv2（Pi4B 的 GIC-400）和 GICv3（Pi5 的 GIC-600）的详细对照：GICv3 多了 Redistributor，用系统寄存器替代部分 MMIO，支持 MSI。Pi5 适配的最大坑是 GICv3 的初始化流程完全不同。

## 核心要点

### 对照表

| 特性 | GICv2 (GIC-400) | GICv3 (GIC-600) |
|------|-----------------|-----------------|
| 平台 | Pi4B | Pi5 / QEMU virt |
| 最大中断数 | 最多 1020 | 最多 1020+（支持更多） |
| 亲和性路由 | GICD_ITARGETSR | GICR（Redistributor，每核独立） |
| 寄存器访问 | MMIO（GICD/GICC 基址） | MMIO + 系统寄存器（ICC_*_EL1） |
| 中断确认 | 读 GICC_IAR | 读 ICC_IAR0_EL1 / ICC_IAR1_EL1 |
| 结束中断 | 写 GICC_EOIR | 写 ICC_EOIR0_EL1 / ICC_EOIR1_EL1 |
| MSI | 不支持 | 支持 |

### GICv3 架构变化

```
GICv2:  外设 → GICD → GICC → CPU

GICv3:  外设 → GICD → GICR(每核) → ICC_*_EL1(系统寄存器) → CPU
                    ↑ 新增 Redistributor
```

| 新组件 | 作用 |
|--------|------|
| **Redistributor (GICR)** | 每核一个，管理 PPI/SGI 中断、配置电平/边沿、设置优先级 |
| **ICC_*_EL1 系统寄存器** | 替代 GICC MMIO，IAR/EOIR/PMR 等通过 MSR/MRS 访问 |

### Pi5 适配要点

| 项目 | GICv2 (原书) | GICv3 (Pi5) |
|------|-------------|-------------|
| 初始化 | GICD+GICC 两步 | GICD+GICR+ICC 三步 |
| 中断确认 | `GICC->IAR` (MMIO) | `mrs x0, ICC_IAR1_EL1` (系统寄存器) |
| 结束中断 | `GICC->EOIR = irq` | `msr ICC_EOIR1_EL1, x0` |
| 优先级掩码 | `GICC->PMR` | `msr ICC_PMR_EL1, x0` |
| 目标 CPU | `GICD->ITARGETSR` | GICR 自动路由（每核独立） |

> **Pi5 适配最大坑**：GICv3 多了一层 Redistributor（GICR），初始化流程完全不同。
> 建议：先在 QEMU `virt`（可配 GICv3）上做，再上 Pi5。

## HFT 关联

GICv3 的系统寄存器模式对 HFT 有显著优势——ICC_IAR1_EL1 的读取（MRS 指令）比 GICC_IAR 的 MMIO 读取快 2-3 倍（~20ns vs ~80ns），直接减少中断确认延迟。GICv3 的 Redistributor 让每个核独立管理 PPI 中断，不需要全局锁，多核 HFT 系统中中断配置更高效。如果 HFT 平台支持 GICv3，应优先使用系统寄存器模式而非 MMIO 兼容模式。

## 自测题

1. **GICv3 相比 GICv2 多了哪个组件？它的作用是什么？**

<details>
<summary>答案</summary>

多了 **Redistributor（GICR）**，每个 CPU 核一个。作用：管理该核的 PPI/SGI 中断（使能、优先级、触发类型），替代 GICv2 中 Distributor 集中管理 PPI/SGI 的方式。SPI 中断仍由 Distributor 管理。
</details>

2. **GICv3 如何读中断号？和 GICv2 有什么区别？**

<details>
<summary>答案</summary>

- GICv2：`uint32_t irq = GICC->IAR;`（MMIO 读，~80ns）
- GICv3：`mrs x0, ICC_IAR1_EL1`（系统寄存器读，~20ns）

GICv3 用**系统寄存器**替代 MMIO，更快且不需要知道 GICC 基址。
</details>

3. **在 QEMU 上如何分别测试 GICv2 和 GICv3 代码？**

<details>
<summary>答案</summary>

用 `-machine virt,gic-version=2` 测试 GICv2 代码，用 `-machine virt,gic-version=3` 测试 GICv3 代码。默认是 GICv3。建议先在 gic-version=2 上跑通原书代码，再切到 gic-version=3 适配 Pi5。
</details>

## 参考与延伸

- [§13.1 GIC 分层架构](01-gic-architecture.md) — GICv2 的两层架构
- [§13.2 关键寄存器](02-gic-registers.md) — GICv2 寄存器详解
- [§13.4 中断处理流程](04-irq-flow.md) — IAR/EOIR 在 GICv3 中的变化
