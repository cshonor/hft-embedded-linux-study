# Ch 9 · 浮点简介：基础、类型与传输

> ***ARM Assembly Language*** — William Sw Smith · **跳过**  
> **English:** Introduction to Floating-Point: Basics, Data Types, and Data Transfer

---

## 本章定位

| | |
|---|---|
| **标签** | **跳过**（嵌入式 Linux / 奔跑吧主线）— **M4F / [24 飞控](../../24-Motion-Control-Motor/)** 路径 **选读** |
| **内容** | **IEEE 754 单精度** · **s0–s31** · **CPACR/FPSCR** · **VLDR/VCVT** |
| **前置** | [Ch3 §3.6–3.7 FPU 预览](../chapter-03-instruction-sets-v4t-v7m/notes/section-3-6-example-float.md) |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§9.1** | 简介 | [notes/section-9-1-intro.md](./notes/section-9-1-intro.md) |
| **§9.2** | 浮点历史 | [notes/section-9-2-history.md](./notes/section-9-2-history.md) |
| **§9.3** | 浮点概述 | [notes/section-9-3-overview.md](./notes/section-9-3-overview.md) |
| **§9.4** | 浮点数据类型 | [notes/section-9-4-data-types.md](./notes/section-9-4-data-types.md) |
| **§9.5** | 浮点可表示的值 — 正常 · 次正常 · 零 | [notes/section-9-5-representable.md](./notes/section-9-5-representable.md) |
| **§9.6** | 特殊值 — 无穷大 · NaN | [notes/section-9-6-special-values.md](./notes/section-9-6-special-values.md) |
| **§9.7** | Cortex-M4 浮点寄存器文件 | [notes/section-9-7-fp-registers.md](./notes/section-9-7-fp-registers.md) |
| **§9.8** | FPU 控制寄存器 — FPSCR · CPACR | [notes/section-9-8-fpu-control.md](./notes/section-9-8-fpu-control.md) |
| **§9.9** | 浮点数据传输 | [notes/section-9-9-fp-transfer.md](./notes/section-9-9-fp-transfer.md) |
| **§9.10** | 半精度与单精度转换 | [notes/section-9-10-precision-convert.md](./notes/section-9-10-precision-convert.md) |
| **§9.11** | 浮点与整数/定点格式相互转换 | [notes/section-9-11-int-float-convert.md](./notes/section-9-11-int-float-convert.md) |
| **§9.12** | 练习题 | [notes/section-9-12-exercises.md](./notes/section-9-12-exercises.md) |

---

## 本章 Checklist

- [ ] 画出 **单精度 S/exp/f** 与 **bias 127** 公式
- [ ] 区分 **Normal / Subnormal / ±0 / ∞ / NaN**
- [ ] 说清 **CPACR 开 FPU** 与 **FPSCR** 作用
- [ ] 区分 **`VLDR` / `VMOV`(位拷贝) / `VCVT`(换算)**
- [ ] （选读）**half** 与 **定点 #fbits VCVT** 场景

---

← [Ch 8](../chapter-08-branches-loops/) · 下一章 [Ch 10](../chapter-10-floating-point-rounding-exceptions/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
