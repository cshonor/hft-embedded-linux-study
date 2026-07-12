# Ch 11 · 浮点数据处理指令

> ***ARM Assembly Language*** — William Sw Smith · **跳过**  
> **English:** Floating-Point Data-Processing Instructions

---

## 本章定位

| | |
|---|---|
| **标签** | **跳过**（主线）— **M4F Ch9–11 收官**；C 代码路径 **选读 §11.4/11.7** |
| **要点** | **VCMP+VMRS** · **VFMA vs VMLA** · FPU **无位操作** |
| **前置** | [Ch10 舍入/异常](../chapter-10-floating-point-rounding-exceptions/notes/section-0-本章完整概述.md) |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§11.1** | 简介 | [notes/section-11-1-intro.md](./notes/section-11-1-intro.md) |
| **§11.2** | 浮点指令语法 | [notes/section-11-2-syntax.md](./notes/section-11-2-syntax.md) |
| **§11.3** | 浮点指令摘要 | [notes/section-11-3-summary.md](./notes/section-11-3-summary.md) |
| **§11.4** | 标志位 — 比较指令 · N/Z/C/V | [notes/section-11-4-flags.md](./notes/section-11-4-flags.md) |
| **§11.5** | Flush-to-Zero · 默认 NaN 模式 | [notes/section-11-5-special-modes.md](./notes/section-11-5-special-modes.md) |
| **§11.6** | 非算术指令 — 绝对值 · 求反 | [notes/section-11-6-non-arithmetic.md](./notes/section-11-6-non-arithmetic.md) |
| **§11.7** | 算术指令 — 加减 · 乘加 · 除法 · 平方根 | [notes/section-11-7-arithmetic.md](./notes/section-11-7-arithmetic.md) |
| **§11.8** | 编码示例 | [notes/section-11-8-examples.md](./notes/section-11-8-examples.md) |
| **§11.9** | 练习题 | [notes/section-11-9-exercises.md](./notes/section-11-9-exercises.md) |

---

## 本章 Checklist

- [ ] 写 **`Vop.F32 Sd,Sn,Sm`** 示例
- [ ] 掌握 **VCMP → VMRS APSR_nzcv → B/IT**
- [ ] 对比 **VMLA（双舍入）与 VFMA（融合）**
- [ ] 知 **FZ/DN** 对 subnormal/NaN 的影响
- [ ] 口述 **二分法 + 泰勒 sin** 示例用了哪些 V 指令

---

← [Ch 10](../chapter-10-floating-point-rounding-exceptions/) · 下一章 [Ch 12](../chapter-12-tables/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
