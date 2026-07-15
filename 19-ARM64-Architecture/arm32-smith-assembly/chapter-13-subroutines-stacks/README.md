# Ch 13 · 子程序与堆栈

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** Subroutines and Stacks

---

## 本章定位

| | |
|---|---|
| **角色** | **精读** — **`BL` + 堆栈** 是可复用汇编与 **C 互调** 的基石 |
| **核心模式** | **STMDB/LDMIA** ≡ PUSH/POP · **FD 栈** · 序言/尾声 · **AAPCS** |
| **前置** | [Ch8 BL](../chapter-08-branches-loops/notes/section-8-2-branches.md) · [Ch5 寻址](../chapter-05-loads-stores-addressing/) |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§13.1** | 简介 | [notes/section-13-1-intro.md](./notes/section-13-1-intro.md) |
| **§13.2** | 堆栈 — LDM/STM · PUSH/POP · 满/空 · 递增/递减 | [notes/section-13-2-stacks.md](./notes/section-13-2-stacks.md) |
| **§13.3** | 子程序 | [notes/section-13-3-subroutines.md](./notes/section-13-3-subroutines.md) |
| **§13.4** | 向子程序传递参数 — 寄存器 · 指针 · 堆栈 | [notes/section-13-4-parameters.md](./notes/section-13-4-parameters.md) |
| **§13.5** | ARM APCS — 应用过程调用标准 | [notes/section-13-5-apcs.md](./notes/section-13-5-apcs.md) |
| **§13.6** | 练习题 | [notes/section-13-6-exercises.md](./notes/section-13-6-exercises.md) |

---

## 本章 Checklist

- [ ] 说清 **PUSH/POP** 与 **STMDB/LDMIA** 的对应关系
- [ ] 解释 **满递减 (FD)** 栈的生长方向与 **SP** 含义
- [ ] 写可重入子程序：**`STMDB sp!, {r4-r7, lr}`** + **`LDMIA sp!, {r4-r7, pc}`**
- [ ] 对比三种传参：**寄存器 / 指针 / 堆栈**
- [ ] 背 **AAPCS**：**r0–r3**、**r4–r11** callee-save、**s16–s31**、**8 字节栈对齐**
- [ ] 能对照 [02 C](../../02-c-programming/) 说明 `int foo(int a,int b)` 的寄存器布局

---

← [Ch 12](../chapter-12-tables/) · 下一章 [Ch 14](../chapter-14-exception-handling-arm7tdmi/) · [OUTLINE](../OUTLINE.md) · [19 README](../../README.md)
