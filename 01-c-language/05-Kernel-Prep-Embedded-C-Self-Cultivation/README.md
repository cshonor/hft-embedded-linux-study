# 《嵌入式 C 语言自我修养》

**从芯片、编译器到操作系统** · 王利涛

> **第 5 本书** · GNU C 扩展（标准 → 内核的桥）

## 定位

阶段 2 · GNU-C 扩展（内核专属，必学）。从 C 语言出发，在默认读者已掌握基本语法的基础上，深入探讨 CPU 工作原理、计算机体系结构、ARM 平台下程序的编译/链接、以及程序运行时的内存堆栈管理等底层知识。

Linux 内核、内核模块、DPDK 代码大量依赖 GNU-C 扩展；标准 C 教材不讲这些内容。读 LKD、虚拟内存、内核网络源码前需先过本书。

## 阅读建议（2026-09-08 重排）

> **本书只取两块**：**GNU C 扩展**（ch06 是唯一核心）+ **嵌入式 C 落地**（ch04 链接、ch05 内存、ch01 工具链、ch10 裸机/中断/寄存器）。
> **ch02 体系结构、ch03.1–3.5 ARM 指令** 🗑️ **已删除 73 篇**——CSAPP 讲得更深，不重复读第二遍。
> **ch02 不用再管**：现在只剩一个 README 说明页（记"删了什么、去哪找"），不占学习时间。
> 完整取舍与补写顺序见 **[00 · 本书取舍与补写顺序](./00-ROADMAP-本书取舍与补写顺序.md)**。

### 两条主线（按这个读，不要按章节号读）

**主线一 · GNU C 扩展**（ch06，本书唯一核心）

| 序 | 小节 | 状态 |
|----|------|------|
| 1 | [6.1/6.2 预处理与宏基础](./ch06-gnu-c-extensions/) | 待扩 |
| 2 | [6.3 语句表达式](./ch06-gnu-c-extensions/6.3-statement-expr/6.3-宏构造-利器-语句表达式.md) | ✅ 29 KB |
| 3 | [6.4 typeof 与 container_of](./ch06-gnu-c-extensions/6.4-typeof-container-of/6.4-typeof与container_of宏.md) | ✅ 28 KB |
| 4 | [6.5 零长度数组与柔性数组](./ch06-gnu-c-extensions/6.5-zero-length-array/6.5-零长度数组.md) | ✅ 39 KB |
| 5 | [6.6 `__attribute__` 总纲](./ch06-gnu-c-extensions/6.6-section/6.6.1-GNU-C编译器扩展关键字-__attribute__.md) | ✅ 37 KB |
| 6 | [6.7 aligned 与 packed](./ch06-gnu-c-extensions/6.7-aligned/6.7-属性声明-aligned.md) | ✅ 32 KB |
| 7 | [6.9 weak 与 alias](./ch06-gnu-c-extensions/6.9-weak/6.9-属性声明-weak.md) | ✅ 31 KB |
| 8 | [6.10 inline](./ch06-gnu-c-extensions/6.10-inline/) · 6.11 内建函数 · 6.12 变参宏 | ▶ 下一批 |
| 附 | [ch03.6 内联汇编](./ch03-arm-architecture-and-assembly/)（`__asm__ __volatile__` 本质是 GNU 扩展） | 待扩 |

**主线二 · 嵌入式 C 落地**

| 序 | 小节 | 状态 |
|----|------|------|
| 1 | [10.8 嵌入式 C 开门](./ch10-multitasking-and-os/10.8-register/10.8-寄存器操作.md)（volatile / 屏障 / 位操作 / 字节序 / 未对齐） | ✅ 29 KB |
| 2 | 10.3 中断（ISR 与主循环共享数据） | 待改造 |
| 3 | ch04 链接脚本（`section` 属性 + 链接脚本） | 待扩 |
| 4 | ch05 内存堆栈（栈帧、栈溢出） | 待扩 |
| 5 | ch01 工具链（交叉编译、`objdump`） | 待扩 |

| 档 | 章 | 处置 |
|----|----|------|
| **A 主战场** | ch06 GNU C 扩展 · ch04 编译链接 · ch05 内存堆栈 · ch01 工具链 · ch03.6 内联汇编 | 逐节精写 + WSL 实测 |
| **B 挑着学** | ch09 模块化 · ch08 OOP · ch07 对齐/可移植性 · ch10 裸机/中断/寄存器 | 只补与 C 语言、嵌入式强相关的节 |
| **C 已删** | ch02 计算机体系结构（48 篇）· ch03.1–3.5/3.9 ARM 指令/寻址/伪指令/异常（24 篇） | **物理删除**。只留 ch02/ch03 README 说明去向；ch02 的 MMIO/大小端已改造成 C 视角写进 [10.8](./ch10-multitasking-and-os/10.8-register/10.8-寄存器操作.md) |

原始顺序（按原书结构）保留如下，供查阅：

- **第 1–4 章**：工具链、体系结构、ARM 汇编、编译链接——建立从源码到二进制的完整图景
- **第 5–7 章**：堆栈内存、GNU C 扩展、指针——内核/DPDK 代码的直接前置
- **第 8–10 章**：OOP、模块化、多任务/OS——嵌入式与内核编程思想

## 章节索引

全书 10 章。各章目录下已按小节划分占位笔记；路径均为 ASCII，中文标题在文件内。

| 章 | 目录 | 主题 |
|----|------|------|
| 第 1 章 | [ch01-tools-of-the-trade](./ch01-tools-of-the-trade/) | 工欲善其事，必先利其器 |
| 第 2 章 | [ch02-computer-architecture-and-cpu](./ch02-computer-architecture-and-cpu/) | 计算机体系结构与 CPU 工作原理 —— **🗑️ 正文已删 48 篇**（CSAPP 更深，仅保留去向说明）|
| 第 3 章 | [ch03-arm-architecture-and-assembly](./ch03-arm-architecture-and-assembly/) | ARM 体系结构与汇编语言 —— **🗑️ 已删 24 篇**（指令/寻址/伪指令/异常），**保留 3.6 内联汇编 / 3.7 GNU ARM / 3.8 AArch64** |
| 第 4 章 | [ch04-compile-link-install-run](./ch04-compile-link-install-run/) | 程序的编译、链接、安装和运行 |
| 第 5 章 | [ch05-memory-stack-management](./ch05-memory-stack-management/) | 内存堆栈管理 |
| 第 6 章 | [ch06-gnu-c-extensions](./ch06-gnu-c-extensions/) | GNU C 编译器扩展语法精讲 |
| 第 7 章 | [ch07-data-storage-and-pointers](./ch07-data-storage-and-pointers/) | 数据存储与指针 |
| 第 8 章 | [ch08-oop-in-c](./ch08-oop-in-c/) | C 语言的面向对象编程思想 |
| 第 9 章 | [ch09-modular-programming-in-c](./ch09-modular-programming-in-c/) | C 语言的模块化编程思想 |
| 第 10 章 | [ch10-multitasking-and-os](./ch10-multitasking-and-os/) | C 语言的多任务编程思想和操作系统入门 |

## 学习进度

- [x] 第 1 章 工欲善其事，必先利其器
- [x] 第 2 章 计算机体系结构与 CPU 工作原理 —— 🗑️ 已删（CSAPP 覆盖）
- [x] 第 3 章 ARM 体系结构与汇编语言 —— 🗑️ 部分已删，保留 C/汇编混合编程
- [x] 第 4 章 程序的编译、链接、安装和运行
- [x] 第 5 章 内存堆栈管理
- [x] 第 6 章 GNU C 编译器扩展语法精讲
- [x] 第 7 章 数据存储与指针
- [x] 第 8 章 C 语言的面向对象编程思想
- [x] 第 9 章 C 语言的模块化编程思想
- [x] 第 10 章 C 语言的多任务编程思想和操作系统入门

---

## 代码自测

**题目 1：** 嵌入式 C 自我修养这本书的核心定位是什么？它填补了标准 C 和内核开发之间的什么知识空白？
```c
// 以下内核代码用到了哪些本书讲解的知识点？
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}
```
<details>
<summary>参考答案</summary>

这段内核代码用到了本书讲解的多个知识点：
1. container_of 宏（ch06 GNU C 扩展）：使用 typeof、语句表达式、零指针技巧，从链表节点指针获取宿主结构体指针
2. 侵入式链表（ch08 OOP in C）：list_head 结构体嵌入到其他结构体中，实现通用链表
3. static inline（ch09 模块化编程）：头文件中的内联函数，避免函数调用开销
4. 指针运算（ch07 数据存储与指针）：container_of 内部用 char* 指针减法计算偏移

本书的核心定位：标准 C 到内核开发的桥梁。内核代码大量使用 GNU C 扩展（__attribute__/typeof/section/weak）、OOP 模式（函数指针模拟虚函数）、模块化设计（头文件/源文件分离），这些标准 C 教材不讲，本书系统讲解。

</details>
