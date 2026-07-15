# Ch 3 · 指令集简介：v4T 和 v7-M

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** Introduction to Instruction Sets: v4T and v7-M

---

## 本章定位

| | |
|---|---|
| **角色** | **精读** — 第一次动手：**5 个小程序** 学框架、整数指令、条件/循环 |
| **整数主线** | §3.2–3.5 + §3.8 **必做**；§3.6–3.7 FPU **选做/跳过** |
| **架构** | ARM7 条件后缀 vs M4 **IT 块** + **Thumb-2 only** |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

**前置：** [Ch2 程序员模型](../chapter-02-programmers-model/notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§3.1** | 简介 | [notes/section-3-1-intro.md](./notes/section-3-1-intro.md) |
| **§3.2** | ARM、Thumb 和 Thumb-2 指令对比 | [notes/section-3-2-arm-thumb-compare.md](./notes/section-3-2-arm-thumb-compare.md) |
| **§3.3** | 示例程序 1 — 数据移位 | [notes/section-3-3-example-shift.md](./notes/section-3-3-example-shift.md) |
| **§3.4** | 示例程序 2 — 阶乘计算 | [notes/section-3-4-example-factorial.md](./notes/section-3-4-example-factorial.md) |
| **§3.5** | 示例程序 3 — 寄存器交换 | [notes/section-3-5-example-register-swap.md](./notes/section-3-5-example-register-swap.md) |
| **§3.6** | 示例程序 4 — 浮点数操作 | [notes/section-3-6-example-float.md](./notes/section-3-6-example-float.md) |
| **§3.7** | 示例程序 5 — 整数与浮点寄存器数据传输 | [notes/section-3-7-example-int-float-xfer.md](./notes/section-3-7-example-int-float-xfer.md) |
| **§3.8** | 编程指南 | [notes/section-3-8-programming-guide.md](./notes/section-3-8-programming-guide.md) |
| **§3.9** | 练习题 | [notes/section-3-9-exercises.md](./notes/section-3-9-exercises.md) |

---

## 本章 Checklist

- [ ] 说清 **ARM / Thumb / Thumb-2** 宽度与 M4 选型
- [ ] 能写含 **`AREA`/`ENTRY`** 的最小程序并解释 **`MOV`/`LSL`/`CMP`**
- [ ] 对比 **ARM 条件后缀** 与 **IT 块** 各写一段阶乘循环
- [ ] 会用 **`EOR` 交换** 与 **`LDR =` 大常数**
- [ ] 记住 **初始化** 对寄存器与外设的重要性（§3.8）

---

← [Ch 2](../chapter-02-programmers-model/) · 下一章 [Ch 4](../chapter-04-assembler-rules-directives/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
