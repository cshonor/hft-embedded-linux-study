## §2.1 简介

> **Ch 2 · 程序员模型** · [章导读](../README.md)

---

### 本章在全书中的位置

| | |
|---|---|
| **角色** | **精读** — 从「程序员可见的 CPU 长什么样」切入，是 Ch3 写指令前的**必备地图** |
| **视角** | **程序员模型 (Programmer's Model)**：不关心晶体管，只关心寄存器、模式、异常入口、数据宽度 |
| **双线对比** | **ARM7TDMI (v4T)** 经典 ARM-A 风格 ↔ **Cortex-M4 (v7-M)** 本书实战主战场 |

**前置：** [Ch1 §1.2 RISC/ARM 史](../../chapter-01-overview-computing-systems/notes/section-1-2-risc-history.md) · [Ch1 §1.5 数据表示](../../chapter-01-overview-computing-systems/notes/section-1-5-representation.md)

---

### 三大主题

```
§2.2  数据类型 — Byte / Halfword / Word 与对齐
§2.3  ARM7TDMI — 7 种模式 · Banked 寄存器 · 向量表存跳转指令
§2.4  Cortex-M4 — Handler/Thread · MSP/PSP · 向量表存处理函数地址
```

---

### 与后续 / 其他路径

| 本章概念 | 落地 |
|----------|------|
| r13=SP · r14=LR · r15=PC | **Ch13** 子程序与堆栈 · **Ch18** C/汇编互调 |
| CPSR / xPSR 标志位 | **Ch7** 整数运算与条件执行 |
| 异常向量表 | **Ch14–15** 异常 · [奔跑吧 Ch11 异常](../../../arm64-programming-practice/chapter-11-exception-handling/) |
| 特权 / 用户 | Linux **EL0/EL1**（[奔跑吧 Ch1](../../../arm64-programming-practice/chapter-01-arm64-fundamentals/)） |

---

### 可复述一句话

> Ch2 回答：**两种 ARM 芯片上，程序员能摸到哪些寄存器、异常时 CPU 怎么换上下文、向量表长什么样** — 之后每条 `LDR`/`B` 都有落脚点。
