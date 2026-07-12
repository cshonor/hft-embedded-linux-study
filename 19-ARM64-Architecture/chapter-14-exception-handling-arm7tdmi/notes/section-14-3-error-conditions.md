## §14.3 错误条件

> **Ch 14 · 异常处理：ARM7TDMI** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 错误：执行流中的 fault

相对 **可预期** 的中断，**错误条件** 表示 CPU 在取指或访问数据时 **遇到问题**。

| 异常 | 触发时机 | 典型原因 |
|------|----------|----------|
| **Undefined Instruction** | 译码阶段不认识 opcode | 错指令、Thumb/ARM 状态错、**故意用于软件浮点仿真** |
| **Prefetch Abort** | 取 **指令** 时 fault | 非法 PC、无 execute 权限、缺页（有 MMU） |
| **Data Abort** | **Load/Store 数据** 时 fault | 空指针、对齐、只读写、MMU 权限 |

---

### 未定义指令 — 软件扩展技巧

早期无硬件 FPU 时，编译器可能 emit **未定义 copro 指令**：

```
用户代码 "浮点加"  →  实际是无硬件支持的编码
        ↓
Undefined handler 解码指令 → 用整数库仿真 → 返回
```

**口述：** **Undefined** 不一定是 bug — 可以是 **刻意陷阱** 做 **指令仿真**（与 [Ch9–11](../chapter-09-floating-point-basics/) 浮点硬件对照）。

---

### 预取中止 vs 数据中止

| | **Prefetch Abort** | **Data Abort** |
|---|-------------------|----------------|
| 阶段 | **取指** | **数据访问** |
| LR 修正 | 返回时 often **`#4`** | often **`#8`**（ARM 流水线） |
| MMU | 指令页 fault | 数据页 fault / 权限 |

有 **MMU** 的 ARM7 系统（部分应用处理器）：handler 可 **修复页表** 后 **重试** 同一条指令 — 与 Linux **page fault** 同源（[04 LKD](../../04-Linux-Kernel-Development/)）。

---

### 与中断的响应差异（预告）

**优先级上** Data Abort **高于** IRQ/FIQ（§14.6）— fault 须先处理，否则状态不一致。

---

### 可复述要点

1. **三种 fault**：未定义、预取中止、数据中止 — 分 **译码 / 取指 / 数据** 阶段。  
2. **Undefined** 可用来做 **软件浮点/协处理器仿真**。  
3. **Abort** 在 MMU 系统中是 **缺页/保护** 的硬件入口。
