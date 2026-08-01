# 第 3 章 ARM 体系结构与汇编语言

**ARM Architecture and Assembly**

## 本章目标

掌握 **ARM32 load-store 体系**、常用指令与寻址、GNU 汇编程序结构、**C/汇编混合编程（AAPCS）**，并能用 `objdump -dS` 读懂反汇编。了解 **AArch64** 寄存器与 **异常现场保存**，为内核启动、驱动 IRQ、DPDK ARM 平台优化和 **ch04–ch06** 打底。

## 前置依赖

| 章节 | 内容 |
|------|------|
| **[ch01](../ch01-tools-of-the-trade/)** | `gcc`、`gdb`、`objdump`、`make`、交叉编译概念 |
| **[ch02](../ch02-computer-architecture-and-cpu/)** | ISA、load/store、流水线、Cache、MMIO、大小端 |

## 环境

- **ARM32 交叉工具链**：`arm-linux-gnueabihf-gcc` / `as` / `ld` / `objdump`
- **AArch64（拓展）**：`aarch64-linux-gnu-gcc`
- 本地 x86 可用 `demo/` 验证构建流程；真 ARM 反汇编须交叉编译或 ARM 板/QEMU

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Embedded-C-Self-Cultivation/ch03-arm-architecture-and-assembly/demo

# 本地构建（x86 或本机 gcc）
make clean && make && ./demo03

# ARM 交叉（在已安装工具链的主机上）
export CROSS_COMPILE=arm-linux-gnueabihf-
make clean && make
${CROSS_COMPILE}objdump -dS demo03

# 对照 add_asm 与 AAPCS
${CROSS_COMPILE}objdump -d add_asm.o
make clean
```

## 七大知识模块

| 模块 | 目录 | 核心 |
|------|------|------|
| **1 ARM 体系结构** | 3.1 | RISC load-store、7 模式、CPSR/SPSR、R0–R15 |
| **2 汇编指令** | 3.2 | ldr/str/mov/add/cmp/bl/stm、条件执行 |
| **3 寻址方式** | 3.3 | 七种寻址、栈与批量传送 |
| **4 伪指令** | 3.4 | LDR=、ADR |
| **5 程序设计** | 3.5 | 段、标号、伪操作 |
| **6 混合编程** | 3.6 | ATPCS 栈帧、inline asm volatile |
| **7 GNU 语法** | 3.7 | .section、literal、objdump 实战 |

**拓展**：**3.8** AArch64（X0–X30、无 Thumb）；**3.9** 异常与中断汇编（ch10 预备）。

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_ldr_str** | 内存读写与寻址（练习） | **3.2.1**、**3.3** |
| **demo02_branch** | cmp + b/beq 分支（练习） | **3.2.5**、**3.2.7** |
| **demo03_mixed** | C 调 `add_asm.S`（**demo/** 已提供） | **3.6**、**3.6.1** |
| **demo04_inline** | `asm volatile` 屏障/运算（练习） | **3.6.2** |
| **demo05_objdump** | `objdump -dS` 对照栈帧 | **3.7.7** |
| **demo06_vector** | 异常向量表骨架（练习） | **3.9** |

## 考核要点

1. 说明 **load-store** 与 CISC 访存差异；列出 **ARM32 七种模式** 及 CPSR 作用  
2. 画出 **AAPCS** 下 `add(int,int)` 栈帧（**3.6.1**）并解释 demo `add_asm` 为何可省略压栈  
3. 手写 **ldr/str/mov/add/cmp/bl** 片段，解释 `[r1, r2, lsl #2]`  
4. 区分 **LDR 伪指令** 与 **ADR**；何时需要 `.ltorg`  
5. 写一段 **asm volatile** 并说明 `volatile` 与 clobber  
6. 用 **objdump -dS** 指出参数寄存器与返回寄存器  
7. 简述 **AArch64 X0–X30** 与 ARM32 差异；口述 IRQ **上下文保存** 步骤  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch01** 工具链、**ch02** 体系结构 |
| 后置 | **ch04** 编译链接；**ch05** 堆栈；**ch06** GNU C/asm；**ch10** 中断与 OS |

## 小节

- [3.1 ARM体系结构](./3.1-ARM体系结构.md)
- [3.2 ARM汇编指令](./3.2-instructions/3.2-ARM汇编指令.md)
  - [3.2.1 存储访问指令](./3.2-instructions/3.2.1-存储访问指令.md)
  - [3.2.2 数据传送指令](./3.2-instructions/3.2.2-数据传送指令.md)
  - [3.2.3 算术逻辑运算指令](./3.2-instructions/3.2.3-算术逻辑运算指令.md)
  - [3.2.4 操作数：operand2详解](./3.2-instructions/3.2.4-操作数-operand2详解.md)
  - [3.2.5 比较指令](./3.2-instructions/3.2.5-比较指令.md)
  - [3.2.6 条件执行指令](./3.2-instructions/3.2.6-条件执行指令.md)
  - [3.2.7 跳转指令](./3.2-instructions/3.2.7-跳转指令.md)
- [3.3 ARM寻址方式](./3.3-addressing/3.3-ARM寻址方式.md)
  - [3.3.1 寄存器寻址](./3.3-addressing/3.3.1-寄存器寻址.md)
  - [3.3.2 立即数寻址](./3.3-addressing/3.3.2-立即数寻址.md)
  - [3.3.3 寄存器偏移寻址](./3.3-addressing/3.3.3-寄存器偏移寻址.md)
  - [3.3.4 寄存器间接寻址](./3.3-addressing/3.3.4-寄存器间接寻址.md)
  - [3.3.5 基址寻址](./3.3-addressing/3.3.5-基址寻址.md)
  - [3.3.6 多寄存器寻址](./3.3-addressing/3.3.6-多寄存器寻址.md)
  - [3.3.7 相对寻址](./3.3-addressing/3.3.7-相对寻址.md)
- [3.4 ARM伪指令](./3.4-pseudo-instructions/3.4-ARM伪指令.md)
  - [3.4.1 LDR伪指令](./3.4-pseudo-instructions/3.4.1-LDR伪指令.md)
  - [3.4.2 ADR伪指令](./3.4-pseudo-instructions/3.4.2-ADR伪指令.md)
- [3.5 ARM汇编程序设计](./3.5-asm-design/3.5-ARM汇编程序设计.md)
  - [3.5.1 ARM汇编程序格式](./3.5-asm-design/3.5.1-ARM汇编程序格式.md)
  - [3.5.2 符号与标号](./3.5-asm-design/3.5.2-符号与标号.md)
  - [3.5.3 伪操作](./3.5-asm-design/3.5.3-伪操作.md)
- [3.6 C语言和汇编语言混合编程](./3.6-mixed-programming/3.6-C语言和汇编语言混合编程.md)
  - [3.6.1 ATPCS规则](./3.6-mixed-programming/3.6.1-ATPCS规则.md)
  - [3.6.2 在C程序中内嵌汇编代码](./3.6-mixed-programming/3.6.2-在C程序中内嵌汇编代码.md)
  - [3.6.3 在汇编程序中调用C程序](./3.6-mixed-programming/3.6.3-在汇编程序中调用C程序.md)
- [3.7 GNU ARM汇编语言](./3.7-gnu-arm/3.7-GNU-ARM汇编语言.md)
  - [3.7.1 重新认识编译器](./3.7-gnu-arm/3.7.1-重新认识编译器.md)
  - [3.7.2 GNU ARM编译器的伪操作](./3.7-gnu-arm/3.7.2-GNU-ARM编译器的伪操作.md)
  - [3.7.3 GNU ARM汇编语言中的标号](./3.7-gnu-arm/3.7.3-GNU-ARM汇编语言中的标号.md)
  - [3.7.4 .section伪操作](./3.7-gnu-arm/3.7.4-section伪操作.md)
  - [3.7.5 基本数据格式](./3.7-gnu-arm/3.7.5-基本数据格式.md)
  - [3.7.6 数据定义](./3.7-gnu-arm/3.7.6-数据定义.md)
  - [3.7.7 汇编代码分析实战](./3.7-gnu-arm/3.7.7-汇编代码分析实战.md)
- [3.8 AArch64拓展](./3.8-aarch64/3.8-AArch64拓展.md)
- [3.9 异常与中断汇编](./3.9-exception/3.9-异常与中断汇编.md)
