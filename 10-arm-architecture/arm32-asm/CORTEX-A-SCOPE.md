# 本书会讲 Cortex-A 吗？讲，但不练

> FAQ · Hohl & Hinds *Fundamentals and Techniques* 第 2 版（约 2014）  
> [全书总结](./BOOK-SUMMARY.md) · [OUTLINE](./OUTLINE.md) · Pi5 实战 → [../aarch64-practice/](../aarch64-practice/)

---

## 结论

**会提到 Cortex-A，但重心不在它。**  
主线实操 = **ARM7TDMI（v4T）+ Cortex-M4（v7-M）**；Cortex-A / R 主要是**谱系对比与科普**，不是树莓派 / Linux 汇编教程。

---

## 1. 主次分清

| | 角色 |
|--|------|
| **主线** | Cortex-**M**（及书中 ARM7 经典模型）：样例、习题、Keil/CCS、Tiva 类板 |
| **横向科普** | Cortex-**A**（带 MMU、跑 Linux）、Cortex-**R**（硬实时）— 分类与对比 |
| **没有的** | Cortex-A 汇编实验、Linux 用户态/内核汇编、树莓派开发流程 |

---

## 2. 书里哪里出现 A？

| 位置 | 讲什么 | 不讲什么 |
|------|--------|----------|
| **Ch1** 产品线 | A/R/M 分工；A = MMU + Linux 应用核 | A 核板上跑代码 |
| **架构谱系** | v4 / v5 / v6 / **v7-A** vs **v7-M**；M **仅 Thumb/Thumb-2**，无原生 ARM-state 32 位指令集路径 | A 的完整 ISA 手册级覆盖 |
| **Ch2 / 14 / 15** | 7 模式（ARM7 类）vs M 的 Thread/Handler；向量表差异；A 有 MMU、M 无 | Linux 页表、异常与 GIC 实操 |
| **浮点章** | A 侧完整 VFP 叙事 vs M4 简化 FPU | AArch64 SIMD 主线 |

用途：搞懂 **为何一块芯片跑 Linux、一块只能裸机/RTOS**（与 [Primer · Linux vs RTOS](../../11-embedded-boot-build/primer-system-overview/chapter-01-introduction/1.1-linux-vs-rtos.md)、[SoC 重心](../../11-embedded-boot-build/primer-system-overview/chapter-03-processor-basics/3.0-focus-on-soc.md) 一致）。

---

## 3. 「讲 A ≠ 教 A 开发」

- **讲：** 定位、有无 MMU、指令态差异、特权/异常模型对比  
- **教（全书代码）：** 几乎全是 **M 核 / 书中 ARM7 例程**  
- **你要树莓派 / Pi5：** 学完本书精读章的**思维**（Load/Store、栈、AAPCS、MMIO）→ 转 **[aarch64-practice](../aarch64-practice/)**（A64 · EL · GIC），不要指望本书当 Pi 汇编课

---

## 4. 一句话

本书 **介绍** Cortex-A 是什么、和 M 差在哪；**不教** 树莓派这类 Linux 板的汇编开发。  
学习重心 = **Cortex-M 单片机汇编**；A 核只是对照刻度。
