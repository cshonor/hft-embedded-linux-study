# 《ARM Assembly Language: Fundamentals and Techniques》全书总结（第 2 版）

> **作者：** William **Hohl**、Christopher **Hinds**（书名全称常作 *ARM Assembly Language Fundamentals and Techniques*, 2nd ed）  
> **本仓库目录：** [arm32-asm/](./) · 裁剪标签见 [OUTLINE.md](./OUTLINE.md)  
> **架构范围：** ARMv4T（ARM7TDMI）+ ARMv7-M（Cortex-M）为主；**不是**树莓派 5 / Linux 用的 **AArch64** 主教材  
> **AArch64：** → [../aarch64-practice/](../aarch64-practice/)

> **作者更正：** 目录里旧标签曾写 “William Sw Smith”；章节结构与 Keil/CCS/Tiva/浮点专章对应的是 **Hohl & Hinds** 本书。下文以正确书目为准。

---

## 一、书籍定位

| 项 | 内容 |
|----|------|
| 角色 | 嵌入式 / CS 的 ARM **汇编教材 + 工程师手册** |
| 覆盖 | ARM7TDMI、Cortex-A/R/M 叙事；指令以 **v4T、v7-M** 为主线 |
| 特色 | 经典 ARM7 + 现代 Cortex；**IEEE754 浮点汇编**；Keil MDK / TI CCS；Tiva 等板级样例；异常→外设→C/汇编混合闭环 |

**对本仓库嵌入式 Linux（Pi5）的用法：**  
吃透 **Load/Store、栈、调用约定、MMIO、C↔汇编**（精读章）；浮点 / Keil·CCS 附录可跳；板上 Linux 再转 **A64 + DT + 驱动**。

---

## 二、18 章分层梳理（对齐本目录）

### 第 1 章 · 计算系统与 RISC（选读）

[chapter-01-overview-computing-systems/](./chapter-01-overview-computing-systems/)

- RISC vs CISC；ARM 产品线 → Cortex-A（跑 Linux）/ R（实时）/ M（MCU）  
- 数制、补码、ASCII；存储程序模型；GCC/GDB、Keil、CCS 工具链一瞥  

### 第 2 章 · 程序员模型（精读）

[chapter-02-programmers-model/](./chapter-02-programmers-model/)

| | **ARM7TDMI** | **Cortex-M4** |
|--|--------------|---------------|
| 模式 | 7 模式（User/Sys/SVC/IRQ/FIQ/Abort/Undef） | Thread / Handler |
| 寄存器 | 分组银行（含 SPSR 等） | r0–r15、xPSR（APSR/IPSR/EPSR） |
| 栈 | 模式相关 SP | **MSP / PSP** 双栈 |
| 中断 | 向量表（入口多为指令） | **NVIC**；向量表存**函数地址** |
| 其他 | CPSR 条件标志、T 位 | 可选 FPU S0–S31、MPU |

### 第 3 章 · 指令集入门 v4T & v7-M（精读）

[chapter-03-instruction-sets-v4t-v7m/](./chapter-03-instruction-sets-v4t-v7m/)

ARM 32 / Thumb 16 / Thumb-2 混合；样例（移位、阶乘、交换、浮点、int↔FP）；条件执行 vs **IT** 块。

### 第 4 章 · 伪指令与语法（精读）

[chapter-04-assembler-rules-directives/](./chapter-04-assembler-rules-directives/)

AREA/.sect、EQU、DCB/DCW/DCD、SPACE、LTORG、宏；Keil vs CCS 语法对照。伪指令 **不生成机器码**，只指导汇编器。

### 第 5 章 · Load/Store（精读 · RISC 核心）

[chapter-05-loads-stores-addressing/](./chapter-05-loads-stores-addressing/)

算术只碰寄存器；内存必须 LDR/STR 族；预/后索引与 `!`；大小端；Cortex-M **bit-band**；LDM/STM（栈底层）。

### 第 6 章 · 常量与文字池（选读）

[chapter-06-constants-literal-pools/](./chapter-06-constants-literal-pools/)

可编码立即数限制；`LDR` 伪指令 + literal pool；`LTORG` 防偏移超限。

### 第 7–8 章 · 整数运算 · 分支循环（精读）

[chapter-07](./chapter-07-integer-logic-arithmetic/) · [chapter-08](./chapter-08-branches-loops/)

N/Z/C/V；ADD/SUB/AND/ORR/EOR、移位、MUL/饱和；B/BL/BX、条件分支、循环；ARM 条件后缀 vs M 的 IT。

### 第 9–11 章 · IEEE754 浮点（多数跳过）

[chapter-09](./chapter-09-floating-point-basics/) · [10](./chapter-10-floating-point-rounding-exceptions/) · [11](./chapter-11-floating-point-data-processing/)

格式、特殊值、舍入/异常、FPU 指令。Linux 应用核浮点另议；MCU/DSP 需要再精读。

### 第 12 章 · 查表与数组（选读）

[chapter-12-tables/](./chapter-12-tables/)

### 第 13 章 · 子程序与栈（精读）

[chapter-13-subroutines-stacks/](./chapter-13-subroutines-stacks/)

PUSH/POP ↔ LDM/STM；**AAPCS**：R0–R3 传参、LR 返回；局部变量与现场保存。

### 第 14–15 章 · 异常（选读）

[chapter-14 ARM7](./chapter-14-exception-handling-arm7tdmi/) · [chapter-15 v7-M/NVIC](./chapter-15-exception-handling-v7m/)

ARM7：复位/SVC/Abort/IRQ/FIQ、SPSR。  
Cortex-M：向量表地址、HardFault、SysTick、PendSV、栈切换。  
→ Linux 板级对照：异常模型不同，概念可映射到 [aarch64-practice](../aarch64-practice/) / GIC。

### 第 16 章 · MMIO 外设（精读）

[chapter-16-memory-mapped-peripherals/](./chapter-16-memory-mapped-peripherals/)

UART/ADC/定时器/GPIO 寄存器汇编读写（Tiva/LPC 例）。思维迁到 Linux：**驱动 + DT 描述 MMIO**，不是裸机直接硬编址量产路径。

### 第 17 章 · ARM / Thumb / Thumb-2（选读）

[chapter-17-arm-thumb-thumb2-instructions/](./chapter-17-arm-thumb-thumb2-instructions/)

### 第 18 章 · C 与汇编混合（精读）

[chapter-18-mixing-c-and-assembly/](./chapter-18-mixing-c-and-assembly/)

内联汇编；C↔asm 调用；全局符号。接 [01 C](../../01-c-language/) 与内核/驱动互调。

### 附录

| 附录 | 本仓库 | 标签 |
|------|--------|------|
| A CCS | [appendix-A](./appendix-A-code-composer-studio/) | 跳过（Linux/GCC 路线） |
| B Keil | [appendix-B](./appendix-B-keil-tools/) | 跳过 |
| C ASCII | [appendix-C](./appendix-C-ascii-character-codes/) | 选读 |
| D 向量表示例 | [appendix-D](./appendix-D/) | 选读 |

---

## 三、设计思路（书方）

1. 底层架构 → 指令语法 → 外设 / 混合编程  
2. 样例 + IDE 调试，不只背助记符  
3. 新旧架构对照（维护老 ARM7 + 写 Cortex-M）  
4. 补齐他书常缺的浮点汇编与板级 MMIO  

---

## 四、核心知识点提炼

1. **RISC：** 计算在寄存器；内存只经 Load/Store  
2. **ARM7 ≠ Cortex-M：** 模式、向量表、指令集不可当同一套抄  
3. **汇编三件套：** 寄存器运算 · 寻址 · 分支/循环  
4. Cortex-M：bit-band、NVIC、可选 FPU — **MCU 主流**；跑 Linux 的是 **A 系列 / AArch64**  
5. 无 FPU 用定点；有 FPU 跟 IEEE754  
6. 子程序：**LR** + 栈保存现场  
7. 伪指令 ≠ 机器指令  
8. 混合编程遵守 **AAPCS**（参数/返回值/栈）

---

## 五、与本仓库阅读优先级（嵌入式 Linux）

| 优先级 | 章 | 目的 |
|--------|-----|------|
| **精读** | 2–5、7–8、13、16、18 | 程序员模型、LDR/STR、运算分支、栈、MMIO、C 互调 |
| **选读** | 1、6、12、14–15、17 | 背景、文字池、查表、异常细节、Thumb |
| **跳过（多数）** | 9–11、附录 A/B | 浮点专章、Keil/CCS 工程步骤 |

精读完 → [aarch64-practice](../aarch64-practice/)（Pi5）→ [12 驱动·DT](../../12-device-drivers-dt/) · [Primer](../../11-embedded-boot-build/primer-system-overview/)。

---

## 参考

- Hohl & Hinds, *ARM Assembly Language Fundamentals and Techniques*, 2nd ed  
- 分章笔记入口：[OUTLINE.md](./OUTLINE.md) · [README.md](./README.md)
