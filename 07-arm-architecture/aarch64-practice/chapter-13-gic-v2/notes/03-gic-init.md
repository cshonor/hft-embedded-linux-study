# §13.3 GIC 初始化流程

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

GICv2 的完整初始化流程：使能 Distributor → 使能 CPU Interface → 设 PMR → 使能特定中断 → 设目标 CPU → 设优先级 → 设触发类型。每一步都不可遗漏，否则中断无法正常工作。

## 核心要点

### 初始化代码

```c
// GICv2 寄存器结构体定义
typedef struct {
    uint32_t CTLR;          // 0x000
    uint32_t TYPER;        // 0x004
    uint32_t padding[62];  // 0x008-0x0FC
    uint32_t ISENABLER[32]; // 0x100
    // ... 其他寄存器
    uint8_t  IPRIORITYR[1020]; // 0x400
    uint8_t  ITARGETSR[256];   // 0x800 (每4中断一组, 每中断8位)
    uint32_t ICFGR[64];        // 0xC00
    uint32_t SGIR;             // 0xF00
} gicd_t;

typedef struct {
    uint32_t CTLR;  // 0x000
    uint32_t PMR;   // 0x004
    uint32_t BPR;   // 0x008
    uint32_t IAR;   // 0x00C
    uint32_t EOIR;  // 0x010
} gicc_t;

#define GICD_BASE 0x08000000
#define GICC_BASE 0x08010000
#define GICD ((volatile gicd_t *)GICD_BASE)
#define GICC ((volatile gicc_t *)GICC_BASE)

void gic_init(void) {
    // 1. 使能 Distributor（全局中断分发）
    GICD->CTLR = 1;

    // 2. 使能 CPU Interface（当前核中断接口）
    GICC->CTLR = 1;

    // 3. 设置优先级掩码（允许所有优先级）
    //    PMR 默认=0 会屏蔽所有中断，必须设
    GICC->PMR = 0xFF;

    // 4. 设置优先级分组（允许所有优先级嵌套）
    GICC->BPR = 0;  // 0 = 不分组，允许抢占

    // --- 以下针对每个需要使用的中断号重复设置 ---

    // 5. 使能特定中断（如定时器 IRQ=30）
    GICD->ISENABLER[30/32] |= (1 << (30 % 32));

    // 6. 设置中断目标 CPU（CPU0）
    //    ITARGETSR 每 4 个中断一组，每中断 8 位
    //    bit0 = CPU0, bit1 = CPU1, ...
    GICD->ITARGETSR[30/4] |= (1 << (8 * (30 % 4) + 0));

    // 7. 设置优先级（0=最高, 0xFF=最低）
    GICD->IPRIORITYR[30] = 0xA0;  // 中等优先级

    // 8. 设置触发类型（电平触发，定时器默认）
    //    ICFGR 每 16 个中断一组，每中断 2 位
    //    0b00 = 电平触发, 0b10 = 边沿触发
    GICD->ICFGR[30/16] &= ~(0x3 << (2 * (30 % 16))); // 清除
    // 定时器用电平触发（0b00），不需要额外设
}
```

### 初始化步骤总结

| 步骤 | 寄存器 | 作用 | 必须性 | 默认值 |
|------|--------|------|--------|--------|
| 1 | GICD_CTLR=1 | 全局使能 Distributor | 必须 | 0（禁用） |
| 2 | GICC_CTLR=1 | 使能 CPU Interface | 必须 | 0（禁用） |
| 3 | GICC_PMR=0xFF | 允许所有优先级 | **必须** | 0（屏蔽一切） |
| 4 | GICC_BPR=0 | 优先级不分组 | 可选 | 0（默认不分组） |
| 5 | GICD_ISENABLER | 使能特定中断号 | 按需 | 0（全禁止） |
| 6 | GICD_ITARGETSR | 设置目标 CPU | **必须** | 0（无目标） |
| 7 | GICD_IPRIORITYR | 设置优先级 | 可选 | 0（最高） |
| 8 | GICD_ICFGR | 设置触发类型 | 可选 | 0（电平） |

> **最常见的初始化失败**：忘记步骤 3（PMR=0 屏蔽所有）或步骤 6（ITARGETSR=0 无目标 CPU），导致中断无法到达 CPU。

### 批量初始化多个中断

```c
// 批量初始化 UART(33) + GPIO(34) 中断
void gic_init_peripherals(void) {
    // 使能 IRQ 33 (UART) 和 IRQ 34 (GPIO)
    GICD->ISENABLER[33/32] |= (1 << (33 % 32)) | (1 << (34 % 32));

    // 设目标 CPU0
    GICD->ITARGETSR[33/4] |= (1 << (8 * (33 % 4)));  // IRQ 33 → CPU0
    GICD->ITARGETSR[34/4] |= (1 << (8 * (34 % 4)));  // IRQ 34 → CPU0

    // 设优先级
    GICD->IPRIORITYR[33] = 0xB0;  // UART 低优先级
    GICD->IPRIORITYR[34] = 0x90;  // GPIO 较高优先级

    // 设触发类型
    GICD->ICFGR[33/16] |= (0x2 << (2 * (33 % 16)));  // UART 边沿
    // GPIO 用电平触发，保持默认 0b00
}
```

### 多核 GIC 初始化

```c
// 在每个 CPU 核上执行（GICC 是每核独立的）
void gic_init_cpu(void) {
    // GICD 全局只需初始化一次（CPU0 做）
    // GICC 每核必须初始化（每个核自己做）
    GICC->CTLR = 1;       // 使能本核 CPU Interface
    GICC->PMR = 0xFF;     // 允许所有优先级
    GICC->BPR = 0;        // 不分组
}
```

> GICD 的初始化（CTLR、ISENABLER、ITARGETSR 等）只需在 CPU0 上做一次。
> GICC 的初始化（CTLR、PMR）必须每个核都做——因为 CPU Interface 是每核独立的。

## HFT 关联

GIC 初始化是裸金属 HFT 系统的第一步——没有正确初始化的 GIC，任何外设中断都无法工作。在 HFT 多核系统中，步骤 6（ITARGETSR）特别重要：将定时器中断绑定到交易核，将网卡中断绑定到管理核，实现中断隔离。步骤 7（优先级）可以用来确保交易核的定时器中断优先级高于其他中断。初始化代码应尽量简短，在系统启动早期完成。

GICC_BPR=0 允许所有优先级中断嵌套，在 HFT 中可以确保高优先级定时器中断能抢占低优先级网卡中断。

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
// 即 ITARGETSR[7] 的 bit[23:16] 区域，bit16=CPU0
```

如果 ITARGETSR 默认为 0，中断不知道送给哪个 CPU，永远不会触发。
</details>

3. **GICD_CTLR 和 GICC_CTLR 分别使能什么？能只使能一个吗？多核时谁初始化 GICD？**

<details>
<summary>答案</summary>

- **GICD_CTLR**：使能 Distributor（全局中断分发功能）
- **GICC_CTLR**：使能当前核的 CPU Interface（中断送达当前核）

**两个都必须使能**。只使能 GICD → 中断在 Distributor 中处理但不会送到 CPU Interface。只使能 GICC → 没有中断从 Distributor 送来。两者缺一不可。

多核时：GICD 全局只需在 **CPU0** 上初始化一次（ISENABLER、ITARGETSR 等都是全局的）。GICC 必须**每个核**各自初始化（CTLR、PMR 是每核独立的）。
</details>

4. **为什么 ITARGETSR 默认为 0 会导致中断不工作？**

<details>
<summary>答案</summary>

ITARGETSR 的 8 位中每一位对应一个 CPU 核（bit0=CPU0, bit1=CPU1...）。默认值 0 表示该中断**没有目标 CPU**——Distributor 不知道该把中断送给谁，所以不会发送。必须至少设一位（如 bit0=1 表示路由到 CPU0）。

注意：ITARGETSR 在 GICv2 中是**只读**的（硬件在复位时根据 GICD_TYPER 设置默认值），在 QEMU 上可能默认路由到 CPU0。但在真实硬件上不能依赖默认值，必须显式设置。
</details>

## 参考与延伸

- [§13.2 关键寄存器](02-gic-registers.md) — 初始化涉及的寄存器详解
- [§13.4 中断处理流程](04-irq-flow.md) — 初始化后如何处理中断
- [§13.8 易错点](08-pitfalls.md) — 初始化常见错误
- [§13.7 实验要点](07-lab.md) — 实验 13-1 的完整初始化代码
