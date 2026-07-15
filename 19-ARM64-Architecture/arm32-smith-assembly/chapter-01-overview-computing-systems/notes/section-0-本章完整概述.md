## Ch1 完整概述 · 计算机系统概述

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** An Overview of Computing Systems · **选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **建立背景** | 进入 ARM 汇编前，统一 RISC/ARM 史、硬件模型、数据表示、工具链语言 |
| **不写指令** | 指令细节从 **Ch3** 开始；Ch1 解决「为什么 ARM 这样设计、比特如何表示世界」 |
| **可压缩** | 已有 CSAPP / C 基础者，精读 §1.2 Load/Store 思想 + §1.5 补码 + §1.6–1.7 工具流即可 |

---

### 二、四大主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **RISC 与 ARM 发展史** | §1.2 | [section-1-2-risc-history.md](./section-1-2-risc-history.md) |
| **存储程序模型与计算层级** | §1.3 | [section-1-3-computing-devices.md](./section-1-3-computing-devices.md) |
| **数字系统与数据表示** | §1.4 · §1.5 | [§1.4](./section-1-4-number-systems.md) · [§1.5](./section-1-5-representation.md) |
| **比特→指令与工具链** | §1.6 · §1.7 | [§1.6](./section-1-6-bits-to-commands.md) · [§1.7](./section-1-7-tools.md) |

---

### 三、知识流（口述版）

```
ARM 选择 RISC：固定指令 + 仅 Load/Store 访存
        ↓
冯·诺依曼机：内存里既有指令也有数据，CPU+总线+RAM
        ↓
一切用比特表示：hex 调试、补码整数、IEEE754 浮点、ASCII 字符
        ↓
助记符给汇编器 → .o → 链接器 → 可执行；Keil/CCS 或 GNU 工具链
        ↓
Ch2：这些比特落在哪些寄存器、哪种 CPU 模式下
```

---

### 四、与 HFT / 嵌入式支线对照

| 已学 / 将学 | Ch1 呼应 |
|-------------|----------|
| [01 CSAPP](../../../../01-CSAPP-3rd/) 机器级 | 另一 ISA，同一「表示 + 调用约定」层 |
| [03 Hennessy](../../../../03-Computer-Architecture-6th/) | 微架构在 ISA 之下；RISC 流水线 |
| [08 MikanOS](../../../../08-system-low-level-hands-on/01-mikan-os/) | 也是「比特进内存 → CPU 从入口跑」 |
| [奔跑吧 ARM64](../../../arm64-programming-practice/) | Cortex-**A** + AArch64 主战场 |
| [20 构建](../../../../20-UBoot-Kernel-Build/) | 同一工具链概念，尺度变为内核/根文件系统 |

---

### 五、下一章

→ **[Ch2 程序员模型](../../chapter-02-programmers-model/)** — ARM7TDMI 与 Cortex-M4 的寄存器、模式、向量表（**精读**）
