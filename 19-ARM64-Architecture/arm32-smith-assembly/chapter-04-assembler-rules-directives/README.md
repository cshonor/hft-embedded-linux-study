# Ch 4 · 汇编器规则与伪指令

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** Assembler Rules and Directives

---

## 本章定位

| | |
|---|---|
| **角色** | **精读** — **伪指令** 组织段/数据/常量；Keil ↔ CCS ↔ **GNU gas** |
| **要点** | 标签列规则 · `AREA`/`DCB`/`EQU`/`LTORG` · 宏 vs 子程序 |
| **工具** | 书中 Keil/CCS；读内核/U-Boot 用 **GNU 列**（附录 A/B 可跳过） |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

**前置：** [Ch3 指令集入门](../chapter-03-instruction-sets-v4t-v7m/notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§4.1** | 简介 | [notes/section-4-1-intro.md](./notes/section-4-1-intro.md) |
| **§4.2** | 汇编语言模块结构 | [notes/section-4-2-module-structure.md](./notes/section-4-2-module-structure.md) |
| **§4.3** | 预定义的寄存器名称 | [notes/section-4-3-register-names.md](./notes/section-4-3-register-names.md) |
| **§4.4** | 常用伪指令 — Keil / CCS · 代码块 · 对齐 · 文字池 | [notes/section-4-4-directives.md](./notes/section-4-4-directives.md) |
| **§4.5** | 宏 (Macros) | [notes/section-4-5-macros.md](./notes/section-4-5-macros.md) |
| **§4.6** | 汇编器杂项特性 — 操作符 · CCS 数学函数 | [notes/section-4-6-assembler-misc.md](./notes/section-4-6-assembler-misc.md) |
| **§4.7** | 练习题 | [notes/section-4-7-exercises.md](./notes/section-4-7-exercises.md) |

---

## 本章 Checklist

- [ ] 遵守 **标签第 1 列 / 指令缩进** 规则
- [ ] 对照 **Keil ↔ GNU**：`AREA`/`DCB`/`DCD`/`EQU`/`ALIGN`/`LTORG`
- [ ] 解释 **宏展开** 与 **`BL` 子程序** 的体积/速度权衡
- [ ] 用 **EQU + 汇编期移位** 写一个 MMIO 位掩码

---

← [Ch 3](../chapter-03-instruction-sets-v4t-v7m/) · 下一章 [Ch 5](../chapter-05-loads-stores-addressing/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
