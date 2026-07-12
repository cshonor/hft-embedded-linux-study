## Ch3 完整概述 · 指令集简介：v4T 和 v7-M

> ***ARM Assembly Language*** — William Sw Smith  
> **English:** Introduction to Instruction Sets: v4T and v7-M · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md)

---

### 一、本章核心目标

| 目标 | 说明 |
|------|------|
| **第一次写汇编** | 不罗列指令手册 — **5 个递进小程序** + 工具单步 |
| **双 ISA 习惯** | ARM7 **条件后缀** vs M4 **IT 块** + **仅 Thumb-2** |
| **整数主线** | 程序 1–3 + 编程指南 = 嵌入式 Linux 支线 **必达** |
| **浮点支线** | 程序 4–5 = M4F 预览，可 **跳过**（Ch9–11） |

**前置：** [Ch2 完整概述](../../chapter-02-programmers-model/notes/section-0-本章完整概述.md)

---

### 二、五程序地图

| # | 主题 | 关键指令/概念 | 笔记 |
|---|------|---------------|------|
| **1** | 数据移位 | `AREA`/`ENTRY`/`END` · `MOV` · `LSL` | [§3.3](./section-3-3-example-shift.md) |
| **2** | 阶乘 | `CMP` · 条件后缀 · **IT** · 循环 | [§3.4](./section-3-4-example-factorial.md) |
| **3** | 寄存器交换 | `EOR` · `LDR =` | [§3.5](./section-3-5-example-register-swap.md) |
| **4** | 浮点运算 | **CPACR** · `VMOV.F` · `VADD.F` | [§3.6](./section-3-6-example-float.md) |
| **5** | 整浮传数 | `VMOV` R↔S · `VLDR.F` | [§3.7](./section-3-7-example-int-float-xfer.md) |

**指令集对比：** [§3.2 ARM/Thumb/Thumb-2](./section-3-2-arm-thumb-compare.md)  
**习惯：** [§3.8 编程指南](./section-3-8-programming-guide.md)

---

### 三、知识流（口述版）

```
ARM 32b vs Thumb 16b vs Thumb-2 混合 → M4 只用 Thumb-2
        ↓
程序1：文件骨架 + MOV/LSL
        ↓
程序2：CMP + 条件执行(ARM) / IT(M) + 循环
        ↓
程序3：EOR 交换 + LDR 大常数
        ↓
（可选）程序4–5：FPU 使能与 V 指令
        ↓
流程图 + 初始化 → Ch4 伪指令 · Ch5 Load/Store
```

---

### 四、与后续章节

| Ch3 触点 | 展开章节 |
|----------|----------|
| 伪指令 `AREA`/`LDR=` | **Ch4** · **Ch6** 文字池 |
| `CMP`/标志/条件 | **Ch7–8** |
| `LDR`/访存 | **Ch5** |
| FPU | **Ch9–11**（跳过） |
| ARM↔Thumb 切换 | **Ch17** |
| 初始化/MMIO | **Ch16** · [20 U-Boot](../../../20-UBoot-Kernel-Build/) |

---

### 五、下一章

→ **[Ch4 汇编器规则与伪指令](../../chapter-04-assembler-rules-directives/)**（**精读**）
