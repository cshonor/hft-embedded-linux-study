# §13.6 GICv2 vs GICv3 对照

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GICv2（Pi4B 的 GIC-400）和 GICv3（Pi5 的 GIC-600）的详细对照：GICv3 多了 Redistributor，用系统寄存器替代部分 MMIO，支持 MSI。Pi5 适配的最大坑是 GICv3 的初始化流程完全不同。

## 核心要点

### 对照表

| 特性 | GICv2 (GIC-400) | GICv3 (GIC-600) |
|------|-----------------|-----------------|
| 平台 | Pi4B / QEMU virt(可选) | Pi5 / QEMU virt(默认) |
| 最大中断数 | 最多 1020 | 最多 1020+（支持更多，最多 2^24） |
| 亲和性路由 | GICD_ITARGETSR（集中管理） | GICR（Redistributor，每核独立） |
| 寄存器访问 | MMIO（GICD/GICC 基址） | MMIO + 系统寄存器（ICC_*_EL1） |
| 中断确认 | 读 GICC_IAR (MMIO) | 读 ICC_IAR1_EL1 (系统寄存器) |
| 结束中断 | 写 GICC_EOIR (MMIO) | 写 ICC_EOIR1_EL1 (系统寄存器) |
| 优先级掩码 | GICC_PMR (MMIO) | ICC_PMR_EL1 (系统寄存器) |
| MSI | 不支持 | 支持（MSI/MSI-X） |
| LPI 中断 | 不支持 | 支持（基于 ITS 的 LPI） |
| 优先级位数 | 5 位（32 级） | 5-8 位（可配置） |

### 架构变化图

```
GICv2 架构：
  外设 → [GICD Distributor] → [GICC CPU Interface] → CPU
              (全局1个)          (每核1个, MMIO)

GICv3 架构：
  外设 → [GICD Distributor] → [GICR Redistributor] → [ICC 系统寄存器] → CPU
              (全局1个)          (每核1个)              (MRS/MSR)
                                    ↑ 新增组件
```

| 新组件 | 作用 |
|--------|------|
| **Redistributor (GICR)** | 每核一个，管理 PPI/SGI 中断、配置电平/边沿、设置优先级 |
| **ICC_\*_EL1 系统寄存器** | 替代 GICC MMIO，IAR/EOIR/PMR 等通过 MSR/MRS 访问 |
| **ITS (Interrupt Translation Service)** | 可选组件，支持 LPI 中断和 MSI（消息信号中断） |

### Pi5 适配要点

| 项目 | GICv2 (原书) | GICv3 (Pi5) |
|------|-------------|-------------|
| 初始化 | GICD+GICC 两步 | GICD+GICR+ICC 三步 |
| 中断确认 | `GICC->IAR` (MMIO ~80ns) | `mrs x0, ICC_IAR1_EL1` (~20ns) |
| 结束中断 | `GICC->EOIR = irq` | `msr ICC_EOIR1_EL1, x0` |
| 优先级掩码 | `GICC->PMR` | `msr ICC_PMR_EL1, x0` |
| 目标 CPU | `GICD->ITARGETSR` | GICR 自动路由（每核独立管理 PPI/SGI） |
| 触发类型 | `GICD->ICFGR` | GICR_ICFGR（PPI）或 GICD_ICFGR（SPI） |
| GIC 基址 | 0xFF841000 (Pi4B) | BCM2712 新地址（查数据手册） |

### GICv3 初始化流程（对比 GICv2）

```c
// GICv2 初始化（4 步）
void gicv2_init(void) {
    GICD->CTLR = 1;          // 1. 使能 Distributor
    GICC->CTLR = 1;          // 2. 使能 CPU Interface
    GICC->PMR = 0xFF;        // 3. 设优先级掩码
    GICC->BPR = 0;           // 4. 设优先级分组
}

// GICv3 初始化（6 步，多了 Redistributor）
void gicv3_init(void) {
    // 1. 使能 Distributor
    *(volatile uint32_t *)(GICD_BASE + 0x0000) = 1;

    // 2. 使能 Redistributor（每核）
    //    GICR_CTLR，需要 Wakeup 等待
    *(volatile uint32_t *)(GICR_BASE + 0x0000) = 1;
    // 等待 GICR_WAKER.ProcessorSleep = 0

    // 3. 使能系统寄存器访问（ICC_SRE_EL1）
    asm volatile("mrs x0, ICC_SRE_EL1\n"
                 "orr x0, x0, #1\n"      // SRE=1
                 "msr ICC_SRE_EL1, x0\n");

    // 4. 设优先级掩码
    asm volatile("msr ICC_PMR_EL1, %0" :: "r"(0xFF));

    // 5. 使能 CPU Interface（ICC_IGRPEN1_EL1）
    asm volatile("msr ICC_IGRPEN1_EL1, #1");

    // 6. 设优先级分组
    asm volatile("msr ICC_BPR1_EL1, #0");
}
```

### QEMU 测试 GIC 版本

```bash
# GICv2（测试原书代码）
qemu-system-aarch64 -machine virt,gic-version=2 ...

# GICv3（默认，测试 Pi5 适配代码）
qemu-system-aarch64 -machine virt,gic-version=3 ...

# 自动选择（默认 GICv3）
qemu-system-aarch64 -machine virt ...
```

> **Pi5 适配最大坑**：GICv3 多了一层 Redistributor（GICR），初始化流程完全不同。
> 建议：先在 QEMU `virt`（可配 GICv3）上做，再上 Pi5。

## HFT 关联

GICv3 的系统寄存器模式对 HFT 有显著优势——ICC_IAR1_EL1 的读取（MRS 指令）比 GICC_IAR 的 MMIO 读取快 2-3 倍（~20ns vs ~80ns），直接减少中断确认延迟。GICv3 的 Redistributor 让每个核独立管理 PPI 中断，不需要全局锁，多核 HFT 系统中中断配置更高效。如果 HFT 平台支持 GICv3，应优先使用系统寄存器模式而非 MMIO 兼容模式。

GICv3 的 LPI 中断和 ITS 机制对高性能网卡（如 Mellanox）的 MSI-X 中断很重要——每个队列可以有自己的中断号和中断向量，实现精确的中断路由。

## 自测题

1. **GICv3 相比 GICv2 多了哪个组件？它的作用是什么？**

<details>
<summary>答案</summary>

多了 **Redistributor（GICR）**，每个 CPU 核一个。作用：管理该核的 PPI/SGI 中断（使能、优先级、触发类型），替代 GICv2 中 Distributor 集中管理 PPI/SGI 的方式。SPI 中断仍由 Distributor 管理。
</details>

2. **GICv3 如何读中断号？和 GICv2 有什么区别？延迟差异多大？**

<details>
<summary>答案</summary>

- GICv2：`uint32_t irq = GICC->IAR;`（MMIO 读，~80ns）
- GICv3：`mrs x0, ICC_IAR1_EL1`（系统寄存器读，~20ns）

GICv3 用**系统寄存器**替代 MMIO，快 2-3 倍且不需要知道 GICC 基址。系统寄存器访问不走总线，直接在 CPU 内部完成。
</details>

3. **在 QEMU 上如何分别测试 GICv2 和 GICv3 代码？**

<details>
<summary>答案</summary>

用 `-machine virt,gic-version=2` 测试 GICv2 代码，用 `-machine virt,gic-version=3` 测试 GICv3 代码。默认是 GICv3。建议先在 gic-version=2 上跑通原书代码，再切到 gic-version=3 适配 Pi5。
</details>

4. **GICv3 初始化时为什么要先设 ICC_SRE_EL1？**

<details>
<summary>答案</summary>

ICC_SRE_EL1 的 SRE 位（bit0）控制是否允许使用系统寄存器访问 ICC_*_EL1。SRE=0 时只能用 MMIO 模式（GICv2 兼容），SRE=1 才能用 `mrs/msr` 访问 ICC_IAR1_EL1 等。必须先设 SRE=1，否则后续的系统寄存器操作会触发异常。

设 SRE 后还需要使能 ICC_IGRPEN1_EL1（中断组使能），否则中断不会送到 CPU。
</details>

## 参考与延伸

- [§13.1 GIC 分层架构](01-gic-architecture.md) — GICv2 的两层架构
- [§13.2 关键寄存器](02-gic-registers.md) — GICv2 寄存器详解
- [§13.4 中断处理流程](04-irq-flow.md) — IAR/EOIR 在 GICv3 中的变化
- [§13.7 实验要点](07-lab.md) — Pi5 适配实验
