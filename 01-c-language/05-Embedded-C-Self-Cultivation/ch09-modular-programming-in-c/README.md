# 第 9 章 C 语言的模块化编程思想

**Modular Programming in C for Embedded, Kernel & AIoT**

## 本章目标

建立 **高内聚、低耦合、单一职责** 的 C 模块化完整图景：从 **编译链接**（`.o`/`.a`）到 **系统分层**（app/middleware/driver/platform/utils），从 **`.h`/`.c` 封装**（include guard、不透明类型、`static`）到 **头文件纪律**与 **模块间解耦**（ops、weak、回调、统一 `err_t`）。能独立搭建 **Makefile 多模块** 与 **CMake** 工程，理解 Linux 内核 **include/** 组织；为 **ch10 终章综合项目** 提供目录与构建骨架。

## 前置依赖

| 章节 | 内容 |
|------|------|
| **[ch01](../ch01-tools-of-the-trade/)** | Makefile、CMake、Git |
| **[ch04](../ch04-compile-link-install-run/)** | 静/动态库、链接、符号 |
| **[ch06](../ch06-gnu-c-extensions/)** | **`weak`/`alias`**、宏 |
| **[ch07](../ch07-data-storage-and-pointers/)** | 结构体、指针 |
| **[ch08](../ch08-oop-in-c/)** | 不透明类型、**ops/vtable**、分层 |

## 环境

- **编译器**：GCC 或 Clang，**`-std=gnu11 -Wall -Wextra`**
- **构建**：`make`、`ar`；可选 **CMake ≥ 3.16**
- **工具**：`nm`（查看 `.a` 导出符号）、`tree`
- **交叉编译**：`arm-none-eabi-gcc` + `CMAKE_TOOLCHAIN_FILE`（**9.1**、**demo03_cmake**）
- **拓展**：**demo/**（见下）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Embedded-C-Self-Cultivation/ch09-modular-programming-in-c/demo

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
| **1 编译链接** | **9.1** | 翻译单元、符号、`.a`、增量构建、install |
| **2 系统划分** | **9.2**、**9.2.1–9.2.3** | 五层架构、划分方法、目录树、config/gitignore |
| **3 模块封装** | **9.3** | `.h`/`.c`、guard、opaque、`static`、配对 API |
| **4 头文件** | **9.4**、**9.4.1–9.4.9** | 声明/定义、前向声明、路径、内核 include、inline |
| **5 设计原则** | **9.5**、**9.6** | SOLID C 化、pitfalls、goto 错误路径 |
| **6 模块通信** | **9.7**、**9.7.1–9.7.3** | 全局变量、回调、异步队列 |
| **7 进阶与 AIoT** | **9.8**、**9.8.1–9.8.2**、**9.9** | 跨平台 weak、框架接入、云边端模块图 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_minimal** | app/driver/utils 最小模块化 | **9.2.3**、**9.3** |
| **demo02_make** | 根 Makefile + `mod_uart` 子 Makefile → `.a` | **9.1**、**9.2.3** |
| **demo03_cmake** | `add_library`、`target_link_libraries`、交叉编译 | **9.1** |
| **demo04_weak** | platform weak 默认 + BSP 强符号覆盖 | **9.8.1**、**ch06 6.9** |
| **demo05_log_err** | `LOG_INFO`/`LOG_ERR`、`err_t`/`err_str` | **9.5** |
| **demo06_callback** | driver 事件 → app 注册回调解耦 | **9.7.2**、**ch08 ops** |

## 考核要点

1. 画出 **编译 → 链接** 流程，说明 `.a` 与多个 `.o` 的关系（**9.1**）
2. 设计 **app/middleware/driver/platform/utils** 五层目录，标注依赖方向（**9.2**）
3. 写出 **不透明类型** 模块模板：`typedef` + 仅 `.c` 可见 struct（**9.3**、**ch08 8.3.1**）
4. 解释头文件 **`extern` 声明 vs 全局定义**，避免 `multiple definition`（**9.4.3–9.4.4**）
5. 用 **前向声明** 打破 `a.h` ↔ `b.h` 循环依赖（**9.4.5**）
6. 编写 **子目录 Makefile** 产出 `libmod_uart.a` 并 `make install`（**9.2.3**、**demo02_make**）
7. 用 **CMake** 声明静态库并链接到可执行文件（**demo03_cmake**）
8. 对比 **全局变量**、**回调**、**消息队列** 的耦合度与适用场景（**9.7**）
9. 说明 **weak 符号** 如何实现跨平台 platform 而不改 app（**9.8.1**、**demo04_weak**）
10. 列举模块化 **四大 pitfalls**（循环依赖、全局变量、头文件膨胀、增量构建失效）及对策（**9.5**）
11. 描述 Linux 内核 **`include/linux/` vs 驱动私有头** 的分工（**9.4.8**）
12. 说明 **ch09** 如何为 **ch10 终章项目** 提供构建与解耦基础（**9.9**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch01** 类型；**ch04** 链接属性；**ch06** weak；**ch08** ops/分层 |
| 后置 | **ch10** OS/驱动并发与终章综合项目 |

## 小节

- [9.1 模块的编译和链接](./9.1-模块的编译和链接.md)
- [9.2 系统模块划分](./9.2-module-division/9.2-系统模块划分.md)
  - [9.2.1 模块划分方法](./9.2-module-division/9.2.1-模块划分方法.md)
  - [9.2.2 面向对象编程的思维陷阱](./9.2-module-division/9.2.2-面向对象编程的思维陷阱.md)
  - [9.2.3 规划合理的目录结构](./9.2-module-division/9.2.3-规划合理的目录结构.md)
- [9.3 一个模块的封装](./9.3-一个模块的封装.md)
- [9.4 头文件深度剖析](./9.4-header-files/9.4-头文件深度剖析.md)
  - [9.4.1 基本概念](./9.4-header-files/9.4.1-基本概念.md)
  - [9.4.2 隐式声明](./9.4-header-files/9.4.2-隐式声明.md)
  - [9.4.3 变量的声明与定义](./9.4-header-files/9.4.3-变量的声明与定义.md)
  - [9.4.4 如何区分定义和声明](./9.4-header-files/9.4.4-如何区分定义和声明.md)
  - [9.4.5 前向引用和前向声明](./9.4-header-files/9.4.5-前向引用和前向声明.md)
  - [9.4.6 定义与声明的一致性](./9.4-header-files/9.4.6-定义与声明的一致性.md)
  - [9.4.7 头文件路径](./9.4-header-files/9.4.7-头文件路径.md)
  - [9.4.8 Linux内核中的头文件](./9.4-header-files/9.4.8-Linux内核中的头文件.md)
  - [9.4.9 头文件中的内联函数](./9.4-header-files/9.4.9-头文件中的内联函数.md)
- [9.5 模块设计原则](./9.5-模块设计原则.md)
- [9.6 被误解的关键字：goto](./9.6-被误解的关键字-goto.md)
- [9.7 模块间通信](./9.7-inter-module/9.7-模块间通信.md)
  - [9.7.1 全局变量](./9.7-inter-module/9.7.1-全局变量.md)
  - [9.7.2 回调函数](./9.7-inter-module/9.7.2-回调函数.md)
  - [9.7.3 异步通信](./9.7-inter-module/9.7.3-异步通信.md)
- [9.8 模块设计进阶](./9.8-advanced/9.8-模块设计进阶.md)
  - [9.8.1 跨平台设计](./9.8-advanced/9.8.1-跨平台设计.md)
  - [9.8.2 框架](./9.8-advanced/9.8.2-框架.md)
- [9.9 AIoT时代的模块化编程](./9.9-AIoT时代的模块化编程.md)
