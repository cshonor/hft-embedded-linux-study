## Ch9 完整概述 · 浮点简介：基础、类型与传输

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Introduction to Floating-Point: Basics, Data Types, and Data Transfer · **跳过**（主线）/ **M4F 选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **IEEE 754 单精度** | 格式 · 五类值 · 动态范围 vs 整数 |
| **M4 FPU 硬件** | **s0–s31** · **CPACR** · **FPSCR** |
| **搬运与转换** | **VLDR/VSTR** · **VMOV** · **VCVT** · half |

**前置：** [Ch3 FPU 预览](../chapter-03-instruction-sets-v4t-v7m/notes/section-3-6-example-float.md) · [Ch1 浮点直觉](../chapter-01-overview-computing-systems/notes/section-1-5-representation.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **为何浮点** | §9.2–9.3 | [§9.2](./section-9-2-history.md) · [§9.3](./section-9-3-overview.md) |
| **IEEE 格式** | §9.4 | [§9.4](./section-9-4-data-types.md) |
| **五类值** | §9.5–9.6 | [§9.5](./section-9-5-representable.md) · [§9.6](./section-9-6-special-values.md) |
| **寄存器/控制** | §9.7–9.8 | [§9.7](./section-9-7-fp-registers.md) · [§9.8](./section-9-8-fpu-control.md) |
| **搬运/转换** | §9.9–9.11 | [§9.9](./section-9-9-fp-transfer.md) … [§9.11](./section-9-11-int-float-convert.md) |

---

### 三、知识流（口述版）

```
为何 float：动态范围 >> int32
        ↓
S + exp(bias127) + 1.f → Normal；五类特殊编码
        ↓
CPACR 开 FPU → s0–s31 / FPSCR
        ↓
VLDR/VSTR ↔ Mem；VMOV 位拷贝；VCVT 真换算
        ↓
half 压缩；定点 #fbits ↔ float（ADC/DAC）
        ↓
Ch10 舍入/异常 · Ch11 算术指令
```

---

### 四、路线对照

| 路径 | Ch9 |
|------|-----|
| **Linux 驱动/C float** | 概念即可；少写 V* |
| **M4F 裸机** | 精读 |
| **Ch7 Q 定点** | 无 FPU 替代方案 |
| **[24 姿态/Kalman](../../../24-Motion-Control-Motor/)** | 通常 C + libm；本章懂 **NaN/使能** 即可 |

---

### 五、下一章

→ **[Ch10 舍入与异常](../chapter-10-floating-point-rounding-exceptions/)**（浮点路径 · 主线 **跳过**）
