# §13.1 GIC 分层架构

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GIC（Generic Interrupt Controller）采用两层架构：Distributor（GICD）全局管理所有中断源，CPU Interface（GICC）每核一个负责确认和结束中断。中断信号从外设 → GICD → GICC → CPU。GIC 是 ARM 系统中连接外设中断和 CPU 的唯一桥梁，理解其架构是掌握中断系统的前提。

## 核心要点

### 两层架构

```
              中断源（定时器/UART/GPIO/...）
                        |
                        v
              +---------------------+
              |  Distributor (GICD)  |  ← 全局唯一：仲裁、优先级、路由到哪个CPU
              +---------------------+
                        |
         +--------------+--------------+
         |              |              |
         v              v              v
  +------------+  +------------+  +------------+
  | GICC (CPU0)|  | GICC (CPU1)|  | GICC (CPU2)|  ← 每核一个
  +------------+  +------------+  +------------+
         |              |              |
         v              v              v
       CPU0            CPU1           CPU2
      IRQ线             IRQ线          IRQ线
```

| 组件 | 简称 | 数量 | 作用 |
|------|------|------|------|
| **Distributor** | GICD | 全局 1 个 | 管理所有中断源：使能、优先级、目标 CPU、分发仲裁 |
| **CPU Interface** | GICC | 每核 1 个 | 确认中断（读 IAR 获取中断号）、结束中断（写 EOIR） |

### Distributor 职责详解

| 职责 | 说明 |
|------|------|
| 中断使能/禁止 | 通过 ISENABLER/ICENABLER 控制每个中断号 |
| 优先级管理 | 通过 IPRIORITYR 设置每个中断的优先级（0=最高, 0xFF=最低） |
| 目标 CPU 路由 | 通过 ITARGETSR 指定中断送到哪个 CPU 核 |
| 触发类型配置 | 通过 ICFGR 设置边沿/电平触发 |
| 优先级仲裁 | 多个中断同时 pending 时，选最高优先级送到 CPU Interface |
| 中断状态机 | Inactive → Pending → Active → Inactive（还可 Active+Pending） |

### CPU Interface 职责详解

| 职责 | 说明 |
|------|------|
| 优先级掩码 | PMR 寄存器：优先级数值 >= PMR 的中断才送 CPU |
| 中断确认 | 读 IAR：返回当前最高优先级中断号，标记为 Active |
| 优先级降级 | 写 EOIR：通知 GIC 中断处理完毕，释放优先级 |
| 抢占控制 | BPR（Binary Point Register）：控制优先级分组，决定能否嵌套 |

### GICv2 寄存器基址

| 平台 | GICD 基址 | GICC 基址 | GIC 版本 |
|------|-----------|-----------|----------|
| QEMU virt | 0x08000000 | 0x08010000 | GICv2（可配 GICv3） |
| Pi4B (BCM2711) | 0xFF841000 | 0xFF842000 | GIC-400 (GICv2) |
| **Pi5 (BCM2712)** | GICv3 架构 | 系统寄存器模式 | GIC-600 (GICv3) |

> QEMU 用 `-machine virt,gic-version=2` 强制 GICv2，默认是 GICv3。

### 中断号分配

| 中断号范围 | 类型 | 全称 | 说明 |
|------------|------|------|------|
| 0-15 | SGI | Software Generated Interrupt | 软件写 GICD_SGIR 触发，用于核间通信（IPI） |
| 16-31 | PPI | Private Peripheral Interrupt | 每个 CPU 核私有，如通用定时器（IRQ 30），不共享 |
| 32+ | SPI | Shared Peripheral Interrupt | 全局共享，如 UART（IRQ 33）、GPIO，路由到指定 CPU |

### 中断状态机

```
                  写 ISENABLER
                      |
                      v
  Inactive -------> Pending -------> Active -------> Inactive
  (未激活)    触发    (等待)   读IAR   (处理中)  写EOIR  (完成)
                 ^                                  |
                 |          读IAR时又有新中断         |
                 +-------- Active + Pending --------+
                            (处理中又来新中断)
```

## HFT 关联

理解 GIC 的两层架构对 HFT 多核系统很重要。Distributor 负责中断路由——将网卡中断绑定到特定 CPU 核（通过 ITARGETSR），避免中断在多核间跳动引入缓存失效延迟。CPU Interface 的 IAR/EOIR 机制保证了中断处理的原子性。在 HFT 系统中，通常将交易 CPU 核的中断屏蔽（或只留高优先级定时器），管理 CPU 核处理网卡/UART 中断，实现隔离。

GIC 的优先级机制可以确保交易核的关键中断（如定时器）优先级最高，不会被低优先级中断（如 UART）抢占。PMR 可以在关键交易路径上临时提高屏蔽阈值，阻止所有非紧急中断。

## 自测题

1. **GICD 和 GICC 分别做什么？它们是一对多的关系吗？**

<details>
<summary>答案</summary>

- **GICD（Distributor）**：全局唯一，管理所有中断源的使能、优先级、目标 CPU 路由
- **GICC（CPU Interface）**：每核一个，负责确认中断（读 IAR 获取中断号）和结束中断（写 EOIR）

是**一对多**：1 个 GICD + N 个 GICC（N = CPU 核数）。
</details>

2. **QEMU virt 上 GICD 和 GICC 的基址分别是多少？如何指定 GIC 版本？**

<details>
<summary>答案</summary>

GICD 基址 = **0x08000000**，GICC 基址 = **0x08010000**。在裸金属代码中用 MMIO 方式读写这些地址。

指定 GIC 版本：`-machine virt,gic-version=2`（GICv2）或 `-machine virt,gic-version=3`（GICv3）。默认是 GICv3。
</details>

3. **SGI、PPI、SPI 三种中断有什么区别？**

<details>
<summary>答案</summary>

- **SGI**（Software Generated Interrupt，0-15）：软件写 GICD_SGIR 触发，用于核间通信
- **PPI**（Private Peripheral Interrupt，16-31）：每个 CPU 核私有，如通用定时器中断，不共享
- **SPI**（Shared Peripheral Interrupt，32+）：全局共享，如 UART/GPIO，路由到指定 CPU
</details>

4. **中断状态有哪些？画出从 Inactive 到 Active+Pending 的转换。**

<details>
<summary>答案</summary>

四个状态：Inactive（未激活）→ Pending（等待处理）→ Active（正在处理）→ Inactive（完成）。

如果读 IAR 时同一个中断号又来了新中断，状态变为 **Active+Pending**（正在处理且又有新中断等待）。写 EOIR 后从 Active+Pending 变为 Pending（如果还有等待的中断）或 Inactive。
</details>

## 参考与延伸

- [§13.2 关键寄存器](02-gic-registers.md) — GICD 和 GICC 的具体寄存器
- [§13.3 GIC 初始化流程](03-gic-init.md) — 如何配置这两层
- [§13.6 GICv2 vs GICv3](06-gicv2-vs-gicv3.md) — GICv3 多了 Redistributor
- [Ch12 §12.1 中断处理流程](../../chapter-12-interrupt-handling/notes/01-interrupt-flow.md) — GIC 在完整中断流程中的位置
