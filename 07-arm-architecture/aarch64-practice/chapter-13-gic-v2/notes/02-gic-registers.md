# §13.2 关键寄存器

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GICv2 的 Distributor（GICD）和 CPU Interface（GICC）的关键寄存器：CTLR（控制）、ISENABLER（使能）、IPRIORITYR（优先级）、ITARGETSR（目标 CPU）、IAR（确认）、EOIR（结束）。

## 核心要点

### Distributor (GICD)

| 偏移 | 寄存器 | 作用 |
|------|--------|------|
| 0x000 | GICD_CTLR | Distributor 控制（全局使能） |
| 0x004 | GICD_TYPER | 中断数量、类型信息 |
| 0x100 | GICD_ISENABLERn | 中断使能（每 32 个一组） |
| 0x180 | GICD_ICENABLERn | 中断禁止 |
| 0x400 | GICD_IPRIORITYRn | 优先级设置 |
| 0x800 | GICD_ITARGETSRn | 目标 CPU 设置 |
| 0xC00 | GICD_ICFGRn | 中断触发类型（边沿/电平） |

### CPU Interface (GICC)

| 偏移 | 寄存器 | 作用 |
|------|--------|------|
| 0x000 | GICC_CTLR | CPU 接口使能 |
| 0x004 | GICC_PMR | 优先级掩码（低于此值的中断不送 CPU） |
| 0x00C | **GICC_IAR** | **中断确认** → 读此寄存器获取中断号 |
| 0x010 | **GICC_EOIR** | **结束中断** → 写中断号通知 GIC 处理完毕 |

### 寄存器分组规则

| 寄存器 | 分组 | 索引计算 |
|--------|------|----------|
| ISENABLER | 每 32 个中断一组 | `ISENABLER[irq/32]`，位 = `1 << (irq%32)` |
| IPRIORITYR | 每 1 个中断一字节 | `IPRIORITYR[irq]` |
| ITARGETSR | 每 4 个中断一组（每中断 8 位） | `ITARGETSR[irq/4]`，位域 = `8*(irq%4)` |
| ICFGR | 每 16 个中断一组（每中断 2 位） | `ICFGR[irq/16]`，位域 = `2*(irq%16)` |

## HFT 关联

GICC_PMR（优先级掩码）是 HFT 中断优先级控制的关键——设置高 PMR 值可以只允许高优先级中断到达 CPU，屏蔽低优先级中断的干扰。在交易关键路径上，可以临时提高 PMR 屏蔽网卡中断，完成后恢复。ITARGETSR 控制中断路由到哪个核，HFT 中通常将管理中断路由到非交易核。IAR/EOIR 的原子性保证了中断确认不会竞争。

## 自测题

1. **GICC_IAR 和 GICC_EOIR 分别做什么？它们的关系是什么？**

<details>
<summary>答案</summary>

- **GICC_IAR**（Interrupt Acknowledge Register）：读此寄存器**确认中断**，返回中断号。GIC 标记该中断为"正在处理"
- **GICC_EOIR**（End of Interrupt Register）：写中断号**结束中断**。GIC 释放该中断的优先级

关系：**必须配对使用**。读 IAR 后必须写 EOIR，否则 GIC 认为中断未处理完，不再送同优先级中断。
</details>

2. **如何使能第 30 号中断？写出寄存器和值。**

<details>
<summary>答案</summary>

```c
GICD->ISENABLER[30/32] |= (1 << (30 % 32));
// 即 ISENABLER[0] |= (1 << 30)
```

ISENABLER 每 32 个中断一组，30/32=0（第 0 组），30%32=30（第 30 位）。
</details>

3. **GICC_PMR 的作用是什么？默认值 0 会导致什么问题？**

<details>
<summary>答案</summary>

PMR（Priority Mask Register）设置优先级掩码——**优先级数值低于 PMR 的中断不会送到 CPU**。默认值 0 会屏蔽**所有中断**（所有优先级数值都 >= 0，不"低于"0 但 GICv2 的比较逻辑是 priority >= PMR 才送）。必须设置 PMR=0xFF（允许所有优先级）才能收到中断。
</details>

## 参考与延伸

- [§13.1 GIC 分层架构](01-gic-architecture.md) — 这些寄存器属于哪个组件
- [§13.3 GIC 初始化流程](03-gic-init.md) — 如何配置这些寄存器
- [§13.4 中断处理流程](04-irq-flow.md) — IAR/EOIR 在中断处理中的使用
