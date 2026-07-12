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

| 章 | 英文标题 | 中文 | 标签 | 与 19→20→21 的关联 |
|----|----------|------|------|---------------------|
| **1** | An Overview of Computing Systems | 计算机系统概述 | **选读** | 建立 ARM 生态背景 |
| **2** | The Programmer's Model | 程序员模型 | **精读** | 寄存器 · 模式 · 与 x86/CSAPP 对照 |
| **3** | Introduction to Instruction Sets: v4T and v7-M | 指令集简介：v4T 和 v7-M | **精读** | ARM/Thumb 基础；**AArch64 语法另查 ARMv8-A Guide** |
| **4** | Assembler Rules and Directives | 汇编器规则与伪指令 | **精读** | 读 U-Boot/内核 `.S` 中的 `.word` / `.align` 等 |
| **5** | Loads, Stores, and Addressing | 加载、存储与寻址 | **精读** | **Load/Store 架构** — 驱动 MMIO 读写根基 |
| **6** | Constants and Literal Pools | 常量与文字池 | **选读** | 理解 PC 相对寻址、常量表 |
| **7** | Integer Logic and Arithmetic | 整数逻辑与算术 | **精读** | 位操作 · 地址计算 |
| **8** | Branches and Loops | 分支与循环 | **精读** | 控制流 · 读启动/异常路径 |
| **9** | Introduction to Floating-Point: Basics, Data Types, and Data Transfer | 浮点简介：基础、类型与传输 | **跳过** | Linux 内核/驱动热路径少用手写 FP 汇编 |
| **10** | Introduction to Floating-Point: Rounding and Exceptions | 浮点简介：舍入与异常 | **跳过** | 同上 |
| **11** | Floating-Point Data-Processing Instructions | 浮点数据处理指令 | **跳过** | 同上 |
| **12** | Tables | 表 | **选读** | 查表跳转 · 部分 boot 代码风格 |
| **13** | Subroutines and Stacks | 子程序与堆栈 | **精读** | **函数调用约定 · 栈帧** — 读 `head.S` / 与 C 互调 |
| **14** | Exception Handling: ARM7TDMI | 异常处理：ARM7TDMI | **选读** | 异常概念；**Linux 用 AArch64 异常模型** |
| **15** | Exception Handling: v7-M | 异常处理：v7-M | **选读** | Cortex-M 专用；**概念可对照** 内核中断入口 |
| **16** | Memory-Mapped Peripherals | 内存映射外设 | **精读** | **MMIO** — 直连 [21 驱动](../21-Linux-Device-Driver/) · [22 DT reg](../22-Device-Tree-Study/) |
| **17** | ARM, Thumb and Thumb-2 Instructions | ARM、Thumb 和 Thumb-2 指令 | **选读** | 指令速查；AArch64 用 ARM DDI 手册 |
| **18** | Mixing C and Assembly | C 与汇编混合编程 | **精读** | **`extern` / 调用约定** — 与 [02 C](../02-c-programming/) 衔接 |

---

## 附录及其他

| 部分 | 英文 | 标签 | 说明 |
|------|------|------|------|
| **附录 A** | Running Code Composer Studio | **跳过** | TI CCS — 本书实验 IDE；**本路线用 WSL + GCC** |
| **附录 B** | Running Keil Tools | **跳过** | Keil MDK — Cortex-M 商业链；**不纳入 Linux 支线** |
| **附录 C** | ASCII Character Codes | **选读** | 调试字符/协议时速查 |
| **附录 D** | Appendix D | **选读** | 按书内实际标题决定是否查阅 |
| **术语表** | Glossary | **选读** | 遇到缩写时翻 |
| **参考文献** | References | **选读** | 延伸 ARM 官方文档入口 |

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
