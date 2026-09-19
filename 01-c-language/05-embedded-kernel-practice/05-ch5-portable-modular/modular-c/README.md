# 参考 · 模块化编程（原第 9 章）

**Modular Programming in C for Embedded, Kernel & AIoT**

## 本章目标

建立 **高内聚、低耦合、单一职责** 的 C 模块化完整图景：从 **编译链接**（`.o`/`.a`）到 **系统分层**（app/middleware/driver/platform/utils），从 **`.h`/`.c` 封装**（include guard、不透明类型、`static`）到 **头文件纪律**与 **模块间解耦**（ops、weak、回调、统一 `err_t`）。能独立搭建 **Makefile 多模块** 与 **CMake** 工程，理解 Linux 内核 **include/** 组织；为 **ch10 终章综合项目** 提供目录与构建骨架。

## 前置依赖

| 章节 | 内容 |
|------|------|
| **[ch01](../../06-ch6-toolchain-custom/toolchain)** | Makefile、CMake、Git |
| **[ch04](../../06-ch6-toolchain-custom/compile-and-link)** | 静/动态库、链接、符号 |
| **[ch06](../../01-ch1-gnu-c-basics)** | **`weak`/`alias`**、宏 |
| **[ch07](../data-and-pointers)** | 结构体、指针 |
| **[ch08](../oop-in-c)** | 不透明类型、**ops/vtable**、分层 |

## 环境

- **编译器**：GCC 或 Clang，**`-std=gnu11 -Wall -Wextra`**
- **构建**：`make`、`ar`；可选 **CMake ≥ 3.16**
- **工具**：`nm`（查看 `.a` 导出符号）、`tree`
- **交叉编译**：`arm-none-eabi-gcc` + `CMAKE_TOOLCHAIN_FILE`（**5.19**、**demo03_cmake**）
- **拓展**：**demo/**（见下）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-embedded-kernel-practice/08-modular-c/demo

make all

./demo01_minimal/demo01_minimal
./demo02_make/demo02_app
make mod_uart

make demo03
./demo03_cmake/build/demo03_cmake

./demo04_weak/demo04_weak
./demo04_weak/demo04_weak_board
./demo05_log_err/demo05_log_err
./demo06_callback/demo06_callback

make clean
```

## 七大知识模块

| 模块 | 目录 | 核心 |
|------|------|------|
| **1 编译链接** | **5.19** | 翻译单元、符号、`.a`、增量构建、install |
| **2 系统划分** | **5.20**、**5.20.1–5.20.3** | 五层架构、划分方法、目录树、config/gitignore |
| **3 模块封装** | **5.21** | `.h`/`.c`、guard、opaque、`static`、配对 API |
| **4 头文件** | **5.22**、**5.22.1–5.22.9** | 声明/定义、前向声明、路径、内核 include、inline |
| **5 设计原则** | **5.23**、**5.24** | SOLID C 化、pitfalls、goto 错误路径 |
| **6 模块通信** | **5.25**、**5.25.1–5.25.3** | 全局变量、回调、异步队列 |
| **7 进阶与 AIoT** | **5.26**、**5.26.1–5.26.2**、**5.27** | 跨平台 weak、框架接入、云边端模块图 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_minimal** | app/driver/utils 最小模块化 | **5.20.3**、**5.21** |
| **demo02_make** | 根 Makefile + `mod_uart` 子 Makefile → `.a` | **5.19**、**5.20.3** |
| **demo03_cmake** | `add_library`、`target_link_libraries`、交叉编译 | **5.19** |
| **demo04_weak** | platform weak 默认 + BSP 强符号覆盖 | **5.26.1**、**ch06 2.4** |
| **demo05_log_err** | `LOG_INFO`/`LOG_ERR`、`err_t`/`err_str` | **5.23** |
| **demo06_callback** | driver 事件 → app 注册回调解耦 | **5.25.2**、**ch08 ops** |

## 考核要点

1. 画出 **编译 → 链接** 流程，说明 `.a` 与多个 `.o` 的关系（**5.19**）
2. 设计 **app/middleware/driver/platform/utils** 五层目录，标注依赖方向（**5.20**）
3. 写出 **不透明类型** 模块模板：`typedef` + 仅 `.c` 可见 struct（**5.21**、**ch08 5.16.1**）
4. 解释头文件 **`extern` 声明 vs 全局定义**，避免 `multiple definition`（**5.22.3–5.22.4**）
5. 用 **前向声明** 打破 `a.h` ↔ `b.h` 循环依赖（**5.22.5**）
6. 编写 **子目录 Makefile** 产出 `libmod_uart.a` 并 `make install`（**5.20.3**、**demo02_make**）
7. 用 **CMake** 声明静态库并链接到可执行文件（**demo03_cmake**）
8. 对比 **全局变量**、**回调**、**消息队列** 的耦合度与适用场景（**5.25**）
9. 说明 **weak 符号** 如何实现跨平台 platform 而不改 app（**5.26.1**、**demo04_weak**）
10. 列举模块化 **四大 pitfalls**（循环依赖、全局变量、头文件膨胀、增量构建失效）及对策（**5.23**）
11. 描述 Linux 内核 **`include/linux/` vs 驱动私有头** 的分工（**5.22.8**）
12. 说明 **ch09** 如何为 **ch10 终章项目** 提供构建与解耦基础（**5.27**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch01** 类型；**ch04** 链接属性；**ch06** weak；**ch08** ops/分层 |
| 后置 | **ch10** OS/驱动并发与终章综合项目 |

## 小节

- [5.19 模块的编译和链接](./5.19-模块的编译和链接.md)
- [5.20 系统模块划分](./5.20-module-division/5.20-系统模块划分.md)
  - [5.20.1 模块划分方法](./5.20-module-division/5.20.1-模块划分方法.md)
  - [5.20.2 面向对象编程的思维陷阱](./5.20-module-division/5.20.2-面向对象编程的思维陷阱.md)
  - [5.20.3 规划合理的目录结构](./5.20-module-division/5.20.3-规划合理的目录结构.md)
- [5.21 一个模块的封装](./5.21-一个模块的封装.md)
- [5.22 头文件深度剖析](./5.22-header-files/5.22-头文件深度剖析.md)
  - [5.22.1 基本概念](./5.22-header-files/5.22.1-基本概念.md)
  - [5.22.2 隐式声明](./5.22-header-files/5.22.2-隐式声明.md)
  - [5.22.3 变量的声明与定义](./5.22-header-files/5.22.3-变量的声明与定义.md)
  - [5.22.4 如何区分定义和声明](./5.22-header-files/5.22.4-如何区分定义和声明.md)
  - [5.22.5 前向引用和前向声明](./5.22-header-files/5.22.5-前向引用和前向声明.md)
  - [5.22.6 定义与声明的一致性](./5.22-header-files/5.22.6-定义与声明的一致性.md)
  - [5.22.7 头文件路径](./5.22-header-files/5.22.7-头文件路径.md)
  - [5.22.8 Linux内核中的头文件](./5.22-header-files/5.22.8-Linux内核中的头文件.md)
  - [5.22.9 头文件中的内联函数](./5.22-header-files/5.22.9-头文件中的内联函数.md)
- [5.23 模块设计原则](./5.23-模块设计原则.md)
- [5.24 被误解的关键字：goto](./5.24-被误解的关键字-goto.md)
- [5.25 模块间通信](./5.25-inter-module/5.25-模块间通信.md)
  - [5.25.1 全局变量](./5.25-inter-module/5.25.1-全局变量.md)
  - [5.25.2 回调函数](./5.25-inter-module/5.25.2-回调函数.md)
  - [5.25.3 异步通信](./5.25-inter-module/5.25.3-异步通信.md)
- [5.26 模块设计进阶](./5.26-advanced/5.26-模块设计进阶.md)
  - [5.26.1 跨平台设计](./5.26-advanced/5.26.1-跨平台设计.md)
  - [5.26.2 框架](./5.26-advanced/5.26.2-框架.md)
- [5.27 AIoT时代的模块化编程](./5.27-AIoT时代的模块化编程.md)


---

## 代码自测

**题目 1：** 以下模块设计有什么问题？如何改进？
```c
// utils.h
int counter = 0;  // 全局变量定义在头文件中
void inc(void);

// utils.c
#include "utils.h"
void inc(void) { counter++; }

// a.c
#include "utils.h"
void func_a(void) { inc(); }

// b.c
#include "utils.h"
void func_b(void) { inc(); }

// main.c
#include "utils.h"
int main(void) { func_a(); func_b(); return counter; }
```
<details>
<summary>参考答案</summary>

问题：int counter = 0; 定义在头文件中，被 a.c、b.c、main.c 三个文件包含后，产生多个 counter 定义，链接时报 multiple definition of counter 错误。

改进：
1. 头文件中只声明，不定义：extern int counter;
2. 在 utils.c 中定义：int counter = 0;
3. 或者用 static 限制作用域（如果每个文件需要独立的 counter）

正确的模块设计原则：
1. 头文件只放声明（extern 变量、函数原型、类型定义、宏）
2. .c 文件放定义（变量定义、函数实现）
3. 使用 static 隐藏模块内部细节（信息封装）
4. 头文件加 #ifndef 头文件守卫防止重复包含

这是 ch09 模块化编程的核心——头文件只声明，源文件才定义。

</details>

## 代码自测

**题目 1：** 嵌入式 C 自我修养这本书的学习路线是什么？为什么说它是标准 C 到内核的桥梁？

<details>
<summary>参考答案</summary>

路线：标准 C → GNU C 扩展 → 嵌入式系统编程 → 操作系统基础。桥梁作用：内核代码大量使用 __attribute__/typeof/container_of/section/weak 等 GNU 扩展，这些标准 C 教材不讲。本书填补了标准 C 到内核驱动开发之间的知识空白。

</details>
