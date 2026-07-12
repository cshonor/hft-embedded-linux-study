# Ch 18 · C 与汇编混合编程

> ***ARM Assembly Language*** — William Sw Smith · **精读**  
> **English:** Mixing C and Assembly

---

## 本章定位

| | |
|---|---|
| **角色** | **精读** — Smith **正文混编收官**（inline · embedded · AAPCS 互调） |
| **核心模式** | **`__asm` 短插片** · **embedded 全函数 + `BX lr`** · **extern/BL** |
| **前置** | [Ch13 AAPCS](../chapter-13-subroutines-stacks/notes/section-13-5-apcs.md) · [Ch17](../chapter-17-arm-thumb-thumb2-instructions/) |

📋 **口述总览** → [notes/section-0-本章完整概述.md](./notes/section-0-本章完整概述.md)

---

## 小节笔记

| 小节 | 标题 | 笔记 |
|------|------|------|
| **§18.1** | 简介 | [notes/section-18-1-intro.md](./notes/section-18-1-intro.md) |
| **§18.2** | 内联汇编 (Inline Assembler) | [notes/section-18-2-inline-asm.md](./notes/section-18-2-inline-asm.md) |
| **§18.3** | 内嵌汇编 (Embedded Assembler) | [notes/section-18-3-embedded-asm.md](./notes/section-18-3-embedded-asm.md) |
| **§18.4** | C 与汇编相互调用 — APCS | [notes/section-18-4-c-asm-calls.md](./notes/section-18-4-c-asm-calls.md) |
| **§18.5** | 练习题 | [notes/section-18-5-exercises.md](./notes/section-18-5-exercises.md) |

---

## 本章 Checklist

- [ ] 对比 **Inline**（受限、compiler 分配 reg）与 **Embedded**（全 ISA、独立 BL）
- [ ] 列 **Inline 限制**：无 Thumb/BX/SVC、不改 PC/SP、无伪指令
- [ ] **Embedded**：手写 **返回** · 自保证 **AAPCS** · 表达式/前导零陷阱
- [ ] 实现 **C 调 asm_strcpy** 与 **asm 调 C 函数**（r0–r3 + 栈参）
- [ ] 说明 **VCVT/饱和** 封装为 C 库函数的价值
- [ ] 对照 [奔跑吧 Ch10 GCC inline asm](../arm64-programming-practice/chapter-10-gcc-inline-asm/) 与 [20 U-Boot](../../20-UBoot-Kernel-Build/) **start.S + C**

---

← [Ch 17](../chapter-17-arm-thumb-thumb2-instructions/) · [OUTLINE](../OUTLINE.md) · [19 README](../README.md) · **Smith 正文完**
