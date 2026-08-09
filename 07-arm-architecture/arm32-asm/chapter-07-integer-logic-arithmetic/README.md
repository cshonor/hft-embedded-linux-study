# Ch 7 · 整数逻辑与算术

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** Integer Logic and Arithmetic

---

## 本章定位

| | |
|---|---|
| **角色** | **精读** — 全书 **最核心 ALU 章**；标志位 → **Ch8** 条件分支 |
| **必做** | §7.2–7.4 · §7.6（位域与 MMIO） |
| **选读** | §7.5 DSP · §7.7 Q 定点（无 FPU/飞控 int 路径再加深） |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

**前置：** [Ch5–6](../chapter-05-loads-stores-addressing/notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§7.1** | 简介 | [notes/section-7-1-intro.md](./notes/section-7-1-intro.md) |
| **§7.2** | 标志位 — N · V · Z · C | [notes/section-7-2-flags.md](./notes/section-7-2-flags.md) |
| **§7.3** | 比较指令 | [notes/section-7-3-compare.md](./notes/section-7-3-compare.md) |
| **§7.4** | 数据处理 — 布尔 · 移位 · 加减 · 饱和 · 乘除 | [notes/section-7-4-data-processing.md](./notes/section-7-4-data-processing.md) |
| **§7.5** | DSP 扩展 | [notes/section-7-5-dsp.md](./notes/section-7-5-dsp.md) |
| **§7.6** | 位操作指令 | [notes/section-7-6-bit-ops.md](./notes/section-7-6-bit-ops.md) |
| **§7.7** | 分数表示法 (Fractional Notation) | [notes/section-7-7-fractional.md](./notes/section-7-7-fractional.md) |
| **§7.8** | 练习题 | [notes/section-7-8-exercises.md](./notes/section-7-8-exercises.md) |

---

## 本章 Checklist

- [ ] 说清 **N · Z · C · V** 与 **`S` 后缀**
- [ ] 区分 **CMP / CMN / TST / TEQ**
- [ ] 会用 **shift-add** 乘小常数 · **ADC** 做 64 bit 加
- [ ] 知道 **M4 UDIV/SDIV** vs ARM7 软件除法
- [ ] 会用 **UBFX/BFI** 口述一个寄存器 bitfield
- [ ] （选读）**SSAT** 与 **Q 格式** 乘法 shift 规则

---

← [Ch 6](../chapter-06-constants-literal-pools/) · 下一章 [Ch 8](../chapter-08-branches-loops/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
