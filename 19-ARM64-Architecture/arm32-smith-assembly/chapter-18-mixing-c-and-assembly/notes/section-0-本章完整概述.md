## Ch18 完整概述 · C 与汇编混合编程

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Mixing C and Assembly · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **为何混编** | DSP/饱和运算 · **PSR/Q** · **VCVT** · MMIO · 手写热点 |
| **内联汇编** | **`__asm`** — 短片段、编译器分配寄存器、**限制多** |
| **内嵌汇编** | C 模块内 **完整 asm 函数** — 全指令集、**须自写返回** |
| **互调** | **AAPCS** — **r0–r3** 参返 · callee-save · **C↔asm `BL`** |

**前置：** [Ch13 AAPCS](../chapter-13-subroutines-stacks/notes/section-13-5-apcs.md) · [Ch17 Interwork](../chapter-17-arm-thumb-thumb2-instructions/notes/section-17-5-interworking.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **动机 · 两路径** | §18.1 | [section-18-1-intro.md](./section-18-1-intro.md) |
| **Inline Assembler** | §18.2 | [section-18-2-inline-asm.md](./section-18-2-inline-asm.md) |
| **Embedded Assembler** | §18.3 | [section-18-3-embedded-asm.md](./section-18-3-embedded-asm.md) |
| **C ↔ Asm 调用** | §18.4 | [section-18-4-c-asm-calls.md](./section-18-4-c-asm-calls.md) |
| **练习** | §18.5 | [section-18-5-exercises.md](./section-18-5-exercises.md) |

---

### 三、两种方法对照

| | **内联汇编 (Inline)** | **内嵌汇编 (Embedded)** |
|---|----------------------|-------------------------|
| 粒度 | **几条指令** | **整函数** |
| 指令集 | **受限**（书中：无 Thumb、无 BX/SVC…） | **完整 ARM/Thumb** |
| 调用开销 | 可能被 **内联进 C** | **独立函数**，正常 **BL** |
| 返回 | 编译器收尾 | **手写 `BX lr`** |
| AAPCS | 编译器多帮忙 | **程序员全责** |
| 适用 | 读 Q 标志、单条饱和指令 | **strcpy**、**VCVT 库** |

**Linux/GCC 路线：** 另见 [奔跑吧 Ch10 GCC inline asm](../arm64-programming-practice/chapter-10-gcc-inline-asm/) · 独立 **`.S` + `.global`**

---

### 四、知识流（口述版）

```
C 主程序
        ↓
短操作？ → __asm inline（PSR、SSAT…）
复杂？   → embedded asm 或 .S 文件
        ↓
互调：extern 声明 · AAPCS · -mthumb/interwork
        ↓
21 驱动 · 内核 arch/arm · U-Boot 板级 C+asm
```

---

### 五、与 HFT / 嵌入式链

| 模块 | 关联 |
|------|------|
| [02 C](../../02-c-programming/) | 调用约定 · `volatile` |
| [Ch7 饱和/Q](../chapter-07-integer-logic-arithmetic/) | inline 读 **Q 标志** |
| [Ch9–11 浮点](../chapter-09-floating-point-basics/) | **VCVT** 封装给 C |
| [Ch16 MMIO](../chapter-16-memory-mapped-peripherals/) | **`volatile uint32_t *REG`** |
| [21 驱动](../../21-Linux-Device-Driver/) | **`readl/writel`** · 极少手写 asm |
| [04 LKD](../../04-Linux-Kernel-Development/) | **`arch/arm/lib`** · **`asm/`** 宏 |

---

### 六、Smith 正文位置

**Ch18 = 第四部分（高级/混合）精读收官** — 之后为附录（CCS 等，可跳过）。

---

### 七、下一模块

→ **奔跑吧 Ch10**（AArch64 **GCC 内联 asm**）· **模块 20**（U-Boot **start.S + C**）
