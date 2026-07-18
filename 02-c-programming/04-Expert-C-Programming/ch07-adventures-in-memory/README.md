# 第 7 章 对内存的思考

**Adventures in Memory** — Peter van der Linden, *Expert C Programming*

## 本章目标

从 **硬件（80x86 对齐、Cache）** 到 **OS（虚拟内存、页表、ASLR）** 再到 **程序员日常（struct padding、malloc、泄漏、信号）** 建立完整内存图景。掌握 **结构体填充手算**、**`packed` 代价**、**VA/PA/MMU** 与 **ch05 kernel.lds** 虚拟加载地址；理解 **64 B Cache 行**、**伪共享与 padding 修复**；区分 **数据段 / 堆 / 栈**、**malloc 元数据**、**返回栈局部指针 UB**、**const 三种形式**；能用 **valgrind/ASan** 查 **泄漏 / UAF / double free / 堆溢出**；区分 **SIGBUS vs SIGSEGV**。衔接 **ch06 五区域布局**、**ch05 ELF/链接脚本**、**ch08 求值顺序**。

## x86-64 典型对齐表（7.1 / 7.2）

| 类型 | 大小 (B) | 对齐 (B) | 备注 |
|------|----------|----------|------|
| **`char`** | 1 | 1 | |
| **`short`** | 2 | 2 | |
| **`int`** | 4 | 4 | |
| **`long` / 指针** | 8 | 8 | LP64 ABI |
| **`double`** | 8 | 8 | |
| **`long double`** | 16 | 16 | 平台相关 |

**struct 规则**：成员偏移按各自对齐；**`sizeof(struct)`** = 向上取整到 **最大成员对齐**。

**Test1 手算（demo01）**：

```text
  struct { char a; int b; short c; }
  0:a  1-3:pad  4-7:b  8-9:c  10-15:pad  →  sizeof = 16
```

## 虚拟内存一图（7.3）

```text
  进程 VA 空间（每进程独立页表）
  ┌─────────────────────────────────────┐
  │ 栈 ←向下    （ASLR 随机基址）         │
  │ mmap / 堆 ←向上                      │
  │ .bss / .data / .rodata / .text       │
  └─────────────────────────────────────┘
           │ MMU 页表（CR3）
           ▼
  物理 RAM + swap          kernel.lds: 链接 VA 0x80000（ch05）
```

| 概念 | 说明 |
|------|------|
| **VA / PA** | 程序见 VA；硬件经 MMU 译 PA |
| **Page fault** | 缺页、COW、权限违例 |
| **ASLR** | 栈/堆/mmap 随机化（**ch06 6.3**） |
| **隔离** | 独立页表 + R/W/X 权限 |

## Cache 与伪共享（7.4）

```text
  Core0 ── L1 ──┐
  Core1 ── L1 ──┼── L3 ── RAM
                └── 64B Cache Line（MESI）

  c1 || c2 同线 → false sharing → padding 隔离（demo04）
```

## 堆与常见陷阱（7.5 / 7.6）

| 陷阱 | 后果 | 检测 |
|------|------|------|
| **`return` 栈数组** | 悬垂指针 UB | **demo02**, `-Wreturn-local-addr` |
| **`malloc` 无 `free`** | RSS 涨 | **valgrind**, LSan |
| **UAF** | 数据损坏 | ASan |
| **double free** | 堆结构破坏 | ASan |
| **heap overflow** | 元数据损坏 | ASan, **7.5** chunk 头 |

## const 三种形式（7.5 / demo03）

| 声明 | 指针可变 | 内容可变 |
|------|----------|----------|
| **`const char *p`** | ✅ | ❌ **`.rodata`** |
| **`char *const p`** | ❌ | ✅（若指向可写区） |
| **`const char *const p`** | ❌ | ❌ |

## 前置依赖

| 依赖 | 说明 |
|------|------|
| **[Expert C ch06](../ch06-runtime-data-structures/)** | **五区域**、**栈帧**、**brk/mmap**、**valgrind**（**6.2–6.9, 6.11**） |
| **[Expert C ch05](../ch05-thinking-of-linking/)** | **ELF 段**、**kernel.lds**、**`_stext/_ebss`**（**5.1, 5.5, demo04**） |
| **[Expert C ch04](../ch04-arrays-are-not-pointers/)** | 存储与 **`sizeof`** |
| **[Expert C ch02](../ch02-its-not-a-bug-its-a-language-feature/)** | **`const`** 语义基础 |

## 环境

- **OS**：Linux / WSL（**pmap**、**valgrind**、**pthread**）
- **编译器**：GCC 或 Clang，`gcc --version`
- **推荐 flags**：`-std=c11 -Wall -Wextra -g -pthread`
- **demo/**：见下（已存在，勿改源码）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Expert-C-Programming/ch07-adventures-in-memory/demo

# 7.2 结构体对齐 / packed / offsetof
cd demo01_struct_align && make && ./demo01 && cd ..

# 7.5 返回栈局部数组 → 悬垂指针
cd demo02_stack_dangle && make && ./demo02 && cd ..

# 7.5 const 三种形式 vs .rodata
cd demo03_const_rodata && make && ./demo03 && cd ..

# 7.4 伪共享：shared vs padded 计时
cd demo04_false_sharing && make && ./main && cd ..

# 7.6 工具（对任一 demo 二进制）
valgrind --leak-check=full demo/demo01_struct_align/demo01
pmap -x $$
```

## 知识模块

| 模块 | 小节 | 核心 |
|------|------|------|
| **1 硬件演进** | **7.1** | **80x86** 史；**对齐** 硬件根源 |
| **2 内存模型** | **7.2** | **平坦 VA**；**padding**；**packed**；**序列化陷阱** |
| **3 虚拟内存** | **7.3** | **VA/PA/MMU**；**页故障**；**ASLR**；**kernel.lds** |
| **4 Cache** | **7.4** | **L1/L2/L3**；**64B 行**；**局部性**；**false sharing** |
| **5 数据段与堆** | **7.5** | **malloc 元数据**；**brk/mmap**；**const**；**栈返回 UB** |
| **6 泄漏与损坏** | **7.6** | **泄漏**；**UAF**；**double free**；**堆溢出** |
| **7 信号** | **7.7** | **SIGSEGV vs SIGBUS**；**SPARC 对齐** |
| **8 轻松一下** | **7.8** | **Thing King**；**页面游戏** 寓言 |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_struct_align** | **Test1 16B**、**packed**、**offsetof** | **7.1**, **7.2** |
| **demo02_stack_dangle** | **`return buf`** 悬垂指针 | **7.5**, **7.6** |
| **demo03_const_rodata** | **`const char *` / `char *const` / 双 const** | **7.5**, **7.7** |
| **demo04_false_sharing** | **同 Cache 行 vs 64B padding** | **7.4** |

## 高频考点 / 面试题

1. **`struct { char a; int b; short c; }` 在 x86-64 上 sizeof 多少？为什么？** → **16**；**b** 从 4 对齐需 **3B pad**，末尾对齐到 **8** 再 **6B pad**（**7.2**, **demo01**）

2. **VA 和 PA 区别？MMU 做什么？** → 程序用 **VA**；**MMU+页表** 译 **PA**；每进程 **独立页表** 实现隔离（**7.3**, **ch06 6.3**）

3. **什么是 false sharing？如何修复？** → 不同核写 **同一 Cache 行** 不同变量；**padding/`alignas(64)`**、**线程局部累加**（**7.4**, **demo04**）

4. **`const char *p` 与 `char *const p` 区别？写 `p[0]='x'` 会怎样？** → 前者 **指针可变、内容只读**（**rodata → SIGSEGV**）；后者 **指针不可变**（**7.5**, **demo03**）

5. **SIGSEGV 和 SIGBUS 常见触发差异？** → **SIGSEGV**：无效地址/权限；**SIGBUS**：**未对齐**（严格架构）、**mmap 文件异常** 等（**7.7**）

**拓展：**

- **`__attribute__((packed))` 有何风险？**（**7.2**, **7.7** 可移植性）
- **malloc 实现为什么要 chunk 头？heap overflow 为何常拖到 `free` 才崩？**（**7.5**, **7.6**）
- **kernel.lds 里 `. = 0x80000` 含义？与用户进程 ASLR 关系？**（**7.3**, **ch05 demo04**）
- **为何不能 `write(fd, &s, sizeof(s))` 做网络协议？**（**7.2** padding/endian）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **[ch06](../ch06-runtime-data-structures/)** 运行时布局、栈、堆；**[ch05](../ch05-thinking-of-linking/)** ELF、**kernel.lds** |
| 后置 | **[ch08](../ch08-halloween-vs-christmas/)** 类型、**求值顺序**、副作用 |
| 关联 | **[Embedded C](../../05-Embedded-C-Self-Cultivation/)** 裸机对齐、链接脚本 |
| 全书 | **ch04** 数组存储；**ch09** 数组再论 |

## 小节

- [7.1 Intel 80x86系列](./7.1-Intel-80x86系列.md)
- [7.2 Intel 80x86内存模型以及它的工作原理](./7.2-Intel-80x86内存模型以及它的工作原理.md)
- [7.3 虚拟内存](./7.3-虚拟内存.md)
- [7.4 Cache存储器](./7.4-Cache存储器.md)
- [7.5 数据段和堆](./7.5-数据段和堆.md)
- [7.6 内存泄漏](./7.6-内存泄漏.md)
- [7.7 总线错误](./7.7-总线错误.md)
- [7.8 轻松一下——「Thing King」和「页面游戏」](./7.8-轻松一下-Thing-King-和-页面游戏.md)
