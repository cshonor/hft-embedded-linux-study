# §12.5 中断控制器演进

> **来源：** [Ch12 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

不同平台使用不同版本的 GIC：Pi4B 用 GICv2（GIC-400），Pi5 用 GICv3（GIC-600），QEMU virt 默认 GICv3。原书 GICv2 代码不能直接用在 Pi5 上。

## 核心要点

| 平台 | 中断控制器 | 版本 |
|------|-----------|------|
| Pi4B (BCM2711) | GIC-400 | **GICv2** |
| Pi5 (BCM2712) | GIC-600 | **GICv3** |
| QEMU `-M virt` | 默认 | **GICv3**（可配置 GICv2） |

### 版本差异概览

| 特性 | GICv2 | GICv3 |
|------|-------|-------|
| 组件 | Distributor + CPU Interface | Distributor + Redistributor + CPU Interface |
| 寄存器访问 | 纯 MMIO | MMIO + 系统寄存器（ICC_*_EL1） |
| 中断确认 | 读 GICC_IAR | 读 ICC_IAR1_EL1 |
| MSI | 不支持 | 支持 |
| 亲和性 | GICD_ITARGETSR | GICR（每核独立） |

> **Pi5 适配坑**：原书 GICv2 代码（寄存器映射、初始化流程）不能直接用在 Pi5 上。
> 建议先在 QEMU `virt`（支持 GICv3）上做实验，再上 Pi5。详见 Ch13。

## HFT 关联

GICv3 的 Redistributor 架构对多核 HFT 系统更友好——每个核有独立的 Redistributor，中断亲和性设置不需要全局锁。但 GICv3 的初始化流程更复杂，需要额外配置 Redistributor。在 Pi5 上做 HFT 开发时，建议先用 QEMU `virt -machine virt,gic-version=3` 验证 GICv3 代码，再迁移到 Pi5。GICv3 的系统寄存器模式（ICC_*_EL1）比 MMIO 模式更快——读写系统寄存器不需要总线访问。

## 自测题

1. **Pi4B 和 Pi5 分别使用哪个版本的 GIC？**

<details>
<summary>答案</summary>

Pi4B（BCM2711）使用 **GIC-400（GICv2）**。Pi5（BCM2712）使用 **GIC-600（GICv3）**。
</details>

2. **GICv3 相比 GICv2 多了什么组件？有什么好处？**

<details>
<summary>答案</summary>

多了 **Redistributor（GICR）**，每个核一个。好处：中断亲和性（路由到哪个 CPU）在 Redistributor 中配置，**不需要全局锁**，多核并发性更好。GICv2 的亲和性在 Distributor 的 ITARGETSR 中集中配置，多核竞争。
</details>

3. **QEMU 上如何指定使用 GICv2 还是 GICv3？**

<details>
<summary>答案</summary>

用 `-machine virt,gic-version=2` 或 `-machine virt,gic-version=3`。默认是 GICv3。在 GICv2 模式下可以用原书的 GICv2 代码做实验。
</details>

## 参考与延伸

- [§12.1 中断处理全流程](01-interrupt-flow.md) — GIC 在流程中的角色
- [Ch13 GIC-V2](../../chapter-13-gic-v2/notes/section-0-本章完整概述.md) — GICv2 的详细架构和寄存器
- [§13.6 GICv2 vs GICv3](../../chapter-13-gic-v2/notes/section-0-本章完整概述.md) — 两版本的详细对照
