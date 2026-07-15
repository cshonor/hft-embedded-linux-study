## §11.1 简介

> **Ch 11 · 浮点数据处理指令** · [章导读](../README.md)

---

### 本章在全书中的位置

| | |
|---|---|
| **标签** | **跳过**（Linux/奔跑吧主线）— **M4F 浮点三章收官**（Ch9–11） |
| **承接** | [Ch9 搬运/转换](../chapter-09-floating-point-basics/notes/section-0-本章完整概述.md) · [Ch10 舍入/异常](../chapter-10-floating-point-rounding-exceptions/notes/section-0-本章完整概述.md) |
| **后续** | **Ch12** 查表 · **Ch22** NEON · [24 飞控](../../../24-Motion-Control-Motor/) 多用 C `float` |

---

### 本章覆盖的 FPU 能力

| 有 | 无 |
|----|-----|
| **VADD/VSUB/VMUL/VDIV/VSQRT** | **布尔/位操作** — 须 **VMOV→整数寄存器** 用 AND/ORR |
| **VABS/VNEG** | |
| **VMLA / VFMA** 等 MAC | |
| **VCMP/VCMPE** + **VMRS** 接条件分支 | |

**Ch3 §3.6** 已见 `VADD.F` — 本章 **指令全集 + 比较/ MAC 精度**。

---

### 可复述一句话

> Ch11 = **M4 FPU 怎么算** — 语法、比较接 APSR、FZ/DN、**链式 MAC vs 融合 FMA**、二分法综合例。
