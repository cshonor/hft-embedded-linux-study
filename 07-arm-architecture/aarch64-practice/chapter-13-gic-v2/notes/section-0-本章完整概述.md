# Ch13 完整总结 · GIC-V2 中断控制器

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

GIC（Generic Interrupt Controller）是 ARM 的中断控制器。本章讲 GICv2（Pi4B 的 GIC-400）。理解 GIC 的分层架构和中断路由后，才能配置中断源、优先级、分发。

> **Pi5 适配**：Pi5 用 GIC-600(GICv3)，寄存器和流程不同。但 GICv2 的概念（分发器/CPU接口/中断号）在 GICv3 中仍然适用。

---

## 13.1 GIC 分层架构 ⭐

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

---

## 13.2 关键寄存器 ⭐

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

---

## 13.3 GIC 初始化流程

```c
void gic_init(void) {
    // 1. 使能 Distributor
    GICD->CTLR = 1;

    // 2. 使能 CPU Interface
    GICC->CTLR = 1;

    // 3. 设置优先级掩码（允许所有优先级）
    GICC->PMR = 0xFF;

    // 4. 使能特定中断（如定时器 IRQ=30）
    GICD->ISENABLER[30/32] |= (1 << (30 % 32));

    // 5. 设置中断目标 CPU（CPU0）
    GICD->ITARGETSR[30/4] |= (1 << (8 * (30 % 4) + 0));  // bit0=CPU0

    // 6. 设置优先级
    GICD->IPRIORITYR[30] = 0xA0;  // 优先级 0xA0
}
```

---

## 13.4 中断处理流程 ⭐

```c
void gic_handle_irq(void) {
    // 1. 读 IAR 获取中断号
    uint32_t irq = GICC->IAR;

    // 2. 根据 irq 分发到 ISR
    switch (irq) {
        case 30:  // 通用定时器
            timer_isr();
            break;
        case 33:  // UART
            uart_isr();
            break;
        default:
            // 未知中断
            break;
    }

    // 3. 写 EOIR 结束中断
    GICC->EOIR = irq;
}
```

> **IAR 和 EOIR 必须配对**：读 IAR 后 GIC 认为该中断正在处理，直到写 EOIR。  
> 不写 EOIR → 该优先级被占住，同优先级的中断不再送来。

---

## 13.5 中断触发类型

| 类型 | 说明 | GICD_ICFGR 值 |
|------|------|-------------|
| **边沿触发** (Edge) | 信号跳变时触发 | 0b10 |
| **电平触发** (Level) | 信号保持高电平触发 | 0b00 |

```c
// 设置中断 30 为边沿触发
GICD->ICFGR[30/16] &= ~(0x3 << (2 * (30 % 16)));
GICD->ICFGR[30/16] |=  (0x2 << (2 * (30 % 16)));
```

> 通用定时器中断通常是**电平触发**。  
> GPIO 按键中断通常用**边沿触发**。

---

## 13.6 GICv2 vs GICv3 对照

| 特性 | GICv2 (GIC-400) | GICv3 (GIC-600) |
|------|-----------------|-----------------|
| 平台 | Pi4B | Pi5 / QEMU virt |
| 最大中断数 | 最多 1020 | 最多 1020+（支持更多） |
| 亲和性路由 | GICD_ITARGETSR | GICR（Redistributor，每核独立） |
| 寄存器访问 | MMIO（GICD/GICC 基址） | MMIO + 系统寄存器（ICC_*_EL1） |
| 中断确认 | 读 GICC_IAR | 读 ICC_IAR0_EL1 / ICC_IAR1_EL1 |
| 结束中断 | 写 GICC_EOIR | 写 ICC_EOIR0_EL1 / ICC_EOIR1_EL1 |
| MSI | 不支持 | 支持 |

> **Pi5 适配最大坑**：GICv3 多了一层 Redistributor（GICR），初始化流程完全不同。  
> 建议：先在 QEMU `virt`（可配 GICv3）上做，再上 Pi5。

---

## 13.7 实验要点

| 实验 | 内容 | 平台 | Pi5 适配 |
|------|------|------|----------|
| 13-1 | 通用定时器中断（GICv2 流程） | QEMU | QEMU 可配 GICv2 |
| 13-2 | 树莓派系统定时器 | Pi4B | Pi5=GICv3，需改接口 |

---

## 13.8 易错点清单

1. **IAR/EOIR 不配对** → GIC 卡住，不再送中断。
2. **忘记设 PMR** → PMR 默认 0，所有中断被掩蔽。
3. **中断号除法搞错** → ISENABLER 每 32 个一组，`irq/32` 是组号，`irq%32` 是位号。
4. **目标 CPU 没设** → ITARGETSR 默认 0，中断不知道送给哪个核。
5. **Pi5 用 GICv2 代码** → 寄存器地址和流程完全不同，必须改 GICv3。

---

## 书中思考题（自测）

1. GIC 的 Distributor 和 CPU Interface 分别做什么？
2. 中断处理的 IAR→EOIR 流程是什么？不写 EOIR 会怎样？
3. GICD_ISENABLER 如何使能第 N 号中断？
4. 边沿触发和电平触发的区别？通用定时器用哪种？
5. GICv2 和 GICv3 的主要区别？Pi5 用哪个？

**参考答案：**

1. Distributor=**全局仲裁/优先级/路由**；CPU Interface=**每核确认中断/返回中断号/结束中断**。  
2. 读 IAR 获取中断号 → 处理 → 写 EOIR 结束。不写 EOIR → **GIC 不再送同优先级中断**。  
3. `ISENABLER[N/32] |= (1 << (N%32))`。  
4. 边沿=信号跳变触发；电平=保持高电平触发。通用定时器通常用**电平触发**。  
5. GICv3 多了 Redistributor、用系统寄存器、支持 MSI。Pi5 用 **GIC-600(GICv3)**。

---

上一章 [Ch12 中断处理](../../chapter-12-interrupt-handling/) · 下一章 [Ch14 内存管理](../../chapter-14-memory-management/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
