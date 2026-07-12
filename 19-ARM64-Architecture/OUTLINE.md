# ARM Assembly Language — 章节目录与阅读裁剪

> **书目：** *ARM Assembly Language* — **William Sw Smith**  
> **模块：** [19-ARM64-Architecture/](./README.md) · 嵌入式支线 **汇编思维入门**  
> **架构说明：** 本书正文以 **ARM v4T（ARM7TDMI）** 与 **v7-M（Cortex-M）** 为主 — **不是** AArch64 专用教材；学的是 **汇编思维、Load/Store、栈、异常、MMIO、C/汇编互调**。AArch64 实战见 [**《ARM64体系结构编程与实践》**](./arm64-programming-practice/OUTLINE.md)。

| 标签 | 含义 |
|------|------|
| **精读** | 嵌入式 Linux 支线必看 |
| **选读** | 有上下文价值；时间紧可后补 |
| **跳过** | 与 Linux/GCC 路线无关或本书工具链专用 |

---

## 全书结构（四部分 · 18 章）

| 部分 | 章 | 主题 |
|------|-----|------|
| **一** | 1–2 | 基础概念与程序员模型 |
| **二** | 3–8 | 汇编语言与指令集核心 |
| **三** | 9–11 | 浮点运算（Cortex-M4F 等） |
| **四** | 12–18 | 高级编程 · 硬件 · 混合编程 |

---

## 第一部分：基础概念与程序员模型

### 第 1 章 · 计算机系统概述 · **选读**

[chapter-01-overview-computing-systems/](./chapter-01-overview-computing-systems/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **1.1** | 简介 | [§1.1](./chapter-01-overview-computing-systems/notes/section-1-1-intro.md) |
| **1.2** | RISC 历史 — ARM 起源 · 公司创建 · 现状 · Cortex A/R/M 系列 | [§1.2](./chapter-01-overview-computing-systems/notes/section-1-2-risc-history.md) |
| **1.3** | 计算设备 | [§1.3](./chapter-01-overview-computing-systems/notes/section-1-3-computing-devices.md) |
| **1.4** | 数字系统 | [§1.4](./chapter-01-overview-computing-systems/notes/section-1-4-number-systems.md) |
| **1.5** | 数字与字符的表示 — 整数 · 浮点 · 字符 | [§1.5](./chapter-01-overview-computing-systems/notes/section-1-5-representation.md) |
| **1.6** | 将比特翻译为命令 | [§1.6](./chapter-01-overview-computing-systems/notes/section-1-6-bits-to-commands.md) |
| **1.7** | 工具 — 开源 · Keil · Code Composer Studio | [§1.7](./chapter-01-overview-computing-systems/notes/section-1-7-tools.md) |
| **1.8** | 练习题 | [§1.8](./chapter-01-overview-computing-systems/notes/section-1-8-exercises.md) |

### 第 2 章 · 程序员模型 · **精读**

[chapter-02-programmers-model/](./chapter-02-programmers-model/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **2.1** | 简介 | [§2.1](./chapter-02-programmers-model/notes/section-2-1-intro.md) |
| **2.2** | 数据类型 | [§2.2](./chapter-02-programmers-model/notes/section-2-2-data-types.md) |
| **2.3** | ARM7TDMI — 处理器模式 · 寄存器组织 · 向量表 | [§2.3](./chapter-02-programmers-model/notes/section-2-3-arm7tdmi.md) |
| **2.4** | Cortex-M4 — 处理器模式 · 寄存器组织 · 向量表 | [§2.4](./chapter-02-programmers-model/notes/section-2-4-cortex-m4.md) |
| **2.5** | 练习题 | [§2.5](./chapter-02-programmers-model/notes/section-2-5-exercises.md) |

---

## 第二部分：汇编语言与指令集核心

### 第 3 章 · 指令集简介：v4T 和 v7-M · **精读**

[chapter-03-instruction-sets-v4t-v7m/](./chapter-03-instruction-sets-v4t-v7m/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **3.1** | 简介 | [§3.1](./chapter-03-instruction-sets-v4t-v7m/notes/section-3-1-intro.md) |
| **3.2** | ARM、Thumb 和 Thumb-2 指令对比 | [§3.2](./chapter-03-instruction-sets-v4t-v7m/notes/section-3-2-arm-thumb-compare.md) |
| **3.3** | 示例程序 1 — 数据移位 | [§3.3](./chapter-03-instruction-sets-v4t-v7m/notes/section-3-3-example-shift.md) |
| **3.4** | 示例程序 2 — 阶乘计算 | [§3.4](./chapter-03-instruction-sets-v4t-v7m/notes/section-3-4-example-factorial.md) |
| **3.5** | 示例程序 3 — 寄存器交换 | [§3.5](./chapter-03-instruction-sets-v4t-v7m/notes/section-3-5-example-register-swap.md) |
| **3.6** | 示例程序 4 — 浮点数操作 | [§3.6](./chapter-03-instruction-sets-v4t-v7m/notes/section-3-6-example-float.md) |
| **3.7** | 示例程序 5 — 整数与浮点寄存器数据传输 | [§3.7](./chapter-03-instruction-sets-v4t-v7m/notes/section-3-7-example-int-float-xfer.md) |
| **3.8** | 编程指南 | [§3.8](./chapter-03-instruction-sets-v4t-v7m/notes/section-3-8-programming-guide.md) |
| **3.9** | 练习题 | [§3.9](./chapter-03-instruction-sets-v4t-v7m/notes/section-3-9-exercises.md) |

### 第 4 章 · 汇编器规则与伪指令 · **精读**

[chapter-04-assembler-rules-directives/](./chapter-04-assembler-rules-directives/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **4.1** | 简介 | [§4.1](./chapter-04-assembler-rules-directives/notes/section-4-1-intro.md) |
| **4.2** | 汇编语言模块结构 | [§4.2](./chapter-04-assembler-rules-directives/notes/section-4-2-module-structure.md) |
| **4.3** | 预定义的寄存器名称 | [§4.3](./chapter-04-assembler-rules-directives/notes/section-4-3-register-names.md) |
| **4.4** | 常用伪指令 — Keil/CCS · 代码/数据块 · 对齐 · 文字池 | [§4.4](./chapter-04-assembler-rules-directives/notes/section-4-4-directives.md) |
| **4.5** | 宏 (Macros) | [§4.5](./chapter-04-assembler-rules-directives/notes/section-4-5-macros.md) |
| **4.6** | 汇编器杂项特性 — 操作符 · CCS 数学函数 | [§4.6](./chapter-04-assembler-rules-directives/notes/section-4-6-assembler-misc.md) |
| **4.7** | 练习题 | [§4.7](./chapter-04-assembler-rules-directives/notes/section-4-7-exercises.md) |

### 第 5 章 · 加载、存储与寻址 · **精读**

[chapter-05-loads-stores-addressing/](./chapter-05-loads-stores-addressing/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **5.1** | 简介 | [§5.1](./chapter-05-loads-stores-addressing/notes/section-5-1-intro.md) |
| **5.2** | 内存 | [§5.2](./chapter-05-loads-stores-addressing/notes/section-5-2-memory.md) |
| **5.3** | 加载与存储指令 | [§5.3](./chapter-05-loads-stores-addressing/notes/section-5-3-load-store.md) |
| **5.4** | 操作数寻址 — 前变址 · 后变址 | [§5.4](./chapter-05-loads-stores-addressing/notes/section-5-4-addressing.md) |
| **5.5** | 字节序 (Endianness) | [§5.5](./chapter-05-loads-stores-addressing/notes/section-5-5-endianness.md) |
| **5.6** | 位带内存 (Bit-Banded Memory) — Cortex-M | [§5.6](./chapter-05-loads-stores-addressing/notes/section-5-6-bit-banded.md) |
| **5.7** | 内存注意事项 | [§5.7](./chapter-05-loads-stores-addressing/notes/section-5-7-memory-notes.md) |
| **5.8** | 练习题 | [§5.8](./chapter-05-loads-stores-addressing/notes/section-5-8-exercises.md) |

### 第 6 章 · 常量与文字池 · **选读**

[chapter-06-constants-literal-pools/](./chapter-06-constants-literal-pools/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **6.1** | 简介 | [§6.1](./chapter-06-constants-literal-pools/notes/section-6-1-intro.md) |
| **6.2** | ARM 循环移位方案 — 常数编码进指令 | [§6.2](./chapter-06-constants-literal-pools/notes/section-6-2-rotate-constants.md) |
| **6.3** | 加载常量 — MOVW/MOVT | [§6.3](./chapter-06-constants-literal-pools/notes/section-6-3-load-constants.md) |
| **6.4** | 文字池 (Literal Pools) | [§6.4](./chapter-06-constants-literal-pools/notes/section-6-4-literal-pools.md) |
| **6.5** | 向寄存器加载地址 | [§6.5](./chapter-06-constants-literal-pools/notes/section-6-5-load-addresses.md) |
| **6.6** | 练习题 | [§6.6](./chapter-06-constants-literal-pools/notes/section-6-6-exercises.md) |

### 第 7 章 · 整数逻辑与算术运算 · **精读**

[chapter-07-integer-logic-arithmetic/](./chapter-07-integer-logic-arithmetic/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **7.1** | 简介 | [§7.1](./chapter-07-integer-logic-arithmetic/notes/section-7-1-intro.md) |
| **7.2** | 标志位 — N · V · Z · C | [§7.2](./chapter-07-integer-logic-arithmetic/notes/section-7-2-flags.md) |
| **7.3** | 比较指令 | [§7.3](./chapter-07-integer-logic-arithmetic/notes/section-7-3-compare.md) |
| **7.4** | 数据处理 — 布尔 · 移位 · 加减 · 饱和 · 乘除 | [§7.4](./chapter-07-integer-logic-arithmetic/notes/section-7-4-data-processing.md) |
| **7.5** | DSP 扩展 | [§7.5](./chapter-07-integer-logic-arithmetic/notes/section-7-5-dsp.md) |
| **7.6** | 位操作指令 | [§7.6](./chapter-07-integer-logic-arithmetic/notes/section-7-6-bit-ops.md) |
| **7.7** | 分数表示法 (Fractional Notation) | [§7.7](./chapter-07-integer-logic-arithmetic/notes/section-7-7-fractional.md) |
| **7.8** | 练习题 | [§7.8](./chapter-07-integer-logic-arithmetic/notes/section-7-8-exercises.md) |

### 第 8 章 · 分支与循环 · **精读**

[chapter-08-branches-loops/](./chapter-08-branches-loops/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **8.1** | 简介 | [§8.1](./chapter-08-branches-loops/notes/section-8-1-intro.md) |
| **8.2** | 分支机制 — ARM7TDMI · v7-M | [§8.2](./chapter-08-branches-loops/notes/section-8-2-branches.md) |
| **8.3** | 循环 — While · For · Do-While | [§8.3](./chapter-08-branches-loops/notes/section-8-3-loops.md) |
| **8.4** | 条件执行 — v4T 条件执行 · v7-M IT 块 | [§8.4](./chapter-08-branches-loops/notes/section-8-4-conditional.md) |
| **8.5** | 直线型编码 — 循环展开 | [§8.5](./chapter-08-branches-loops/notes/section-8-5-straight-line.md) |
| **8.6** | 练习题 | [§8.6](./chapter-08-branches-loops/notes/section-8-6-exercises.md) |

---

## 第三部分：浮点运算（Cortex-M4F 等）

### 第 9 章 · 浮点简介：基础、类型与传输 · **跳过**

[chapter-09-floating-point-basics/](./chapter-09-floating-point-basics/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **9.1–9.3** | 简介与历史 | [§9.1](./chapter-09-floating-point-basics/notes/section-9-1-intro.md) · [§9.2](./chapter-09-floating-point-basics/notes/section-9-2-history.md) · [§9.3](./chapter-09-floating-point-basics/notes/section-9-3-overview.md) |
| **9.4** | 浮点数据类型 | [§9.4](./chapter-09-floating-point-basics/notes/section-9-4-data-types.md) |
| **9.5–9.6** | 可表示的值 — 正常 · 次正常 · 零 · 无穷 · NaN | [§9.5](./chapter-09-floating-point-basics/notes/section-9-5-representable.md) · [§9.6](./chapter-09-floating-point-basics/notes/section-9-6-special-values.md) |
| **9.7** | Cortex-M4 浮点寄存器文件 | [§9.7](./chapter-09-floating-point-basics/notes/section-9-7-fp-registers.md) |
| **9.8** | FPU 控制寄存器 — FPSCR · CPACR | [§9.8](./chapter-09-floating-point-basics/notes/section-9-8-fpu-control.md) |
| **9.9–9.11** | 浮点数据传输与格式转换 | [§9.9](./chapter-09-floating-point-basics/notes/section-9-9-fp-transfer.md) · [§9.10](./chapter-09-floating-point-basics/notes/section-9-10-precision-convert.md) · [§9.11](./chapter-09-floating-point-basics/notes/section-9-11-int-float-convert.md) |
| **9.12** | 练习题 | [§9.12](./chapter-09-floating-point-basics/notes/section-9-12-exercises.md) |

### 第 10 章 · 浮点简介：舍入与异常 · **跳过**

[chapter-10-floating-point-rounding-exceptions/](./chapter-10-floating-point-rounding-exceptions/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **10.1** | 简介 | [§10.1](./chapter-10-floating-point-rounding-exceptions/notes/section-10-1-intro.md) |
| **10.2** | 舍入 — IEEE 754-2008 舍入模式 | [§10.2](./chapter-10-floating-point-rounding-exceptions/notes/section-10-2-rounding.md) |
| **10.3** | 异常 — 除零 · 无效 · 溢出 · 下溢 · 不精确 | [§10.3](./chapter-10-floating-point-rounding-exceptions/notes/section-10-3-exceptions.md) |
| **10.4** | 代数定律与浮点运算 | [§10.4](./chapter-10-floating-point-rounding-exceptions/notes/section-10-4-algebra.md) |
| **10.5** | 规格化与抵消 | [§10.5](./chapter-10-floating-point-rounding-exceptions/notes/section-10-5-normalization.md) |
| **10.6** | 练习题 | [§10.6](./chapter-10-floating-point-rounding-exceptions/notes/section-10-6-exercises.md) |

### 第 11 章 · 浮点数据处理指令 · **跳过**

[chapter-11-floating-point-data-processing/](./chapter-11-floating-point-data-processing/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **11.1–11.3** | 简介 · 指令语法 · 摘要 | [§11.1](./chapter-11-floating-point-data-processing/notes/section-11-1-intro.md) · [§11.2](./chapter-11-floating-point-data-processing/notes/section-11-2-syntax.md) · [§11.3](./chapter-11-floating-point-data-processing/notes/section-11-3-summary.md) |
| **11.4** | 标志位 — 比较指令 · N/Z/C/V | [§11.4](./chapter-11-floating-point-data-processing/notes/section-11-4-flags.md) |
| **11.5** | Flush-to-Zero · 默认 NaN 模式 | [§11.5](./chapter-11-floating-point-data-processing/notes/section-11-5-special-modes.md) |
| **11.6** | 非算术指令 — 绝对值 · 求反 | [§11.6](./chapter-11-floating-point-data-processing/notes/section-11-6-non-arithmetic.md) |
| **11.7** | 算术指令 — 加减 · 乘加 · 除法 · 平方根 | [§11.7](./chapter-11-floating-point-data-processing/notes/section-11-7-arithmetic.md) |
| **11.8** | 编码示例 | [§11.8](./chapter-11-floating-point-data-processing/notes/section-11-8-examples.md) |
| **11.9** | 练习题 | [§11.9](./chapter-11-floating-point-data-processing/notes/section-11-9-exercises.md) |

---

## 第四部分：高级编程 · 硬件 · 混合编程

### 第 12 章 · 表 · **选读**

[chapter-12-tables/](./chapter-12-tables/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **12.1** | 简介 | [§12.1](./chapter-12-tables/notes/section-12-1-intro.md) |
| **12.2** | 整数查找表 | [§12.2](./chapter-12-tables/notes/section-12-2-int-lookup.md) |
| **12.3** | 浮点查找表 | [§12.3](./chapter-12-tables/notes/section-12-3-float-lookup.md) |
| **12.4** | 二分查找 (Binary Searches) | [§12.4](./chapter-12-tables/notes/section-12-4-binary-search.md) |
| **12.5** | 练习题 | [§12.5](./chapter-12-tables/notes/section-12-5-exercises.md) |

### 第 13 章 · 子程序与堆栈 · **精读**

[chapter-13-subroutines-stacks/](./chapter-13-subroutines-stacks/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **13.1** | 简介 | [§13.1](./chapter-13-subroutines-stacks/notes/section-13-1-intro.md) |
| **13.2** | 堆栈 — LDM/STM · PUSH/POP · 满/空 · 递增/递减 | [§13.2](./chapter-13-subroutines-stacks/notes/section-13-2-stacks.md) |
| **13.3** | 子程序 | [§13.3](./chapter-13-subroutines-stacks/notes/section-13-3-subroutines.md) |
| **13.4** | 向子程序传递参数 — 寄存器 · 指针 · 堆栈 | [§13.4](./chapter-13-subroutines-stacks/notes/section-13-4-parameters.md) |
| **13.5** | ARM APCS — 应用过程调用标准 | [§13.5](./chapter-13-subroutines-stacks/notes/section-13-5-apcs.md) |
| **13.6** | 练习题 | [§13.6](./chapter-13-subroutines-stacks/notes/section-13-6-exercises.md) |

### 第 14 章 · 异常处理：ARM7TDMI · **选读**

[chapter-14-exception-handling-arm7tdmi/](./chapter-14-exception-handling-arm7tdmi/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **14.1–14.7** | 基础机制 — 中断 · 错误 · 异常序列 · 向量表 · 优先级 | [§14.1](./chapter-14-exception-handling-arm7tdmi/notes/section-14-1-intro.md) … [§14.7](./chapter-14-exception-handling-arm7tdmi/notes/section-14-7-mechanism.md) |
| **14.8** | 处理异常的程序 — 复位 · 未定义 · VIC · 中止 · SVC | [§14.8](./chapter-14-exception-handling-arm7tdmi/notes/section-14-8-handler-code.md) |
| **14.9** | 练习题 | [§14.9](./chapter-14-exception-handling-arm7tdmi/notes/section-14-9-exercises.md) |

### 第 15 章 · 异常处理：v7-M · **选读**

[chapter-15-exception-handling-v7m/](./chapter-15-exception-handling-v7m/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **15.1–15.6** | v7-M 异常架构 — 特权 · 向量表 · MSP/PSP · 出入栈 · 故障类型 | [§15.1](./chapter-15-exception-handling-v7m/notes/section-15-1-intro.md) … [§15.6](./chapter-15-exception-handling-v7m/notes/section-15-6-fault-types.md) |
| **15.7** | 中断 — 基于 NVIC 的外部中断 | [§15.7](./chapter-15-exception-handling-v7m/notes/section-15-7-nvic.md) |
| **15.8** | 练习题 | [§15.8](./chapter-15-exception-handling-v7m/notes/section-15-8-exercises.md) |

### 第 16 章 · 内存映射外设 · **精读**

[chapter-16-memory-mapped-peripherals/](./chapter-16-memory-mapped-peripherals/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **16.1** | 简介 | [§16.1](./chapter-16-memory-mapped-peripherals/notes/section-16-1-intro.md) |
| **16.2** | LPC2104 — UART 通信 | [§16.2](./chapter-16-memory-mapped-peripherals/notes/section-16-2-lpc2104-uart.md) |
| **16.3** | LPC2132 — D/A 转换器生成正弦波 | [§16.3](./chapter-16-memory-mapped-peripherals/notes/section-16-3-lpc2132-dac.md) |
| **16.4** | Tiva Launchpad — GPIO 操作 | [§16.4](./chapter-16-memory-mapped-peripherals/notes/section-16-4-tiva-gpio.md) |
| **16.5** | 练习题 | [§16.5](./chapter-16-memory-mapped-peripherals/notes/section-16-5-exercises.md) |

### 第 17 章 · ARM、Thumb 和 Thumb-2 指令 · **选读**

[chapter-17-arm-thumb-thumb2-instructions/](./chapter-17-arm-thumb-thumb2-instructions/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **17.1** | 简介 | [§17.1](./chapter-17-arm-thumb-thumb2-instructions/notes/section-17-1-intro.md) |
| **17.2** | ARM 与 16 位 Thumb 指令 | [§17.2](./chapter-17-arm-thumb-thumb2-instructions/notes/section-17-2-arm-vs-thumb16.md) |
| **17.3** | 32 位 Thumb 指令 (Thumb-2) | [§17.3](./chapter-17-arm-thumb-thumb2-instructions/notes/section-17-3-thumb2.md) |
| **17.4** | ARM 与 Thumb 状态切换 — BX 等 | [§17.4](./chapter-17-arm-thumb-thumb2-instructions/notes/section-17-4-state-switch.md) |
| **17.5** | 如何为 Thumb 编译代码 — Interworking | [§17.5](./chapter-17-arm-thumb-thumb2-instructions/notes/section-17-5-interworking.md) |
| **17.6** | 练习题 | [§17.6](./chapter-17-arm-thumb-thumb2-instructions/notes/section-17-6-exercises.md) |

### 第 18 章 · C 与汇编混合编程 · **精读**

[chapter-18-mixing-c-and-assembly/](./chapter-18-mixing-c-and-assembly/)

| 小节 | 内容 | 笔记 |
|------|------|------|
| **18.1** | 简介 | [§18.1](./chapter-18-mixing-c-and-assembly/notes/section-18-1-intro.md) |
| **18.2** | 内联汇编 (Inline Assembler) | [§18.2](./chapter-18-mixing-c-and-assembly/notes/section-18-2-inline-asm.md) |
| **18.3** | 嵌入式汇编 (Embedded Assembler) | [§18.3](./chapter-18-mixing-c-and-assembly/notes/section-18-3-embedded-asm.md) |
| **18.4** | C 与汇编相互调用 — APCS | [§18.4](./chapter-18-mixing-c-and-assembly/notes/section-18-4-c-asm-calls.md) |
| **18.5** | 练习题 | [§18.5](./chapter-18-mixing-c-and-assembly/notes/section-18-5-exercises.md) |

---

## 附录及其他

| 部分 | 英文 | 标签 | 文件夹 |
|------|------|------|--------|
| **附录 A** | Running Code Composer Studio | **跳过** | [appendix-A](./appendix-A-code-composer-studio/) |
| **附录 B** | Running Keil Tools | **跳过** | [appendix-B](./appendix-B-keil-tools/) |
| **附录 C** | ASCII Character Codes | **选读** | [appendix-C](./appendix-C-ascii-character-codes/) |
| **附录 D** | Complete Example Source Code | **选读** | [appendix-D](./appendix-D/) |
| **术语表** | Glossary | **选读** | [glossary/](./glossary/) |
| **参考文献** | References | **选读** | [references/](./references/) |

---

## 推荐阅读顺序（嵌入式 Linux 支线）

```
2  程序员模型
   ↓
3  指令集（v4T/v7-M 基础）
   ↓
4  伪指令 → 5  Load/Store → 7  整数运算 → 8  分支
   ↓
13 子程序与堆栈  ←→  18 C/汇编混合
   ↓
16 MMIO 外设
   ↓
（选读 14–15 异常概念）→ [奔跑吧 ARM64 主书](./arm64-programming-practice/) → 开 20 U-Boot / 21 驱动
```

---

## 与 HFT / MikanOS 对照

| 已学（HFT 链） | 本书对应 |
|----------------|----------|
| [01 CSAPP](../01-CSAPP-3rd/) Ch3 机器级 | 另一 ISA 的同一层思维 |
| [08 MikanOS](../08-system-low-level-hands-on/01-mikan-os/) x86 UEFI | **Ch16 MMIO** ≈ GOP 写帧缓冲 · **Ch13/18** ≈ Loader 调内核 |
| [04 LKD](../04-Linux-Kernel-Development/) 中断 | **Ch14–15** 异常概念 → [奔跑吧 Ch11–13](./arm64-programming-practice/chapter-11-exception-handling/) |

---

← [19 README](./README.md) · [奔跑吧 ARM64 OUTLINE](./arm64-programming-practice/OUTLINE.md) · 下一模块 [20 构建](../20-UBoot-Kernel-Build/)
