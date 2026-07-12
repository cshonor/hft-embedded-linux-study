# Ch 10 · 浮点简介：舍入与异常

> ***ARM Assembly Language*** — William Sw Smith · **跳过**  
> **English:** Introduction to Floating-Point: Rounding and Exceptions

---

## 本章定位

| | |
|---|---|
| **标签** | **跳过**（主线）— **M4F / float 控制环** 选读 **§10.3–10.5** |
| **承接** | [Ch9 IEEE754/FPSCR](../chapter-09-floating-point-basics/notes/section-0-本章完整概述.md) |
| **解答** | float 为何 **反直觉** — 舍入 + 异常 + 结合律失效 |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§10.1** | 简介 | [notes/section-10-1-intro.md](./notes/section-10-1-intro.md) |
| **§10.2** | 舍入 — IEEE 754-2008 舍入模式 | [notes/section-10-2-rounding.md](./notes/section-10-2-rounding.md) |
| **§10.3** | 异常 — 除零 · 无效 · 溢出 · 下溢 · 不精确 | [notes/section-10-3-exceptions.md](./notes/section-10-3-exceptions.md) |
| **§10.4** | 代数定律与浮点运算 | [notes/section-10-4-algebra.md](./notes/section-10-4-algebra.md) |
| **§10.5** | 规格化与抵消 | [notes/section-10-5-normalization.md](./notes/section-10-5-normalization.md) |
| **§10.6** | 练习题 | [notes/section-10-6-exercises.md](./notes/section-10-6-exercises.md) |

---

## 本章 Checklist

- [ ] 解释 **G/S 位** 与 **RNE（默认）/ RZ**
- [ ] 列举 **DZC/IOC/OFC/UFC/IXC** 触发与默认结果
- [ ] 说明 **浮点除零 → ±∞** 而非 crash
- [ ] 口述 **结合律失效** 与 **灾难性抵消**

---

← [Ch 9](../chapter-09-floating-point-basics/) · 下一章 [Ch 11](../chapter-11-floating-point-data-processing/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
