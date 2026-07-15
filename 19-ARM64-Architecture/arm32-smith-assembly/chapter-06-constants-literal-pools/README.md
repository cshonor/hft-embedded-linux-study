# Ch 6 · 常量与文字池

> ***ARM Assembly Language*** — William Sw Smith · **选读**  
> **English:** Constants and Literal Pools

---

## 本章定位

| | |
|---|---|
| **角色** | **选读** — **`LDR rd,=imm`** / 文字池 / **MOVW·MOVT** 幕后机制 |
| **痛点** | 32 bit 指令 **塞不下** 完整 32 bit 常数 |
| **必记用法** | `LDR =` 加载基址/掩码 — 与 **Ch5** 一体 |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

**前置：** [Ch5 Load/Store](../chapter-05-loads-stores-addressing/notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§6.1** | 简介 | [notes/section-6-1-intro.md](./notes/section-6-1-intro.md) |
| **§6.2** | ARM 循环移位方案 — 8 bit + 偶数 ROR · MVN | [notes/section-6-2-rotate-constants.md](./notes/section-6-2-rotate-constants.md) |
| **§6.3** | **`LDR =` 伪指令** — 汇编器选 MOV 或文字池 | [notes/section-6-3-load-constants.md](./notes/section-6-3-load-constants.md) |
| **§6.4** | 文字池 · **LTORG** · **MOVW/MOVT** | [notes/section-6-4-literal-pools.md](./notes/section-6-4-literal-pools.md) |
| **§6.5** | 加载地址 — **ADR** / **ADRL** / `LDR=label` | [notes/section-6-5-load-addresses.md](./notes/section-6-5-load-addresses.md) |
| **§6.6** | 练习题 | [notes/section-6-6-exercises.md](./notes/section-6-6-exercises.md) |

---

## 本章 Checklist

- [ ] 解释 **imm8 + 偶数循环右移** 与 **MVN** 扩展
- [ ] 说清 **`LDR r0,=X`** 何时变成 MOV vs `LDR [pc,#n]`
- [ ] 知道何时插入 **`LTORG`**（PC 相对约 ±4KB）
- [ ] 会用 **MOVW+MOVT** 加载任意 32 bit 常数
- [ ] 区分 **`ADR`（近）** 与 **`LDR =label`（远/外部）**

---

← [Ch 5](../chapter-05-loads-stores-addressing/) · 下一章 [Ch 7](../chapter-07-integer-logic-arithmetic/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
