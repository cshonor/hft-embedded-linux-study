# §13.1 GIC 分层架构

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GIC（Generic Interrupt Controller）采用两层架构：Distributor（GICD）全局管理所有中断源，CPU Interface（GICC）每核一个负责确认和结束中断。中断信号从外设 → GICD → GICC → CPU。

## 核心要点

### 两层架构

```
              中断源（定时器/UART/GPIO/...）
                        ↓
              ┌─────────────────────┐
              │  Distributor (GICD)  │  ← 全局仲裁、优先级、路由到哪个CPU
              └─────────────────────┘
                        ↓
              ┌─────────────────────┐
              │  CPU Interface(GICC) │  ← 每核一个：确认中断(IRR)、结束中断(EOR)
              └─────────────────────┘
                        ↓
                    CPU IRQ 线
```

| 组件 | 简称 | 作用 |
|------|------|------|
| **Distributor** | GICD | 管理所有中断源：使能、优先级、目标 CPU、分发 |
| **CPU Interface** | GICC | 每核一个：确认中断、返回中断号、结束中断 |

### GICv2 寄存器基址

| 平台 | GICD 基址 | GICC 基址 |
|------|-----------|-----------|
| QEMU virt | 0x08000000 | 0x08010000 |
| Pi4B (BCM2711) | 0xFF841000 | 0xFF842000 |
| **Pi5 (BCM2712)** | GICv3，地址不同 | — |

### 中断号分配（示例）

| 中断号 | 来源 |
|--------|------|
| 0-15 | SGIs（软件生成中断，核间通信） |
| 16-31 | PPIs（私有外设中断，如定时器） |
| 32+ | SPIs（共享外设中断，如 UART、GPIO） |

## HFT 关联

理解 GIC 的两层架构对 HFT 多核系统很重要。Distributor 负责中断路由——将网卡中断绑定到特定 CPU 核（通过 ITARGETSR），避免中断在多核间跳动引入缓存失效延迟。CPU Interface 的 IAR/EOIR 机制保证了中断处理的原子性。在 HFT 系统中，通常将交易 CPU 核的中断屏蔽（或只留高优先级定时器），管理 CPU 核处理网卡/UART 中断，实现隔离。

## 自测题

1. **GICD 和 GICC 分别做什么？它们是一对多的关系吗？**

<details>
<summary>答案</summary>

- **GICD（Distributor）**：全局唯一，管理所有中断源的使能、优先级、目标 CPU 路由
- **GICC（CPU Interface）**：每核一个，负责确认中断（读 IAR 获取中断号）和结束中断（写 EOIR）

是**一对多**：1 个 GICD + N 个 GICC（N = CPU 核数）。
</details>

2. **QEMU virt 上 GICD 和 GICC 的基址分别是多少？**

<details>
<summary>答案</summary>

GICD 基址 = **0x08000000**，GICC 基址 = **0x08010000**。在裸金属代码中用 MMIO 方式读写这些地址。
</details>

3. **SGI、PPI、SPI 三种中断有什么区别？**

<details>
<summary>答案</summary>

- **SGI**（Software Generated Interrupt，0-15）：软件写 GICD_SGIR 触发，用于核间通信
- **PPI**（Private Peripheral Interrupt，16-31）：每个 CPU 核私有，如通用定时器中断，不共享
- **SPI**（Shared Peripheral Interrupt，32+）：全局共享，如 UART/GPIO，路由到指定 CPU
</details>

## 参考与延伸

- [§13.2 关键寄存器](02-gic-registers.md) — GICD 和 GICC 的具体寄存器
- [§13.3 GIC 初始化流程](03-gic-init.md) — 如何配置这两层
- [§13.6 GICv2 vs GICv3](06-gicv2-vs-gicv3.md) — GICv3 多了 Redistributor
