## Ch17 完整概述 · ARM、Thumb 与 Thumb-2 指令

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** ARM, Thumb and Thumb-2 Instructions · **选读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **三代 ISA** | **32-bit ARM** · **16-bit Thumb** · **Thumb-2（16+32 混合）** |
| **为何 Thumb** | **代码密度 ~65–70%** · 16-bit 总线 **_fetch 更快** |
| **状态切换** | **`BX`/`BLX`** — 目标地址 **bit0** = ARM/Thumb |
| **Interworking** | **`BL` 不能切状态** → 链接器 **Veneer** |
| **M4 现实** | **仅 Thumb-2** — 无 ARM 状态、无 Veneer |

**前置：** [Ch3 指令集概览](../chapter-03-instruction-sets-v4t-v7m/) · [Ch8 分支/BX](../chapter-08-branches-loops/notes/section-8-2-branches.md) · [Ch8 §8.4 IT](../chapter-08-branches-loops/notes/section-8-4-conditional.md)

---

### 二、主题 → 小节索引

| 主题 | 小节 | 笔记 |
|------|------|------|
| **动机** | §17.1 | [section-17-1-intro.md](./section-17-1-intro.md) |
| **ARM vs 16-bit Thumb** | §17.2 | [section-17-2-arm-vs-thumb16.md](./section-17-2-arm-vs-thumb16.md) |
| **Thumb-2 · UAL** | §17.3 | [section-17-3-thumb2.md](./section-17-3-thumb2.md) |
| **BX 状态切换** | §17.4 | [section-17-4-state-switch.md](./section-17-4-state-switch.md) |
| **Veneer / Interworking** | §17.5 | [section-17-5-interworking.md](./section-17-5-interworking.md) |
| **练习** | §17.6 | [section-17-6-exercises.md](./section-17-6-exercises.md) |

---

### 三、演进时间线（口述版）

```
32-bit ARM（条件执行、全寄存器）
        ↓
16-bit Thumb（密度↑、限制多、PLA「解压」执行）
        ↓
Thumb-2 2003（16+32 混合、UAL、Cortex-M 唯一 ISA）
        ↓
AArch64 A64（另起炉灶 — 奔跑吧主书）
```

---

### 四、三代对照表

| | **ARM 32** | **Thumb 16** | **Thumb-2** |
|---|------------|--------------|-------------|
| 宽度 | 固定 32 | 固定 16 | **16 或 32** |
| 条件执行 | **`{cond}` 后缀** | **无**（除 B{cond}） | **IT 块** + 32-bit |
| 寄存器 | r0–r15 | 多数 **r0–r7** | **r0–r12** 等 |
| 标志 **S** | 可选后缀 | **多数默认更新** | **UAL：须显式 S** |
| 典型 CPU | ARM7 **ARM 态** | ARM7 **Thumb 态** | **Cortex-M3/M4** |

---

### 五、与 HFT / 嵌入式链

| 模块 | 关联 |
|------|------|
| [Ch14 异常](../chapter-14-exception-handling-arm7tdmi/) | ARM7 异常入口 **强制 ARM 态** |
| [Ch15 v7-M](../chapter-15-exception-handling-v7m/) | **永远 Thumb-2** |
| [Ch18 C/Asm](../chapter-18-mixing-c-and-assembly/) | 编译选项 **`-mthumb`** · interwork |
| [04 LKD / 内核](../../04-Linux-Kernel-Development/) | 早期 ARM32 内核 **ARM/Thumb 混链** |
| [奔跑吧 A64](../arm64-programming-practice/) | **无 Thumb** — 另一套固定 32-bit |

---

### 六、阅读建议（选读章）

做 **Cortex-M4**：精读 **§17.3 Thumb-2 + UAL**；§17.4–17.5 作 **ARM7/历史背景**。做 **Linux AArch32 文献**：§17.4–17.5 **Veneer** 仍常见。

---

### 七、下一章

→ **[Ch18 C 与汇编混合编程](../chapter-18-mixing-c-and-assembly/)**（**精读**）
