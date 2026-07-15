# Ch 12 · 表

> ***ARM Assembly Language*** — William Sw Smith · **选读**  
> **English:** Tables

---

## 本章定位

| | |
|---|---|
| **角色** | **选读** — **查表换速度** + **有序表二分搜索** |
| **核心模式** | 缩放寻址 · Q31 sin 象限压缩 · VLDR 文字池 · **mid=(lo+hi) ASR #1** |
| **前置** | [Ch5 寻址](../chapter-05-loads-stores-addressing/) · [Ch7 Q 格式](../chapter-07-integer-logic-arithmetic/notes/section-7-7-fractional.md) · [Ch8 循环](../chapter-08-branches-loops/) |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§12.1** | 简介 | [notes/section-12-1-intro.md](./notes/section-12-1-intro.md) |
| **§12.2** | 整数查找表 | [notes/section-12-2-int-lookup.md](./notes/section-12-2-int-lookup.md) |
| **§12.3** | 浮点查找表 | [notes/section-12-3-float-lookup.md](./notes/section-12-3-float-lookup.md) |
| **§12.4** | 二分查找 (Binary Searches) | [notes/section-12-4-binary-search.md](./notes/section-12-4-binary-search.md) |
| **§12.5** | 练习题 | [notes/section-12-5-exercises.md](./notes/section-12-5-exercises.md) |

---

## 本章 Checklist

- [ ] 会用 **`LDR [base, index, LSL #2]`** / **`LDRH … LSL #1`** 读表项
- [ ] 说清 **sin 只存 0°–90°** 的象限归约思路
- [ ] 理解 **rsqrt 查表** 为何 beat 全精度 VSQRT/VDIV
- [ ] 手写 **二分查找** 骨架（**ASR #1** 求 mid）
- [ ] 对比 [Ch11 泰勒 sin](../chapter-11-floating-point-data-processing/notes/section-11-8-examples.md)：**算** vs **查**

---

← [Ch 11](../chapter-11-floating-point-data-processing/) · 下一章 [Ch 13](../chapter-13-subroutines-stacks/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
