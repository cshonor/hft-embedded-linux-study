# §12.5 中断控制器演进

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

不同平台使用不同版本的 GIC：Pi4B 用 GICv2（GIC-400），Pi5 用 GICv3（GIC-600），QEMU virt 默认 GICv3。原书 GICv2 代码不能直接用在 Pi5 上。

## 平台与 GIC 版本

| 平台 | SoC | 中断控制器 | GIC 版本 |
|------|-----|-----------|---------|
| Pi4B | BCM2711 | GIC-400 | **GICv2** |
| Pi5 | BCM2712 | GIC-600 | **GICv3** |
| QEMU `-M virt` | - | 可配置 | **GICv3**（默认）/ GICv2 |
| AWS Graviton | - | - | GICv3 |
| Ampere Altra | - | - | GICv3 |

## GICv2 vs GICv3 对比

| 特性 | GICv2 | GICv3 |
|------|-------|-------|
| 架构 | Distributor + CPU Interface | Distributor + Redistributor + CPU Interface |
| 寄存器访问 | 纯 MMIO | MMIO + 系统寄存器（ICC_*_EL1） |
| 中断确认 | 读 GICC_IAR (MMIO) | 读 ICC_IAR1_EL1 (系统寄存器) |
| 中断结束 | 写 GICC_EOIR (MMIO) | 写 ICC_EOIR1_EL1 (系统寄存器) |
| 中断数 | 最多 1020 | 最多 1M+（ESPI 扩展） |
| MSI | ✗ 不支持 | ✓ 支持 |
| 亲和性 | GICD_ITARGETSR (集中) | GICR (每核独立) |
| LPI（大数量中断） | ✗ | ✓ (ITS) |
| 虚拟化 | 硬件有限支持 | 完整 EL2 支持 |

## GICv3 的 Redistributor

```
GICv2 架构：
  ┌──────────────┐
  │ Distributor  │ ← 中断源、优先级、亲和性（集中）
  │  (GICD)      │
  └──┬───┬───┬───┘
     │   │   │
  ┌─┴─┬─┴─┬─┴──┐
  │CPU0│CPU1│CPU2│ CPU Interface（集中映射到每个核）
  └────┘────┘────┘

GICv3 架构：
  ┌──────────────┐
  │ Distributor  │ ← 中断源、优先级（SPI 部分）
  │  (GICD)      │
  └──┬───┬───┬───┘
     │   │   │
  ┌─┴─┬─┴─┬─┴──┐
  │RD0│RD1│RD2 │ Redistributor（每核独立）
  │   │   │   │ ← PPI/SGI 亲和性在这里
  └─┬─└─┬─└─┬──┘
    │   │   │
  ┌─┴─┬─┴─┬─┴──┐
  │CI0│CI1│CI2 │ CPU Interface（系统寄存器）
  └───┘───┘───┘
```

| 组件 | GICv3 角色 | MMIO 地址 |
|------|----------|----------|
| GICD (Distributor) | SPI 中断管理 | 基址 + 0x0 |
| GICR (Redistributor) | PPI/SGI + LPI 管理 | 每核基址 + offset |
| CPU Interface | IAR/EOIR/PMR | 系统寄存器（ICC_*_EL1） |

## 系统寄存器模式（GICv3）

```asm
// GICv3 中断确认（系统寄存器）
mrs x0, ICC_IAR1_EL1    // 读中断号
// x0 = 中断号

// GICv3 中断结束
msr ICC_EOIR1_EL1, x0   // 写回中断号

// 设置优先级掩码
msr ICC_PMR_EL1, #0xff  // 允许所有优先级

// 使能 CPU Interface
msr ICC_CTLR_EL1, x0    // 配置
msr ICC_IGRPEN1_EL1, #1 // 使能 group 1 中断
```

| GICv2 MMIO | GICv3 系统寄存器 |
|-----------|-----------------|
| GICC_IAR | ICC_IAR1_EL1 |
| GICC_EOIR | ICC_EOIR1_EL1 |
| GICC_PMR | ICC_PMR_EL1 |
| GICC_CTLR | ICC_CTLR_EL1 |

## Pi5 适配坑

原书 GICv2 代码不能直接用在 Pi5 上。主要差异：

| 差异点 | GICv2 (Pi4) | GICv3 (Pi5) |
|--------|------------|------------|
| 基址 | GICC_BASE + offset | GICD_BASE + GICR_BASE |
| 中断确认 | `ldr w0, [gicc, #IAR]` | `mrs x0, ICC_IAR1_EL1` |
| 中断结束 | `str w0, [gicc, #EOIR]` | `msr ICC_EOIR1_EL1, x0` |
| 初始化 | 配置 GICC + GICD | 配置 GICD + 每核 GICR + ICC_*_EL1 |
| 亲和性 | GICD_ITARGETSR | GICR的 GICR_ISENABLER |

## HFT 关联

GICv3 的 Redistributor 架构对多核 HFT 系统更友好——每个核有独立的 Redistributor，中断亲和性设置不需要全局锁。但 GICv3 的初始化流程更复杂，需要额外配置 Redistributor。在 Pi5 上做 HFT 开发时，建议先用 QEMU `virt -machine virt,gic-version=3` 验证 GICv3 代码，再迁移到 Pi5。GICv3 的系统寄存器模式（ICC_*_EL1）比 MMIO 模式更快——读写系统寄存器不需要总线访问。

## 自测题

1. **Pi4B 和 Pi5 分别使用哪个版本的 GIC？**
<details><summary>答案</summary>
Pi4B（BCM2711）使用 **GIC-400（GICv2）**。Pi5（BCM2712）使用 **GIC-600（GICv3）**。QEMU `-M virt` 默认 GICv3，可用 `gic-version=2` 指定 GICv2。
</details>

2. **GICv3 相比 GICv2 多了什么组件？有什么好处？**
<details><summary>答案</summary>
多了 **Redistributor（GICR）**，每个核一个。好处：PPI/SGI 的亲和性在每核的 Redistributor 中配置，**不需要全局锁**，多核并发性更好。GICv2 的亲和性在 Distributor 的 ITARGETSR 中集中配置，多核竞争。另外 GICv3 支持 LPI（大数量中断）和 MSI。
</details>

3. **QEMU 上如何指定使用 GICv2 还是 GICv3？**
<details><summary>答案</summary>
`-machine virt,gic-version=2` 或 `-machine virt,gic-version=3`。默认是 GICv3。在 GICv2 模式下可以用原书的 GICv2 代码做实验。`gic-version=host` 用主机的 GIC（需要 ARM 主机）。
</details>

4. **GICv3 用系统寄存器（ICC_IAR1_EL1）确认中断比 GICv2 的 MMIO（GICC_IAR）有什么延迟优势？**
<details><summary>答案</summary>
系统寄存器访问（MRS）是 CPU 内部操作，延迟固定且低。MMIO 访问需要经过总线（AXI/APB）往返到 GIC 外设 → 延迟更高且不确定（总线仲裁、其他 master 竞争）。在 A76 上 MRS ~1-2 cycles，MMIO ~20-50 cycles。每次中断确认/结束可省几十纳秒。
</details>

5. **GICv3 的 Redistributor 有什么独特的初始化步骤？**
<details><summary>答案</summary>
（1）每个核需要初始化自己的 Redistributor（GICR）（2）检查 GICR_TYPER 确定 PPI/SGI 基址（3）等待 GICR_WAKER.ProcessingSleep=0（唤醒 Redistributor）（4）配置 GICR_ISENABLER 使能 PPI/SGI（5）配置 GICR_ICENABLER 禁制不需要的。GICv2 不需要这些——PPI/SGI 在 Distributor 集中配置。
</details>

## 参考与延伸

- [§12.1 中断处理全流程](01-interrupt-flow.md) — GIC 在流程中的角色
- [Ch13 GIC-V2](../../chapter-13-gic-v2/notes/section-0-本章完整概述.md) — GICv2 的详细架构和寄存器
- [§13.6 GICv2 vs GICv3](../../chapter-13-gic-v2/notes/section-0-本章完整概述.md) — 两版本的详细对照
