## §17.1 简介

> **Ch 17 · ARM、Thumb 和 Thumb-2 指令** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 为何单独成章

[Ch3](../chapter-03-instruction-sets-v4t-v7m/) 已区分 **v4T** 与 **v7-M**；本书多数例程混用 **ARM7（双态）** 与 **M4（Thumb-2 only）**。本章 **系统梳理** 三种指令宽度/语义，避免：

- 写 M4 时找 **ARM 32-bit `{cond}`** 后缀  
- 读 ARM7 文献时不懂 **BX bit0**  
- 链接错误时不知 **Veneer** 从哪来  

---

### 三条 ISA 线

```
ARM 32-bit     — 经典、条件执行、密度低
Thumb 16-bit   — 压缩、限制多、ARM7 第二态
Thumb-2        — 16+32 超集、UAL、现代 MCU
```

**口述定位：** **Thumb-2 = 你现在写 Cortex-M 的汇编**；前两代是 **理解 ARM7 与链接器** 的背景。

---

### 与 CPSR / 流水线

ARM7 **CPSR 的 T 位** = 当前 **Thumb 态**（[Ch2](../chapter-02-programmers-model/)）。  
异常时硬件常 **清 T** → 进 **ARM 态** handler（[Ch14](../chapter-14-exception-handling-arm7tdmi/)）。

M4 **无 T 位切换概念** — 始终 Thumb。

---

### 可复述要点

1. 本章讲 **代码密度 vs 表达能力** 的架构权衡。  
2. **M4 路线：Thumb-2 为主**；ARM/16-Thumb 为 **读老代码/ARM7**。  
3. 下一章 **Ch18** 在 **编译/链接** 层用这些规则。
