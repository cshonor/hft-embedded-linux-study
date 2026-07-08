# ARM64 架构 · 汇编前置

**文件夹 19** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24)

> **定位：** ARM **A 架构**（应用处理器）— **非** STM32 / MCU 裸机。  
> **主线：** HFT（x86-64）；**本模块：** 飞行器 / 网关等 **嵌入式 Linux 退路**。  
> **书目原则：** **全外文** — 以汇编书打地基，再进 U-Boot/内核/驱动。

---

## 必读书（1 本 · 核心）

| # | 书目 | 读什么 |
|---|------|--------|
| 1 | ***ARM Assembly Language*** — William Sw Smith | ARM64 **指令集** · 函数序言/尾声 · **与 C 互调** · 系统调用 `svc` |

**可选参考（不替代主书）：** ARM 官方 *ARMv8-A Programmer's Guide* — EL0–EL3、异常、内存模型速查。

---

## 为何汇编前置

| 后续模块 | 需要汇编直觉 |
|----------|--------------|
| [20 构建](../20-UBoot-Kernel-Build/) | U-Boot 启动早期、内核 `head.S` |
| [21 驱动](../21-Linux-Device-Driver/) | 中断入口、原子指令 `ldxr`/`stxr` |
| [04 LKD](../04-Linux-Kernel-Development/) | 对照 x86 `syscall` ↔ ARM `svc` |

**顺序：** 读完本章 **再开** [20 Mastering Embedded Linux Programming](../20-UBoot-Kernel-Build/)。

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

- [ ] 能解释 **EL1 内核 / EL0 用户态** 与 x86 Ring 的对应关系  
- [ ] 能读简单 **ARM64 汇编**（函数序言、系统调用）  
- [ ] 知道 **设备树** 为何取代 hard-coded 寄存器（→ [22](../22-Device-Tree-Study/)）

**下一章：** [20 嵌入式 Linux 构建](../20-UBoot-Kernel-Build/)
