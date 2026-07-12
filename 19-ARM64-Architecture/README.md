# ARM64 架构 · 汇编前置

**文件夹 19** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24)

> **定位：** ARM **A 架构**（应用处理器）— **非** STM32 / MCU 裸机。  
> **主线：** HFT（x86-64）；**本模块：** 飞行器 / 网关等 **嵌入式 Linux 退路**。  
> **书目原则：** **全外文** — 以汇编书打地基，再进 U-Boot/内核/驱动。

---

## 必读书（1 本 · 核心）

| # | 书目 | 读什么 |
|---|------|--------|
| 1 | ***ARM Assembly Language*** — William Sw Smith | **v4T / v7-M 汇编基础** · Load/Store · 栈 · MMIO · **C/汇编互调**（→ [OUTLINE.md](./OUTLINE.md) 章节目录） |

**架构边界：** 本书 **不是 AArch64 专用教材** — 正文为 **ARM7TDMI / Cortex-M**；嵌入式 Linux（ARM-A）读完后用 ARM 官方 [*ARMv8-A Programmer's Guide*](https://developer.arm.com/documentation/den0024/latest) 补 **64 位语法与 EL0–EL3**。

---

## 为何汇编前置

| 后续模块 | 需要汇编直觉 |
|----------|--------------|
| [20 构建](../20-UBoot-Kernel-Build/) | U-Boot 启动早期、内核 `head.S` |
| [21 驱动](../21-Linux-Device-Driver/) | 中断入口、原子指令 `ldxr`/`stxr` |
| [04 LKD](../04-Linux-Kernel-Development/) | 对照 x86 `syscall` ↔ ARM `svc` |

**顺序：** 按 [OUTLINE.md](./OUTLINE.md) 精读 **Ch2–8、13、16、18** → 再开 [20 Mastering Embedded Linux Programming](../20-UBoot-Kernel-Build/)。

---

## 章节目录（文件夹）

📋 **阅读裁剪与标签** → [OUTLINE.md](./OUTLINE.md)

### 正文（Ch 1–18）

| 章 | 文件夹 | 标签 |
|----|--------|------|
| 1 | [chapter-01-overview-computing-systems](./chapter-01-overview-computing-systems/) | 选读 |
| 2 | [chapter-02-programmers-model](./chapter-02-programmers-model/) | **精读** |
| 3 | [chapter-03-instruction-sets-v4t-v7m](./chapter-03-instruction-sets-v4t-v7m/) | **精读** |
| 4 | [chapter-04-assembler-rules-directives](./chapter-04-assembler-rules-directives/) | **精读** |
| 5 | [chapter-05-loads-stores-addressing](./chapter-05-loads-stores-addressing/) | **精读** |
| 6 | [chapter-06-constants-literal-pools](./chapter-06-constants-literal-pools/) | 选读 |
| 7 | [chapter-07-integer-logic-arithmetic](./chapter-07-integer-logic-arithmetic/) | **精读** |
| 8 | [chapter-08-branches-loops](./chapter-08-branches-loops/) | **精读** |
| 9–11 | [ch09](./chapter-09-floating-point-basics/) · [ch10](./chapter-10-floating-point-rounding-exceptions/) · [ch11](./chapter-11-floating-point-data-processing/) | 跳过 |
| 12 | [chapter-12-tables](./chapter-12-tables/) | 选读 |
| 13 | [chapter-13-subroutines-stacks](./chapter-13-subroutines-stacks/) | **精读** |
| 14–15 | [ch14](./chapter-14-exception-handling-arm7tdmi/) · [ch15](./chapter-15-exception-handling-v7m/) | 选读 |
| 16 | [chapter-16-memory-mapped-peripherals](./chapter-16-memory-mapped-peripherals/) | **精读** |
| 17 | [chapter-17-arm-thumb-thumb2-instructions](./chapter-17-arm-thumb-thumb2-instructions/) | 选读 |
| 18 | [chapter-18-mixing-c-and-assembly](./chapter-18-mixing-c-and-assembly/) | **精读** |

### 附录与其他

| | 文件夹 |
|---|--------|
| 附录 A–D | [A](./appendix-A-code-composer-studio/) · [B](./appendix-B-keil-tools/) · [C](./appendix-C-ascii-character-codes/) · [D](./appendix-D/) |
| 术语表 / 参考文献 | [glossary/](./glossary/) · [references/](./references/) |
| 实验代码 | [code/](./code/) |

---

## 复用（HFT 链）

| 已有 | 本模块用法 |
|------|------------|
| **[02 C](../02-c-programming/)** | 汇编与 C 互调、指针/MMIO |
| [01 CSAPP](../01-CSAPP-3rd/) x86-64 | **对照学** cache、调用约定 |
| [03 Hennessy](../03-Computer-Architecture-6th/) Ch2 | MESI — ARM 同样适用 |
| [08 MikanOS](../08-system-low-level-hands-on/01-mikan-os/) Ch3+ | UEFI/x86 启动链 — 与 ARM **概念平行**（Loader → 内核） |

---

## x86 ↔ ARM64 对照要点

| 主题 | x86-64（已学） | ARM64（本章） |
|------|----------------|---------------|
| 特权级 | Ring 0–3 | **EL0–EL3** |
| 系统调用 | `syscall` | **`svc`** |
| 页表 | 4/5 级 | **4 级**（类似） |
| 原子/屏障 | `lock` / `mfence` | **`ldxr`/`stxr` · DMB/DSB** |

---

## 验收

- [ ] 按 [OUTLINE.md](./OUTLINE.md) 完成 **精读章**（Load/Store · 栈 · MMIO · C/汇编）  
- [ ] 能解释 **EL1 内核 / EL0 用户态** 与 x86 Ring 的对应关系（ARMv8-A Guide）  
- [ ] 能读简单 **ARM 汇编**（函数序言、MMIO 读写）  
- [ ] 知道 **设备树** 为何取代 hard-coded 寄存器（→ [22](../22-Device-Tree-Study/)）

**下一章：** [20 嵌入式 Linux 构建](../20-UBoot-Kernel-Build/)
