# Ch 2 · 程序员模型

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** The Programmer's Model

---

## 本章定位

| | |
|---|---|
| **角色** | **精读** — 程序员可见的 **寄存器、模式、向量表**；Ch3 写指令前的地图 |
| **双线** | **ARM7TDMI (v4T)** 经典对照 · **Cortex-M4 (v7-M)** 本书实验平台 |
| **架构** | 本书 **v4T / v7-M**；AArch64 EL/向量见 [奔跑吧 ARM64](../arm64-programming-practice/) |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

**前置：** [Ch1 概述](../chapter-01-overview-computing-systems/notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§2.1** | 简介 | [notes/section-2-1-intro.md](./notes/section-2-1-intro.md) |
| **§2.2** | 数据类型 | [notes/section-2-2-data-types.md](./notes/section-2-2-data-types.md) |
| **§2.3** | ARM7TDMI — 处理器模式 · 寄存器 · 向量表 | [notes/section-2-3-arm7tdmi.md](./notes/section-2-3-arm7tdmi.md) |
| **§2.4** | Cortex-M4 — 处理器模式 · 寄存器 · 向量表 | [notes/section-2-4-cortex-m4.md](./notes/section-2-4-cortex-m4.md) |
| **§2.5** | 练习题 | [notes/section-2-5-exercises.md](./notes/section-2-5-exercises.md) |

---

## 本章 Checklist

- [ ] 说出 **s8/u8 … s32/u32** 与 **byte/halfword/word** 对应及 **对齐**
- [ ] 说明 **f32/f64** 宽度、内存占用与 **s/d 寄存器**（细节 → Ch9）
- [ ] 背 **7 模式四类**：User/System · SVC · IRQ/FIQ · UND/ABT
- [ ] 说清 **Bank 四作用**：独立 SP · LR 返址 · SPSR 快照 · FIQ r8–r12
- [ ] 区分 **ARM32 7 模式** vs **ARM64 EL0–EL3**（[奔跑吧](../../arm64-programming-practice/)）
- [ ] 解释 M4 **MSP/PSP**、**Thread/Handler**、**向量表存地址且 LSB=1**
- [ ] 能对照 [Ch3](../chapter-03-instruction-sets-v4t-v7m/) 示例辨认 r0–r15 用途

---

← [Ch 1](../chapter-01-overview-computing-systems/) · 下一章 [Ch 3](../chapter-03-instruction-sets-v4t-v7m/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md)
