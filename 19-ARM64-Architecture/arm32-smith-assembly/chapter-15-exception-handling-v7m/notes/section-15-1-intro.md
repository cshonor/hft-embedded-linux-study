## §15.1 简介

> **Ch 15 · 异常处理：v7-M** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 本章解决什么问题

[Ch14](../chapter-14-exception-handling-arm7tdmi/) 讲 **ARM7TDMI** — 七种模式、向量存 **`B`**、软件 **`SUBS pc,lr,#n`**。

**Cortex-M3/M4 (v7-M)** 为 **微控制器** 重设计异常模型：

| 设计目标 | 手段 |
|----------|------|
| **低延迟** | 硬件 **自动压栈** 8 寄存器 |
| **少汇编** | ISR 多数用 **C 函数** 即可 |
| **统一模型** | **Fault + 外设 IRQ** 都走 NVIC 编号 |
| **RTOS 友好** | **MSP/PSP** + **PendSV/SVCall** |

---

### 与 Ch2 的关系

[Ch2 §2.4](../../chapter-02-programmers-model/notes/section-2-4-cortex-m4.md) 已预告 **Thread/Handler、MSP/PSP、向量表首项 MSP** — 本章 **展开机制与编程**。

---

### 全书位置

```
Ch13 子程序/栈  →  Ch14 ARM7 异常（背景）
                        ↓
                 Ch15 v7-M 异常（M4 实操）
                        ↓
                 Ch16 MMIO + 外设中断
```

---

### 可复述要点

1. **v7-M** = MCU 标准异常模型；**Ch15 权重 > Ch14**（若做 M4）。  
2. 核心新词：**NVIC · 硬件栈帧 · EXC_RETURN · MSP/PSP**。  
3. 开发者 **大部分 ISR 用 C** — 汇编主要用于 **启动、OS 切换、极短路径**。
