# §13.2 关键寄存器

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GICv2 的 Distributor（GICD）和 CPU Interface（GICC）的关键寄存器：CTLR（控制）、ISENABLER（使能）、IPRIORITYR（优先级）、ITARGETSR（目标 CPU）、IAR（确认）、EOIR（结束）。掌握这些寄存器的偏移地址和索引计算是编写 GIC 驱动代码的基础。

## 核心要点

### Distributor (GICD) 寄存器表

| 偏移 | 寄存器 | 作用 | 读写 |
|------|--------|------|------|
| 0x000 | GICD_CTLR | Distributor 全局控制（bit0=使能） | R/W |
| 0x004 | GICD_TYPER | 中断数量、类型信息（只读） | RO |
| 0x100 | GICD_ISENABLERn | 中断使能（写 1 使能，每 32 个一组） | WO |
| 0x180 | GICD_ICENABLERn | 中断禁止（写 1 禁止，每 32 个一组） | WO |
| 0x200 | GICD_ISPENDRn | 中断 pending 状态读取/设置 | R/W |
| 0x280 | GICD_ICPENDRn | 中断 pending 清除 | WO |
| 0x300 | GICD_ISACTIVERn | 中断 active 状态读取/设置 | R/W |
| 0x380 | GICD_ICACTIVERn | 中断 active 状态清除 | WO |
| 0x400 | GICD_IPRIORITYRn | 优先级设置（每中断 1 字节） | R/W |
| 0x800 | GICD_ITARGETSRn | 目标 CPU 设置（每中断 8 位） | R/W |
| 0xC00 | GICD_ICFGRn | 中断触发类型（每中断 2 位） | R/W |
| 0xF00 | GICD_SGIR | 软件中断生成（写此寄存器发 SGI） | WO |

### CPU Interface (GICC) 寄存器表

| 偏移 | 寄存器 | 作用 | 读写 |
|------|--------|------|------|
| 0x000 | GICC_CTLR | CPU Interface 使能（bit0=使能） | R/W |
| 0x004 | GICC_PMR | 优先级掩码（低于此值的中断不送 CPU） | R/W |
| 0x008 | GICC_BPR | 优先级分组（控制抢占） | R/W |
| 0x00C | **GICC_IAR** | **中断确认** → 读此寄存器获取中断号 | RO |
| 0x010 | **GICC_EOIR** | **结束中断** → 写中断号通知 GIC 处理完毕 | WO |
| 0x014 | GICC_RPR | 运行优先级（当前正在处理的中断优先级） | RO |
| 0x018 | GICC_HPPIR | 最高优先级 pending 中断号 | RO |

### 寄存器分组规则（重点）

| 寄存器 | 分组大小 | 每中断占用 | 索引计算 | 位/字节定位 |
|--------|----------|-----------|----------|------------|
| ISENABLER | 32 中断/组 | 1 位 | `ISENABLER[irq/32]` | `1 << (irq%32)` |
| ICENABLER | 32 中断/组 | 1 位 | `ICENABLER[irq/32]` | `1 << (irq%32)` |
| IPRIORITYR | 1 中断/字节 | 1 字节 | `IPRIORITYR[irq]` | 直接写字节 |
| ITARGETSR | 4 中断/组 | 8 位 | `ITARGETSR[irq/4]` | `8*(irq%4)` 位域 |
| ICFGR | 16 中断/组 | 2 位 | `ICFGR[irq/16]` | `2*(irq%16)` 位域 |

### 索引计算示例（中断号 33）

```c
// ISENABLER: 33/32 = 1, 33%32 = 1 → ISENABLER[1] bit1
GICD->ISENABLER[33/32] = (1 << (33 % 32));  // ISENABLER[1] |= 0x2

// IPRIORITYR: 直接索引 → IPRIORITYR[33]
GICD->IPRIORITYR[33] = 0xA0;  // 优先级 0xA0

// ITARGETSR: 33/4 = 8, 33%4 = 1 → ITARGETSR[8] bit[15:8]
GICD->ITARGETSR[33/4] |= (1 << (8 * (33 % 4) + 0));  // bit8 = CPU0

// ICFGR: 33/16 = 2, 33%16 = 1 → ICFGR[2] bit[3:2]
GICD->ICFGR[33/16] |= (0x2 << (2 * (33 % 16)));  // bit[3:2] = 0b10 = 边沿
```

### GICD_TYPER（中断数量探测）

| 位 | 字段 | 说明 |
|----|------|------|
| [4:0] | ITLinesNumber | 支持 SPI 数 = 32 × (ITLinesNumber+1) |
| [7] | SecurityExtn | 是否支持安全扩展 |
| [10:8] | CPUNumber | CPU 接口数量 - 1 |

> 运行时可通过 `GICD_TYPER` 动态获取支持的中断数量，而不是硬编码。

### GICC_IAR 返回值

| 返回值 | 含义 | 处理方式 |
|--------|------|----------|
| 0-1010 | 有效中断号 | 分发到 ISR，处理后写 EOIR |
| 1023 | Spurious interrupt（伪中断） | **不写 EOIR**，直接返回 |

## HFT 关联

GICC_PMR（优先级掩码）是 HFT 中断优先级控制的关键——设置高 PMR 值可以只允许高优先级中断到达 CPU，屏蔽低优先级中断的干扰。在交易关键路径上，可以临时提高 PMR 屏蔽网卡中断，完成后恢复。ITARGETSR 控制中断路由到哪个核，HFT 中通常将管理中断路由到非交易核。IAR/EOIR 的原子性保证了中断确认不会竞争。

GICC_BPR 控制优先级分组——在 HFT 中可以设为 0（不分组），允许所有不同优先级的中断嵌套，确保高优先级中断（如定时器）能抢占低优先级中断。

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

PMR（Priority Mask Register）设置优先级掩码——优先级数值 >= PMR 的中断才送到 CPU。默认值 0 会屏蔽**所有中断**。必须设置 PMR=0xFF（允许所有优先级）才能收到中断。
</details>

4. **中断号 33 的 IPRIORITYR 和 ICFGR 分别怎么索引？**

<details>
<summary>答案</summary>

- IPRIORITYR：`IPRIORITYR[33]`（每中断一字节，直接索引，偏移 = 0x400 + 33）
- ICFGR：`ICFGR[33/16]` = `ICFGR[2]`，位域 = `2*(33%16)` = `2*1` = bit[3:2]

33/16=2（第 2 组），33%16=1（第 1 个 2 位槽）。
</details>

## 参考与延伸

- [§13.1 GIC 分层架构](01-gic-architecture.md) — 这些寄存器属于哪个组件
- [§13.3 GIC 初始化流程](03-gic-init.md) — 如何配置这些寄存器
- [§13.4 中断处理流程](04-irq-flow.md) — IAR/EOIR 在中断处理中的使用
- [§13.8 易错点](08-pitfalls.md) — 寄存器索引计算常见错误
