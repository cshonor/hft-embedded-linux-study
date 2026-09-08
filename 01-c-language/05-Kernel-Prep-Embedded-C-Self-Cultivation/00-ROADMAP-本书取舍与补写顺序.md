# 00 · 本书取舍与补写顺序（GNU C 扩展 × 嵌入式应用主线）

[← 第 5 本书 README](./README.md) · [ch06 GNU C 扩展](./ch06-gnu-c-extensions/README.md)

## 为什么要有这份文件

这本书原书 10 章是**"从芯片到操作系统"的通识结构**：第 2 章讲体系结构、第 3 章讲 ARM 汇编、第 10 章讲多任务与 OS——这些内容和 **CSAPP**、操作系统教材大面积重叠，逐章精读的边际收益很低。

而你来这本书真正要拿走的是两样东西：

1. **GNU C 扩展**——标准 C 教材不讲、但内核/DPDK/裸机代码里到处都是的语法（`__attribute__`、`typeof`/`container_of`、语句表达式、零长数组、`section`/`aligned`/`weak`、内联汇编、内建函数）。
2. **嵌入式 C 的落地套路**——程序怎么变成二进制、怎么链接到指定地址、栈和堆怎么摆、设备寄存器怎么用 C 表达、模块怎么切。

所以这份文件把 10 章重排成 **A / B / C 三档**，并规定补写顺序。以后每一批笔记都按这个顺序推进，C 档只保留索引、不再投入。

---

## 一、三档定位总表

数据为 2026-09-08 实测统计（篇数 / 中位字节 / 骨架占比，骨架 = 小于 2.5 KB）。

| 档 | 章 | 目录 | 篇 | 中位 | 骨架% | 定位 | 与已有资源的重叠 | 处置 |
|----|----|------|----|------|-------|------|------|------|
| **A** | ch06 | [GNU C 扩展](./ch06-gnu-c-extensions/) | 67 | 2125 | 74% | **全书唯一核心**。标准 C → 内核的桥 | 无（唯一来源） | 逐节精写 + WSL 实测，目标 10–40 KB/篇 |
| **A** | ch04 | [编译链接安装运行](./ch04-compile-link-install-run/) | 34 | 2222 | 88% | **嵌入式落地地基**：链接脚本、静态/动态库、内核模块、U-boot 重定位 | CSAPP ch7 只讲 ELF 与链接概念，不讲链接脚本与重定位实战 | 精写，目标 15–30 KB/篇 |
| **A** | ch05 | [内存堆栈管理](./ch05-memory-stack-management/) | 33 | 2033 | 84% | **嵌入式最致命的部分**：栈布局、堆、mmap、泄漏、内存错误 | TLPI ch6/ch7/ch10 覆盖进程内存，但栈帧/裸机堆管理不重叠 | 精写，目标 15–30 KB/篇 |
| **A** | ch01 | [工具链](./ch01-tools-of-the-trade/) | 23 | 1403 | 73% | 天天要用的 binutils / ELF / make / gdb | 部分与平时工具使用重叠 | 精写常用部分（1.4 ELF、1.2 make），1.1 vim / 1.3 git 保持索引 |
| **B** | ch09 | [模块化编程](./ch09-modular-programming-in-c/) | 28 | 3677 | 3% | 工程实践：头文件、模块封装、goto | 已达标 | 维持，只补 9.2/9.7 |
| **B** | ch07 | [数据存储与指针](./ch07-data-storage-and-pointers/) | 43 | 2545 | 37% | **只挑与嵌入式强相关的节** | 指针基础与 [Pointers on C](../02-Pointers-on-C/) 重叠 | 精挑 7.2 对齐 / 7.3 可移植性 / 7.4 size_t / 7.13 void；其余维持 |
| **B** | ch08 | [OOP in C](./ch08-oop-in-c/) | 18 | 3496 | 5% | 与 6.4 container_of 联动的面向对象套路 | 已达标 | 维持，与 6.4 双向链接 |
| **B** | ch10 | [多任务与 OS](./ch10-multitasking-and-os/) | 45 | 2928 | 13% | **只挑嵌入式侧**：裸机、中断、寄存器/MMIO | 进程/线程/文件系统/IO 属 OS 通识 | 精挑 10.1 裸机 / 10.3 中断 / 10.8 寄存器；其余维持索引 |
| **B*** | ch03.6/3.7 | [C 与汇编混合编程 / GNU ARM 工具链](./ch03-arm-architecture-and-assembly/) | — | — | — | **内联汇编本质是 GNU C 扩展**（`__asm__ __volatile__`），不属于"硬件课" | — | 提到 A 档精写，与 ch06 联动 |
| **C** | ch02 | [计算机体系结构](./ch02-computer-architecture-and-cpu/) | 49 | 1189 | 97% | 降级 | **CSAPP ch1/3/4/5/6 讲得更深**（流水线、cache、多核、ISA） | **不再投入**。保留目录索引 + 每篇一句"→ 见 CSAPP chX" |
| **C** | ch03.1–3.5, 3.9 | ARM 指令/寻址/伪指令/异常 | — | 1415 | 90% | 降级 | 有 ARM 汇编专书 + 树莓派实操通道 | 保留索引；真要用时按 Pi 实测补 |

> **一句话判据**：这件事"离开这本书就没人系统讲"吗？是 → A 档；"别的书讲得更深" → C 档；"只有部分节属于 C 语言范畴" → B 档。

---

## 二、A 档补写顺序（每批一节，实测优先）

### 批次 A1 —— ch06 全章扫平（最高优先级）

| 序 | 节 | 现状 | 说明 |
|----|----|------|------|
| ✅ | [6.6 属性声明 section](./ch06-gnu-c-extensions/6.6-section/6.6.1-GNU-C编译器扩展关键字-__attribute__.md) | 37 KB | 已完成（第 20 批） |
| ▶ | [6.4 typeof 与 container_of](./ch06-gnu-c-extensions/6.4-typeof-container-of/6.4-typeof与container_of宏.md) | 1.5 KB | **本批** |
| 3 | [6.3 语句表达式](./ch06-gnu-c-extensions/6.3-statement-expr/6.3-宏构造-利器-语句表达式.md) | 1.7 KB | 与 6.4 配套（typeof + 语句表达式 = 内核宏两件套） |
| 4 | [6.5 零长度数组](./ch06-gnu-c-extensions/6.5-zero-length-array/6.5-零长度数组.md) | 1.5 KB | 柔性数组、变长报文 |
| 5 | [6.7 aligned](./ch06-gnu-c-extensions/6.7-aligned/) | 19 KB/6 篇 | 与 cache line、DMA 对齐联动 |
| 6 | [6.9 weak / alias](./ch06-gnu-c-extensions/6.9-weak/) | 16 KB/5 篇 | 驱动/SDK 的符号覆盖机制 |
| 7 | [6.10 inline](./ch06-gnu-c-extensions/6.10-inline/) | 8 KB/6 篇 | 与 HFT 热点路径相关 |
| 8 | [6.11 builtin](./ch06-gnu-c-extensions/6.11-builtin/) | 14 KB/7 篇 | `__builtin_expect` / `__builtin_types_compatible_p` |
| 9 | [6.12 变参宏](./ch06-gnu-c-extensions/6.12-vararg-macro/) | 7 KB/5 篇 | `##__VA_ARGS__` 与日志宏 |
| 10 | [6.8 format](./ch06-gnu-c-extensions/6.8-format/) | 8 KB/4 篇 | 已由 [Pointers on C 7.6.2](../02-Pointers-on-C/ch07-functions/7.6-variable-argument-lists/7.6.2-可变参数的限制.md) 深度覆盖，回链即可 |
| 11 | [6.2 指定初始化](./ch06-gnu-c-extensions/6.2-designated-init/) | 10 KB/5 篇 | 驱动里的 `.member = value` 套路 |
| 12 | [6.1 C 标准](./ch06-gnu-c-extensions/6.1-c-standard/) | 12 KB/6 篇 | 与 [6.0 GNU C 占比](./ch06-gnu-c-extensions/6.0-driver-how-much-gnu-c.md) 联动 |

### 批次 A2 —— ch04 编译链接（嵌入式落地）
4.14 链接脚本 → 4.7 静态库 → 4.8 动态链接 → 4.9 插件 → 4.10 内核模块 → 4.12 U-boot 重定位 → 4.13 binutils。

### 批次 A3 —— ch05 内存堆栈
5.3 栈（栈帧、栈溢出检测）→ 5.4 堆（裸机 malloc 实现）→ 5.5 mmap → 5.6 泄漏 → 5.7 内存错误。

### 批次 A4 —— ch01 工具链 + ch03.6 内联汇编

### 批次 B —— ch10 裸机/中断/寄存器 + ch07 对齐

---

## 三、C 档的处置细则（不再投入，但不删）

1. **保留目录与索引**：章节 README 里的小节链接全部保留，点击不会 404。
2. **每篇顶部加一句重定向**：形如 `> 本篇降级：见 CSAPP 第 6 章（存储器层次结构）。本书此处仅作索引。`
3. **不补写、不实测、不写 Quiz**：骨架文件维持现状，避免把时间花在别处讲得更深的内容上。
4. **例外**：如果某天在 A 档笔记里发现必须引用某个 C 档概念（例如讲 `aligned` 必须解释 cache line），就在 **A 档那篇里**就地讲清楚，并回链 C 档原文——**不回过头去扩写 C 档**。

---

## 四、本篇与哪些笔记联动

| 主题 | 主战场 | 关联笔记 |
|------|--------|----------|
| `container_of` / 侵入式链表 | [6.4](./ch06-gnu-c-extensions/6.4-typeof-container-of/6.4-typeof与container_of宏.md) | [ch08 OOP in C](./ch08-oop-in-c/) |
| `__attribute__` 全族 | [6.6.1](./ch06-gnu-c-extensions/6.6-section/6.6.1-GNU-C编译器扩展关键字-__attribute__.md) | [Pointers on C 18.10 深度实测](../02-Pointers-on-C/ch18-runtime-environment/18.10-GCC属性总览.md) |
| 变参 + format 属性 | [Pointers on C 7.6.2](../02-Pointers-on-C/ch07-functions/7.6-variable-argument-lists/7.6.2-可变参数的限制.md) | [6.8](./ch06-gnu-c-extensions/6.8-format/) |
| 对齐 / 未对齐访问 | [6.7 aligned](./ch06-gnu-c-extensions/6.7-aligned/) | [ch07 7.2 对齐](./ch07-data-storage-and-pointers/7.2-alignment/) |

---

## 代码自测

**题目 1：** 这本书 10 章里，为什么只有 ch06 是"必须精读"？如果只给你一周，你会怎么安排？

<details>
<summary>参考答案</summary>

**ch06 必须精读**的理由：GNU C 扩展是唯一"标准 C 教材不讲、CSAPP 也不讲、但内核/DPDK/裸机代码里无处不在"的知识。`typeof`/`container_of`/语句表达式/零长数组/属性声明，每一条都会直接决定你能不能读懂 Linux 内核源码。CSAPP 不会教 `container_of`，Pointers on C 也不会。

**一周安排**（按 A 档顺序）：

| 天 | 内容 |
|----|------|
| 1–2 | ch06：`__attribute__` 全族（6.6）→ typeof/container_of（6.4）→ 语句表达式（6.3） |
| 3 | ch06：零长度数组（6.5）→ aligned/unaligned（6.7）→ weak/alias（6.9） |
| 4 | ch06：inline（6.10）→ builtin（6.11）→ 变参宏（6.12）→ 指定初始化（6.2） |
| 5 | ch04：链接脚本 + 静态/动态链接 + 内核模块 |
| 6 | ch05：栈帧 + 堆 + mmap + 内存错误 |
| 7 | ch01：binutils/ELF + ch03.6 内联汇编 + ch10.8 寄存器访问（volatile/MMIO） |

**明确跳过**：ch02（CSAPP 更深）、ch03.1–3.5（有 ARM 专书与 Pi 实操）、ch10 的进程/线程/文件系统（OS 教材）。

</details>

**题目 2：** ch03 的"内联汇编"为什么被提到 A 档，而 ch02 的"流水线/Cache"被降到 C 档？

<details>
<summary>参考答案</summary>

判据不是"难不难"，而是**"它是不是 C 语言知识"**。

- **内联汇编（ch03.6）**：写法是 `__asm__ __volatile__("" : : "r"(x) : "memory")`，属于 **GNU C 的语法扩展**——操作数约束、clobber 列表、`volatile` 语义、与编译器优化的交互，全是编译器层面的知识，跟"ARM 有几级流水线"无关。它和 6.x 系列是同一类东西，所以进 A 档。
- **流水线 / Cache（ch02）**：属于**微架构与体系结构**，是硬件设计者的知识。CSAPP 第 4、5、6 章讲得比这本书深得多，而且从 C 程序员视角看，真正需要知道的只有两条结论：cache line 是 64 字节（决定结构体 padding）、分支预测失败有代价（决定 `likely/unlikely`）。这两条结论在用到它们的 A 档笔记里就地讲即可，不必单独读一章。

</details>
