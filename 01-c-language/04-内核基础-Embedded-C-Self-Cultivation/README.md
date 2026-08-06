# 《嵌入式 C 语言自我修养》

**从芯片、编译器到操作系统** · 王利涛

## 定位

阶段 2 · GNU-C 扩展（内核专属，必学）。从 C 语言出发，在默认读者已掌握基本语法的基础上，深入探讨 CPU 工作原理、计算机体系结构、ARM 平台下程序的编译/链接、以及程序运行时的内存堆栈管理等底层知识。

Linux 内核、内核模块、DPDK 代码大量依赖 GNU-C 扩展；标准 C 教材不讲这些内容。读 LKD、虚拟内存、内核网络源码前需先过本书。

## 阅读建议

- **第 1–4 章**：工具链、体系结构、ARM 汇编、编译链接——建立从源码到二进制的完整图景
- **第 5–7 章**：堆栈内存、GNU C 扩展、指针——内核/DPDK 代码的直接前置
- **第 8–10 章**：OOP、模块化、多任务/OS——嵌入式与内核编程思想

## 章节索引

全书 10 章。各章目录下已按小节划分占位笔记；路径均为 ASCII，中文标题在文件内。

| 章 | 目录 | 主题 |
|----|------|------|
| 第 1 章 | [ch01-tools-of-the-trade](./ch01-tools-of-the-trade/) | 工欲善其事，必先利其器 |
| 第 2 章 | [ch02-computer-architecture-and-cpu](./ch02-computer-architecture-and-cpu/) | 计算机体系结构与 CPU 工作原理 |
| 第 3 章 | [ch03-arm-architecture-and-assembly](./ch03-arm-architecture-and-assembly/) | ARM 体系结构与汇编语言 |
| 第 4 章 | [ch04-compile-link-install-run](./ch04-compile-link-install-run/) | 程序的编译、链接、安装和运行 |
| 第 5 章 | [ch05-memory-stack-management](./ch05-memory-stack-management/) | 内存堆栈管理 |
| 第 6 章 | [ch06-gnu-c-extensions](./ch06-gnu-c-extensions/) | GNU C 编译器扩展语法精讲 |
| 第 7 章 | [ch07-data-storage-and-pointers](./ch07-data-storage-and-pointers/) | 数据存储与指针 |
| 第 8 章 | [ch08-oop-in-c](./ch08-oop-in-c/) | C 语言的面向对象编程思想 |
| 第 9 章 | [ch09-modular-programming-in-c](./ch09-modular-programming-in-c/) | C 语言的模块化编程思想 |
| 第 10 章 | [ch10-multitasking-and-os](./ch10-multitasking-and-os/) | C 语言的多任务编程思想和操作系统入门 |

## 学习进度

- [x] 第 1 章 工欲善其事，必先利其器
- [x] 第 2 章 计算机体系结构与 CPU 工作原理
- [x] 第 3 章 ARM 体系结构与汇编语言
- [x] 第 4 章 程序的编译、链接、安装和运行
- [x] 第 5 章 内存堆栈管理
- [x] 第 6 章 GNU C 编译器扩展语法精讲
- [x] 第 7 章 数据存储与指针
- [x] 第 8 章 C 语言的面向对象编程思想
- [x] 第 9 章 C 语言的模块化编程思想
- [x] 第 10 章 C 语言的多任务编程思想和操作系统入门
