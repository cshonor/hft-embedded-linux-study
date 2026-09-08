# 附录 C · ARM 汇编残留（原第 3 章）

> **本书主线不在这里。** 原 3.1–3.5 / 3.9（指令/寻址/伪指令/异常）已物理删除，[3.6 内联汇编](../02-ch2-gnu-c-advanced/3.6-mixed-programming) 归入主线一、[3.7 GNU ARM 工具链](../06-ch6-toolchain-custom/3.7-gnu-arm) 归入主线二。
> 本目录只剩 [3.8 AArch64](./3.8-aarch64/3.8-AArch64拓展.md)（选读）。

**ARM Architecture and Assembly**

> 🗑️ **本章已大幅瘦身（2026-09-08）**
>
> **删除**：3.1 ARM 体系结构（模式/CPSR/寄存器组）、3.2 汇编指令集、3.3 寻址方式、3.4 伪指令、3.5 汇编程序设计、3.9 异常与中断汇编
> —— 这些是**纯硬件/纯汇编**内容，CSAPP ch3/4（x86-64）与 ARM 汇编资料覆盖，且**不能改造成 C 语言视角**，按"别的书讲过的不重复"原则删除。
> 需要回看：`git log --diff-filter=D --name-only -- 10-arm-asm-reference/`。
>
> **保留并提到 A 档**：**3.6 C/汇编混合编程**（`__asm__ __volatile__` 的操作数约束、clobber、volatile 语义全是**编译器层面**知识 = GNU C 扩展）、**3.7 GNU ARM 工具链**（`.section` 与链接脚本联动、读懂 `objdump -dS`）、**3.8 AArch64**（选读）。
>
> 取舍依据见 [00 · 本书取舍与补写顺序](../00-ROADMAP-本书取舍与补写顺序.md)。

## 本章目标

从 **C 程序员视角**打通汇编这一层：**C/汇编混合编程（ATPCS/AAPCS）**、**GNU ARM 汇编语法**（`.section` 与链接脚本），能用 `objdump -dS` 读懂反汇编并验证编译器优化。补充 **AArch64** 寄存器差异。

> 本章不教 ARM 指令集本身——指令集是硬件知识，已删除。
> 本章教的是**「C 代码怎么跟汇编打交道」**，这属于 GNU C 扩展的一部分。

## 前置依赖

| 章节 | 内容 |
|------|------|
| **[ch01](../06-ch6-toolchain-custom/1-toolchain)** | `gcc`、`gdb`、`objdump`、`make`、交叉编译概念 |
| **[10.8 嵌入式 C 开门](../03-ch3-embedded-driver/10.8-register/10.8-寄存器操作.md)** | 读得懂 MMIO 的 C 表达；原 ch02 硬件内容已删 |

## 环境

- **ARM32 交叉工具链**：`arm-linux-gnueabihf-gcc` / `as` / `ld` / `objdump`
- **AArch64（拓展）**：`aarch64-linux-gnu-gcc`
- 本地 x86 可用 `demo/` 验证构建流程；真 ARM 反汇编须交叉编译或 ARM 板/QEMU

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-embedded-kernel-practice/10-arm-asm-reference/demo

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

## 保留下来的三块（都是 C 语言视角）

| 模块 | 目录 | 为什么留 |
|------|------|------|
| **混合编程** | 3.6 | `__asm__ __volatile__` 是 **GNU C 扩展**，不是硬件课 |
| **GNU 语法** | 3.7 | `.section` 直接对应 [ch04 链接脚本](../06-ch6-toolchain-custom/2-compile-and-link)；读懂 `objdump -dS` 是验证 C 优化的刚需 |
| **AArch64** | 3.8 | 选读：X0–X30、无 Thumb；与 [递归栈帧实测](../../02-advanced-pointers-and-memory/ch07-functions/7.5-recursion/7.5-递归.md) 里的 AArch64 对照 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo03_mixed** | C 调 `add_asm.S`（**demo/** 已提供） | **3.6**、**3.6.1** |
| **demo04_inline** | `asm volatile` 屏障/运算（练习） | **3.6.2** |
| **demo05_objdump** | `objdump -dS` 对照栈帧 | **3.7.7** |

## 考核要点（删掉硬件后剩下的）

1. 画出 **AAPCS** 下 `add(int,int)` 栈帧（**3.6.1**）并解释 demo `add_asm` 为何可省略压栈  
2. 写一段 **`asm volatile`** 并说明 `volatile` 与 clobber（`"memory"` 为什么能阻止死存储消除）  
3. 用 **objdump -dS** 指出参数寄存器与返回寄存器  
4. 简述 **AArch64 X0–X30** 与 ARM32 差异  
5. `.section` 伪操作与链接脚本里的段如何对应（→ [ch04](../06-ch6-toolchain-custom/2-compile-and-link)）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch01** 工具链 |
| 后置 | **ch04** 编译链接；**ch05** 堆栈；**ch06** GNU C/asm；**ch10.8** 嵌入式 C 开门 |

## 小节

- ~~3.1 ARM 体系结构 / 3.2 汇编指令 / 3.3 寻址方式 / 3.4 伪指令 / 3.5 汇编程序设计 / 3.9 异常与中断汇编~~ —— **已删除**（纯硬件/纯汇编，无法改造成 C 语言视角）
- **A 档** [3.6 C语言和汇编语言混合编程](../02-ch2-gnu-c-advanced/3.6-mixed-programming/3.6-C语言和汇编语言混合编程.md)
  - [3.6.1 ATPCS规则](../02-ch2-gnu-c-advanced/3.6-mixed-programming/3.6.1-ATPCS规则.md)
  - [3.6.2 在C程序中内嵌汇编代码](../02-ch2-gnu-c-advanced/3.6-mixed-programming/3.6.2-在C程序中内嵌汇编代码.md)
  - [3.6.3 在汇编程序中调用C程序](../02-ch2-gnu-c-advanced/3.6-mixed-programming/3.6.3-在汇编程序中调用C程序.md)
- [3.7 GNU ARM汇编语言](../06-ch6-toolchain-custom/3.7-gnu-arm/3.7-GNU-ARM汇编语言.md)
  - [3.7.1 重新认识编译器](../06-ch6-toolchain-custom/3.7-gnu-arm/3.7.1-重新认识编译器.md)
  - [3.7.2 GNU ARM编译器的伪操作](../06-ch6-toolchain-custom/3.7-gnu-arm/3.7.2-GNU-ARM编译器的伪操作.md)
  - [3.7.3 GNU ARM汇编语言中的标号](../06-ch6-toolchain-custom/3.7-gnu-arm/3.7.3-GNU-ARM汇编语言中的标号.md)
  - [3.7.4 .section伪操作](../06-ch6-toolchain-custom/3.7-gnu-arm/3.7.4-section伪操作.md)
  - [3.7.5 基本数据格式](../06-ch6-toolchain-custom/3.7-gnu-arm/3.7.5-基本数据格式.md)
  - [3.7.6 数据定义](../06-ch6-toolchain-custom/3.7-gnu-arm/3.7.6-数据定义.md)
  - [3.7.7 汇编代码分析实战](../06-ch6-toolchain-custom/3.7-gnu-arm/3.7.7-汇编代码分析实战.md)
- [3.8 AArch64拓展](./3.8-aarch64/3.8-AArch64拓展.md)（选读）

---

## 代码自测

**题目 1：** 下面这段 ARM 汇编是驱动里最常见的 MMIO 读改写，对应的 C 代码是什么？（保留这道题的理由：能读懂反汇编是 C 程序员的刚需，不是 ARM 汇编课）
```c
/* ARM 汇编 */
    ldr r0, =0x1000
    ldr r1, [r0]
    add r1, r1, #1
    str r1, [r0]

/* 等价 C 代码是什么？ */
```
<details>
<summary>参考答案</summary>

等价 C 代码：
volatile int *p = (volatile int *)0x1000;
(*p)++;

逐行分析：
- ldr r0, =0x1000：将地址 0x1000 加载到 r0
- ldr r1, [r0]：从 r0 指向的地址读取值到 r1（*p）
- add r1, r1, #1：r1 = r1 + 1
- str r1, [r0]：将 r1 写回 r0 指向的地址（*p = ...）

注意 `volatile` 是必需的：没有它编译器会把两次读合并成一次、或把整个读改写优化掉。
MMIO 的完整 C 表达、`volatile` 该加在哪、`barrier()` 为什么能阻止死存储消除，见
[10.8 嵌入式 C 开门](../03-ch3-embedded-driver/10.8-register/10.8-寄存器操作.md)。

</details>

## 代码自测

**题目 1：** 嵌入式 C 自我修养这本书的学习路线是什么？为什么说它是标准 C 到内核的桥梁？

<details>
<summary>参考答案</summary>

路线：标准 C → GNU C 扩展 → 嵌入式系统编程 → 操作系统基础。桥梁作用：内核代码大量使用 __attribute__/typeof/container_of/section/weak 等 GNU 扩展，这些标准 C 教材不讲。本书填补了标准 C 到内核驱动开发之间的知识空白。

</details>
