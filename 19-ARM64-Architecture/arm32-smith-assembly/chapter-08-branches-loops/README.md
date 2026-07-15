# Ch 8 · 分支与循环

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** Branches and Loops

---

## 本章定位

| | |
|---|---|
| **角色** | **精读** — **Ch7 标志 → 控制流**；**BL** 衔接 **Ch13** |
| **优化轴** | 流水线 flush · **条件执行/IT** · **SUBS+BNE** · **循环展开** |
| **前置** | [Ch7 标志/CMP](../chapter-07-integer-logic-arithmetic/notes/section-0-本章完整概述.md) |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§8.1** | 简介 | [notes/section-8-1-intro.md](./notes/section-8-1-intro.md) |
| **§8.2** | 分支机制 — ARM7TDMI · v7-M | [notes/section-8-2-branches.md](./notes/section-8-2-branches.md) |
| **§8.3** | 循环 — While · For · Do-While | [notes/section-8-3-loops.md](./notes/section-8-3-loops.md) |
| **§8.4** | 条件执行 — v4T 条件执行 · v7-M IT 块 | [notes/section-8-4-conditional.md](./notes/section-8-4-conditional.md) |
| **§8.5** | 直线型编码 — 循环展开 | [notes/section-8-5-straight-line.md](./notes/section-8-5-straight-line.md) |
| **§8.6** | 练习题 | [notes/section-8-6-exercises.md](./notes/section-8-6-exercises.md) |

---

## 本章 Checklist

- [ ] 说清 **B / BL / BX / BLX** 与 **CBZ/CBNZ** 限制
- [ ] 用 **SUBS + BNE** 写向下 For 循环
- [ ] 实现 **While / Do-While** 骨架
- [ ] 对比 **ARM 条件后缀** 与 **IT 块（T/E 掩码）**
- [ ] 解释 **循环展开** 的空间/时间权衡

---

← [Ch 7](../chapter-07-integer-logic-arithmetic/) · 下一章 [Ch 9](../chapter-09-floating-point-basics/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
