## §18.1 简介

> **Ch 18 · C 与汇编混合编程** · [章导读](../README.md) · [本章概述](./section-0-本章完整概述.md)

---

### 为何 C 不够

深度嵌入式常见需求：

| 需求 | 为何用 asm |
|------|------------|
| **信号/语音 DSP** | 饱和运算、SIMD、手写循环 |
| **硬件特性** | 读 **PSR/Q**、协处理器、**VCVT** |
| **极致优化** | 编译器达不到的 **寄存器/流水线** 安排 |
| **启动/Boot** | [Ch16](../chapter-16-memory-mapped-peripherals/) 级 MMIO + [Ch13](../chapter-13-subroutines-stacks/) 栈 |

**现实：** 90% 用 **C**；热点与 **ABI 边界** 用 **asm 补丁**。

---

### 本章两条路

```
内联汇编 (Inline)     — C 函数体内嵌几条 asm
内嵌汇编 (Embedded)   — C 文件里写完整 asm 函数（带 C 原型）
        +
独立 .s 模块（概念同 Embedded，链接器合并 — 本书侧重前两法）
```

**共同底线：** [Ch13 §13.5 AAPCS](../chapter-13-subroutines-stacks/notes/section-13-5-apcs.md) — **互调必须遵守**。

---

### 与 Ch17 工具链

| 场景 | 编译选项 |
|------|----------|
| **M4 全 Thumb-2** | **`-mthumb`** — [Ch17](../chapter-17-arm-thumb-thumb2-instructions/) |
| **ARM7 混编** | **`-minterwork`** + Veneer |
| **Embedded 函数** | 可 **ARM 或 Thumb** — 书中允许同模块混用 |

---

### 可复述要点

1. 混编 = **C 生产力 + asm 控制力**。  
2. **短 → inline；长/全指令 → embedded 或 .S**。  
3. **AAPCS 是契约** — 违反 = 随机崩溃。
