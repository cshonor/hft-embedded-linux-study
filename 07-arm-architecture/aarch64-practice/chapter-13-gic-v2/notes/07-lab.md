# §13.7 实验要点

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章 2 个实验：通用定时器中断（GICv2 完整流程）和树莓派系统定时器。从 QEMU 验证到 Pi4B 实机，Pi5 需改 GICv3 接口。

## 核心要点

| 实验 | 内容 | 平台 | Pi5 适配 |
|------|------|------|----------|
| 13-1 | 通用定时器中断（GICv2 流程） | QEMU | QEMU 可配 GICv2 |
| 13-2 | 树莓派系统定时器 | Pi4B | Pi5=GICv3，需改接口 |

### 实验 13-1 关键步骤

1. GIC 初始化（GICD CTLR + GICC CTLR + PMR）
2. 使能定时器中断（GICD ISENABLER + ITARGETSR + IPRIORITYR）
3. 设置通用定时器（CNTP_TVAL + CNTP_CTL）
4. 编写 gic_handle_irq：IAR → switch → EOIR
5. ISR 中重设 TVAL + 清中断状态

### 实验 13-2 Pi5 适配清单

| 原书 (Pi4B/GICv2) | Pi5 (GICv3) |
|-------------------|-------------|
| `GICC->IAR` | `mrs x0, ICC_IAR1_EL1` |
| `GICC->EOIR = irq` | `msr ICC_EOIR1_EL1, x0` |
| GICD+GICC 两步初始化 | GICD+GICR+ICC 三步初始化 |
| ITARGETSR 设目标 CPU | GICR 自动路由 |
| GIC 基址 0xFF841000 | BCM2712 新地址 |

## HFT 关联

实验 13-1 是 HFT 定时器中断的完整实现——从 GIC 初始化到 ISR 处理，每一步都需要正确。建议在 QEMU 上完成后，用 `CNTPCT_EL0` 测量从中断触发到 ISR 执行的时间（中断延迟基准）。实验 13-2 的 Pi5 适配是实际 HFT 部署的必经之路——理解 GICv2→GICv3 的差异才能在 Pi5 上正确使用中断。

## 自测题

1. **实验 13-1 中，GIC 初始化后中断不工作，最可能的原因是什么？**

<details>
<summary>答案</summary>

最可能原因：**忘记设 GICC_PMR=0xFF**（默认 0 屏蔽所有中断）或**忘记设 ITARGETSR**（默认 0 = 无目标 CPU）。其他可能：GICD_CTLR 或 GICC_CTLR 没使能、ISENABLER 没设对应位。
</details>

2. **实验 13-2 在 Pi5 上需要修改哪些代码？**

<details>
<summary>答案</summary>

1. GIC 基址改为 BCM2712 的地址（查数据手册）
2. 中断确认从 `GICC->IAR` 改为 `mrs x0, ICC_IAR1_EL1`
3. 结束中断从 `GICC->EOIR = irq` 改为 `msr ICC_EOIR1_EL1, x0`
4. 初始化流程增加 Redistributor（GICR）配置
5. ITARGETSR 可能不需要（GICv3 的 GICR 自动路由 PPI）
</details>

3. **如何测量中断延迟（从定时器触发到 ISR 开始执行）？**

<details>
<summary>答案</summary>

方法：在设置 CNTP_TVAL 前读 `CNTPCT_EL0` 记录时间 T1。在 ISR 入口（保存完现场后）读 `CNTPCT_EL0` 记录 T2。中断延迟 = T2 - T1（转换为纳秒：`(T2-T1) * 1e9 / CNTFRQ`）。注意：T1 到实际触发之间有 TVAL 倒计时间，需要精确计算。
</details>

## 参考与延伸

- [§13.3 GIC 初始化流程](03-gic-init.md) — 实验 13-1 的初始化代码
- [§13.4 中断处理流程](04-irq-flow.md) — 实验 13-1 的 ISR 代码
- [§13.6 GICv2 vs GICv3](06-gicv2-vs-gicv3.md) — 实验 13-2 的 Pi5 适配
