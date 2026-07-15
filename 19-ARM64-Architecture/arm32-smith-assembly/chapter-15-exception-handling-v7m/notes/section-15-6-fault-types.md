## §15.6 异常类型 — 硬故障 · 内存管理故障等

> **Ch 15 · 异常处理：v7-M** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 统一异常编号

Cortex-M 把 **内部 fault** 与 **外部 IRQ** 都纳入 **异常号 (Exception Number)**：

- **系统异常** 1–15（Reset、NMI、HardFault、SVCall…）  
- **外部中断** 16+（**IRQ0 = 异常号 16**）

NVIC 用同一套 **使能/优先级** 机制管理（外部 IRQ 部分）。

---

### 固定最高优先级

| 异常 | 优先级 |
|------|--------|
| **Reset** | **-3** |
| **NMI** | **-2** |
| **HardFault** | **-1** |

**不可屏蔽** 的 NMI；**HardFault** = 「其它 fault 处理不了」或 **双重 fault** 的兜底。

---

### 可配置 Fault（需 SHCSR 等使能）

| Fault | 典型原因 |
|-------|----------|
| **MemManage** | **MPU** 违规 · 访问禁止区域 |
| **BusFault** | AHB 总线错误 · 外设地址无效 |
| **UsageFault** | **未定义指令** · **非法状态**（试图进 ARM 32-bit）· **除零**（若 **DIVBYZERO** trap 开启）· **未对齐**访问（若使能） |

**调试口诀：** 开发板 **HardFault_Handler 死循环** → 读 **CFSR/HFSR/BFAR/MMFAR** 定位（CMSIS **`HardFault_Handler`** 模板）。

---

### 系统服务类异常

| 异常 | 用途 |
|------|------|
| **SVCall** | 用户态 **`SVC #n`** → **系统调用**（同 Ch14 SVC，更规整） |
| **PendSV** | **可挂起** — RTOS **上下文切换**（设最低优先级，在 ISR 退出后批处理） |
| **SysTick** | 片上 **24 bit 递减定时器** — **RTOS tick** / `delay` 时基 |

**RTOS 经典组合：**

```
SysTick     → 时间片
PendSV      → 真正换任务（改 PSP）
SVCall      → 启动第一个任务 / 系统 API
```

---

### Fault vs 外设 IRQ

| | **Fault** | **外设 IRQ** |
|---|-----------|--------------|
| 来源 | CPU/MPU/总线/指令 | GPIO、Timer、UART… |
| 处理 | 常 **log + 停机/复位** | **清标志 + 短服务** |
| 优先级 | HardFault 很高 | NVIC 可配 **嵌套** |

---

### 与 Ch14 对照

| Ch14 | Ch15 |
|------|------|
| Prefetch/Data Abort | MemManage / BusFault |
| Undefined | UsageFault（部分） |
| IRQ + VIC | **IRQ #n + NVIC** |
| SVC | **SVCall** |

---

### 可复述要点

1. **Reset/NMI/HardFault** 优先级 **-3/-2/-1** 固定。  
2. **UsageFault** 可 trap **除零、非法指令、对齐**。  
3. **SysTick + PendSV + SVCall** = **RTOS 三件套**。
