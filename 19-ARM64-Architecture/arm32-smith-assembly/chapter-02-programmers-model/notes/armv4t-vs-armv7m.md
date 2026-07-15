## ARMv4T vs ARMv7-M 完整对比

> **Ch 2 · 程序员模型** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)  
> 展开：[§2.3 ARM7](./section-2-3-arm7tdmi.md) · [§2.4 M4](./section-2-4-cortex-m4.md) · [§2.5 练习](./section-2-5-exercises.md)

---

## 一、基础定位

### 1. ARMv4T

| | |
|--|--|
| **代表** | **ARM7TDMI**（经典 32 位 ARM） |
| **指令** | **双指令集**：ARM 32 位 + Thumb **1**（16 位精简） |
| **T** | = Thumb 第一代 |
| **面向** | 老式 MCU、早期手机基带、工业老设备 |

### 2. ARMv7-M

| | |
|--|--|
| **代表** | **Cortex-M3 / M4 / M7**（本书实验 **M4**） |
| **指令** | **仅 Thumb-2**（16 位短码 + 32 位扩展 **一条状态**） |
| **面向** | 现代单片机、物联网、电机控制 |

⚠ **勿把全 Cortex-M 都标成 v7-M：**

| 内核 | 架构 |
|------|------|
| **M0 / M0+** | **ARMv6-M**（更瘦，无 nPRIV 等细节见 CONTROL 图） |
| **M3 / M4 / M7** | **ARMv7-M** |
| **M23 / M33…** | ARMv8-M（本书 Ch2 不展开） |

---

## 二、核心关键差异

### 1. 指令集

| ARMv4T | ARMv7-M |
|--------|---------|
| **ARM 状态** ↔ **Thumb 状态** 运行时切换 | **无纯 ARM32**；**全程 Thumb-2** |
| Thumb1 功能较阉割 | 长短编码共处，**不必切模式**；密度 + 性能兼顾 |

### 2. 运行模式（最大考点）

| ARMv4T（ARM7） | ARMv7-M（M4） |
|----------------|---------------|
| **7 种** banked 模式（User / System / IRQ / FIQ / SVC / Abort / Undef） | **Thread / Handler** |
| 各异常模式独立 **SP / LR / SPSR**（FIQ 还有 r8–r12） | **无** 七模式 bank |
| | 硬件自动压栈 + **MSP / PSP** 代替多组 SP |

### 3. 状态寄存器

| ARMv4T | ARMv7-M |
|--------|---------|
| **CPSR** + 各异常 **SPSR_*** | 统一 **xPSR**（APSR + IPSR + EPSR） |
| | **无 SPSR**；状态进 **硬件栈帧** |

### 4. 向量表（练习重点）

| ARMv4T | ARMv7-M |
|--------|---------|
| 存 **`B handler` 跳转指令** | 存 **函数地址** |
| `0x00` = Reset 的 `B` | `0x00` = **初始 MSP**；`0x04` = Reset 地址 |
| | 地址 **LSB=1** 标 Thumb |

### 5. 内存对齐

| ARMv4T | ARMv7-M |
|--------|---------|
| **严格**字对齐；非对齐 LDR/STR → fault | **可配置**允许非对齐（MCU 缓冲更灵活） |

### 6. 浮点

| ARMv4T | ARMv7-M |
|--------|---------|
| 通常 **无硬件 FPU**，软件模拟 | M4/M7 **可选 FPU**（**M4F** 单精度）；硬件加速 |

---

## 三、考试极简背诵

1. **v4T = ARM7TDMI** — ARM+Thumb1 · 7 模式 · banked 寄存器 · 向量表存 **`B`**  
2. **v7-M = M3/M4/M7** — **仅 Thumb-2** · Thread/Handler · MSP/PSP · 向量表存 **地址**  
3. **学习顺序：** 先 v4T 懂经典底层 → 再 v7-M 看 MCU 如何简化  

---

## 四、易混：ARMv7 两大分支

| | **ARMv7-A** | **ARMv7-M** |
|--|-------------|-------------|
| 产品 | Cortex-**A**（手机应用核等） | Cortex-**M**（MCU） |
| 指令 | 兼容 ARM32 + Thumb-2（历史上） | **无 ARM32**，仅 Thumb-2 |
| 模型 | 偏经典特权/异常（+ MMU 等） | **Thread/Handler** 极简中断 |
| 本书 Ch2 | **不展开** | **主战场（M4）** |

**当前章只盯两本：** **v4T（ARM7）** 与 **v7-M（M4）**。  
AArch64 → [奔跑吧](../../../arm64-programming-practice/)。

---

## 五、一页总表

| 维度 | ARMv4T | ARMv7-M |
|------|--------|---------|
| 代表 | ARM7TDMI | Cortex-M4 |
| 指令 | ARM + Thumb1 | Thumb-2 only |
| 模式 | 7 + bank | Thread / Handler |
| 栈 | 多模式 SP | MSP + PSP |
| 状态 | CPSR + SPSR | xPSR（无 SPSR） |
| 向量 | `B` 指令 | 函数指针；首字=MSP |
| 对齐 | 强制严格 | 可开非对齐 |
| 浮点 | 通常软件 | 可选硬件 FPU |
