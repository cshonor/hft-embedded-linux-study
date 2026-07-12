## §9.1 简介

> **Ch 9 · 浮点简介：基础、类型与传输** · [章导读](../README.md)

---

### 本章在全书中的位置

| | |
|---|---|
| **角色** | **跳过**（嵌入式 Linux 主线）— **Cortex-M4F / 姿态·ADC** 路径 **选读** |
| **转折** | 整数 ALU（Ch7–8）→ **IEEE 754 + FPU 寄存器 + 搬运/转换** |
| **前置** | [Ch1 §1.5 浮点直觉](../../chapter-01-overview-computing-systems/notes/section-1-5-representation.md) · [Ch3 §3.6–3.7 FPU 预览](../../chapter-03-instruction-sets-v4t-v7m/notes/section-3-6-example-float.md) |
| **后续** | **Ch10** 舍入/异常 · **Ch11** 浮点运算 · **Ch22** NEON（选读） |

---

### 阅读策略

| 读者 | 建议 |
|------|------|
| **→ Linux/驱动/奔跑吧 AArch64** | 用 C `float` + 内核 FPU lazy；**本章可整章跳过** |
| **→ M4F 裸机 / [24 飞控](../../../24-Motion-Control-Motor/) 无 FPU 对比** | 精读 §9.4–9.11；**先开 CPACR**（§9.8） |
| **→ 仅 int/Q 定点** | §9.4 五类值 + NaN 概念即可 — 防 debug 踩坑 |

---

### 可复述一句话

> Ch9 = **IEEE 754 单精度长什么样、s0–s31 怎么用、内存↔FPU 怎么搬、怎么与 int/定点互转**。
