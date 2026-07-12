# Ch 9 · 浮点简介：基础、类型与传输

> ***ARM Assembly Language*** — William Sw Smith · **跳过**  
> **English:** Introduction to Floating-Point: Basics, Data Types, and Data Transfer

---

## 本章定位

<!-- 读完后补充：要点、与 20 U-Boot / 21 驱动的衔接 -->

| | |
|---|---|
| **阅读标签** | **跳过**（见 [OUTLINE](../OUTLINE.md)） |
| **架构** | 本书 **v4T / v7-M**；AArch64 主书见 [奔跑吧 ARM64](../arm64-programming-practice/) |

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

- [ ] 读完原书对应章
- [ ] 在 `notes/` 写下可复述的要点
- [ ] （若 **精读**）能对照 [02 C](../../02-c-programming/) 或内核 `.S` 举例

---

← [Ch 8](../chapter-08-branches-loops/) · 下一章 [Ch 10](../chapter-10-floating-point-rounding-exceptions/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
