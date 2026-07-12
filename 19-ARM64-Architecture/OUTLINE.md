# ARM Assembly Language — 章节目录与阅读裁剪

> **书目：** *ARM Assembly Language* — **William Sw Smith**  
> **模块：** [19-ARM64-Architecture/](./README.md) · 嵌入式支线 **汇编前置**  
> **架构说明：** 本书正文以 **ARM v4T（ARM7TDMI）** 与 **v7-M（Cortex-M）** 为主 — **不是** AArch64 专用教材；学的是 **汇编思维、Load/Store、栈、异常、MMIO、C/汇编互调**，再进 [20 U-Boot](../20-UBoot-Kernel-Build/) / [21 驱动](../21-Linux-Device-Driver/) 时对照 **ARMv8-A** 官方文档补 AArch64 语法差异。

| 标签 | 含义 |
|------|------|
| **精读** | 嵌入式 Linux 支线必看 |
| **选读** | 有上下文价值；时间紧可后补 |
| **跳过** | 与 Linux/GCC 路线无关或本书工具链专用 |

---

## 正文章节

| 章 | 英文标题 | 中文 | 标签 | 文件夹 |
|----|----------|------|------|--------|
| **1** | An Overview of Computing Systems | 计算机系统概述 | **选读** | [chapter-01](./chapter-01-overview-computing-systems/) |
| **2** | The Programmer's Model | 程序员模型 | **精读** | [chapter-02](./chapter-02-programmers-model/) |
| **3** | Introduction to Instruction Sets: v4T and v7-M | 指令集简介：v4T 和 v7-M | **精读** | [chapter-03](./chapter-03-instruction-sets-v4t-v7m/) |
| **4** | Assembler Rules and Directives | 汇编器规则与伪指令 | **精读** | [chapter-04](./chapter-04-assembler-rules-directives/) |
| **5** | Loads, Stores, and Addressing | 加载、存储与寻址 | **精读** | [chapter-05](./chapter-05-loads-stores-addressing/) |
| **6** | Constants and Literal Pools | 常量与文字池 | **选读** | [chapter-06](./chapter-06-constants-literal-pools/) |
| **7** | Integer Logic and Arithmetic | 整数逻辑与算术 | **精读** | [chapter-07](./chapter-07-integer-logic-arithmetic/) |
| **8** | Branches and Loops | 分支与循环 | **精读** | [chapter-08](./chapter-08-branches-loops/) |
| **9** | Introduction to Floating-Point: Basics, Data Types, and Data Transfer | 浮点简介：基础、类型与传输 | **跳过** | [chapter-09](./chapter-09-floating-point-basics/) |
| **10** | Introduction to Floating-Point: Rounding and Exceptions | 浮点简介：舍入与异常 | **跳过** | [chapter-10](./chapter-10-floating-point-rounding-exceptions/) |
| **11** | Floating-Point Data-Processing Instructions | 浮点数据处理指令 | **跳过** | [chapter-11](./chapter-11-floating-point-data-processing/) |
| **12** | Tables | 表 | **选读** | [chapter-12](./chapter-12-tables/) |
| **13** | Subroutines and Stacks | 子程序与堆栈 | **精读** | [chapter-13](./chapter-13-subroutines-stacks/) |
| **14** | Exception Handling: ARM7TDMI | 异常处理：ARM7TDMI | **选读** | [chapter-14](./chapter-14-exception-handling-arm7tdmi/) |
| **15** | Exception Handling: v7-M | 异常处理：v7-M | **选读** | [chapter-15](./chapter-15-exception-handling-v7m/) |
| **16** | Memory-Mapped Peripherals | 内存映射外设 | **精读** | [chapter-16](./chapter-16-memory-mapped-peripherals/) |
| **17** | ARM, Thumb and Thumb-2 Instructions | ARM、Thumb 和 Thumb-2 指令 | **选读** | [chapter-17](./chapter-17-arm-thumb-thumb2-instructions/) |
| **18** | Mixing C and Assembly | C 与汇编混合编程 | **精读** | [chapter-18](./chapter-18-mixing-c-and-assembly/) |

---

## 附录及其他

| 部分 | 英文 | 标签 | 文件夹 |
|------|------|------|--------|
| **附录 A** | Running Code Composer Studio | **跳过** | [appendix-A](./appendix-A-code-composer-studio/) |
| **附录 B** | Running Keil Tools | **跳过** | [appendix-B](./appendix-B-keil-tools/) |
| **附录 C** | ASCII Character Codes | **选读** | [appendix-C](./appendix-C-ascii-character-codes/) |
| **附录 D** | Appendix D | **选读** | [appendix-D](./appendix-D/) |
| **术语表** | Glossary | **选读** | [glossary/](./glossary/) |
| **参考文献** | References | **选读** | [references/](./references/) |

---

## 推荐阅读顺序（嵌入式 Linux 支线）

```
2  程序员模型
   ↓
3  指令集（v4T/v7-M 基础）
   ↓
4  伪指令 → 5  Load/Store → 7  整数运算 → 8  分支
   ↓
13 子程序与堆栈  ←→  18 C/汇编混合
   ↓
16 MMIO 外设
   ↓
（选读 14–15 异常概念）→ 开 20 U-Boot / 21 驱动
```

**AArch64 补完（开 20 前或并行）：** ARM 官方 [*ARMv8-A Programmer's Guide*](https://developer.arm.com/documentation/den0024/latest) — EL0–EL3 · `svc` · 64 位寄存器 · 与本书 **概念一一映射**。

---

## 与 HFT / MikanOS 对照

| 已学（HFT 链） | 本书对应 |
|----------------|----------|
| [01 CSAPP](../01-CSAPP-3rd/) Ch3 机器级 | 另一 ISA 的同一层思维 |
| [08 MikanOS](../08-system-low-level-hands-on/01-mikan-os/) x86 UEFI | **Ch16 MMIO** ≈ GOP 写帧缓冲 · **Ch13/18** ≈ Loader 调内核 |
| [04 LKD](../04-Linux-Kernel-Development/) 中断 | **Ch14–15** 异常概念 → AArch64 `entry.S` |

---

← [19 README](./README.md) · 下一模块 [20 构建](../20-UBoot-Kernel-Build/)
