# ABI · Application Binary Interface（应用二进制接口）

> **01 CSAPP · Ch2 §2.1.2 延伸阅读** · 解释 `sizeof` 为何随平台变、结构体布局、函数怎么调  
> **关联：** [§2.1.2 数据大小与 sizeof](./section-2.1.2-数据大小与sizeof.md) · [§2.1.3 字节序](./section-2.1.3-寻址与字节序.md) · [§3.7 调用约定](../../chapter-03-machine-level-programs/notes/section-3.7-过程与栈帧.md) · [§3.8 指针步长](../../chapter-03-machine-level-programs/notes/section-3.8-指针步长详解.md)

---

## 1. 全称

**Application Binary Interface** — **应用二进制接口**

---

## 2. 一句话核心 · 协议类比

> **ABI = 机器之间的二进制通信协议。**  
> 协议本质：双方提前约定固定规则，否则无法交互。

| | |
|--|--|
| **谁必须遵守** | 编译产物、CPU、OS 内核、动态库（`.so`/`.dll`） |
| **没协议时** | 传参、栈、系统调用各玩各的 → 直接崩 |
| **同一套 ABI** | 分开编译的 `.o` 能链接；别人编好的 `.so` 你能直接调 |

- **源代码** 是给人看的（API、语法、头文件）  
- **ABI** 是 **编译后的二进制** 与硬件/内核之间的 **强制沟通规则**

两套程序（`.o`、动态库、不同编译器产物）**不需要源码**，只要遵守 **同一套 ABI**，就能互相链接、调用、运行。

---

## 3. ABI 规定了什么（对应 CSAPP 章节）

| # | ABI 管什么 | 在 CSAPP 哪里 |
|---|------------|---------------|
| 1 | **基础类型占几字节** | Ch2 §2.1.2 — LP64 下 `long` 8 字节，LLP64（Win x64）下 `long` 4 字节 → **`sizeof` 不同的根源** |
| 2 | **结构体对齐、padding** | Ch3 §3.9 · [ch02 demo](../../code/ch02-endian-and-padding-demo.c) |
| 3 | **函数调用约定** | Ch3 §3.7 — System V AMD64：`%rdi,%rsi,…` 传参，`%rax` 返回值 |
| 4 | **指针宽度、符号命名、链接名修饰** | 动态链接、`dlsym`；C++ mangling 另有一套 |
| 5 | **endian、syscall 号、动态链接格式** | Ch2 §2.1.3；ELF/Mach-O/PE |

---

## 4. API vs ABI（源码协议 vs 二进制协议）

| | **API** | **ABI** |
|--|---------|---------|
| **本质** | 源码层「开发协议」 | **二进制层「通信协议」** |
| **给谁看** | 程序员 ↔ 编译器 | **elf/机器码 ↔ CPU ↔ OS ↔ 动态库** |
| **约定什么** | 函数名、参数**类型**、头文件声明 | 寄存器传参、栈布局、结构体内存排布、系统调用方式 |
| **约束何时** | 只约束源码；不同编译器编完后二进制规则可能不同 | **运行时真正生效**的底层协议 |
| **例子** | `int foo(long x);` 声明 | Linux x64：`foo` 首参进 `%rdi`；`long` 在 LP64 下占 8 字节 |

**同一套 C 源码：**

```text
Linux GCC/Clang  →  System V AMD64 ABI  →  .elf（可互调）
Windows MSVC     →  Microsoft x64 ABI   →  .exe
```

API 看起来一样，**ABI 不同 → 协议不兼容 → 二进制不能互换**（Linux 程序不能直接当 Windows 二进制跑）。

### 两个熟悉的 ABI 例子

**① ARM AAPCS**（ARM 架构过程调用标准 · → [Smith Ch13](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-13-subroutines-stacks/notes/section-13-5-apcs.md)）

| 条款（直觉） | |
|--------------|--|
| 前 4 个整型参数 | **R0–R3** |
| 栈对齐 | 常要求 **8 字节** |
| callee-save | **R4–R11** 子函数须保存/恢复 |

汇编不遵守 → 调 C 函数参数全错、程序飞掉。

**② x86-64 System V ABI**（Linux / HFT 服务器 · → [Ch3 §3.7](../../chapter-03-machine-level-programs/notes/section-3.7-过程与栈帧.md)）

整型参数依次：**`%rdi, %rsi, %rdx, %rcx, %r8, %r9`**（再溢出到栈）。

### 协议的通用性

| 场景 | 结果 |
|------|------|
| 同一架构 + 同一 OS + 同一 ABI | gcc/clang 编的库常可互调（Linux x86_64 → System V） |
| 换 OS / 换 Windows x64 ABI | **协议不兼容**，不能直接跑对方二进制 |

---

## 5. 典型 ABI 速查（64 位）

| 类型 | ILP32（32 位） | LP64（Linux/macOS x64） | LLP64（Windows x64） |
|------|----------------|-------------------------|----------------------|
| `int` | 4 | 4 | 4 |
| `long` | 4 | **8** | **4** |
| 指针 | 4 | 8 | 8 |

→ 本机验证：[ch02-endian-and-padding-demo.c](../../code/ch02-endian-and-padding-demo.c) 的 `demo_sizeof()`

---

## 6. HFT / 跨平台

1. **`sizeof(int/long)` 为何变** — 目标 **ABI** 写死类型模型，不是编译器随意
2. **二进制协议** — `int32_t`/`int64_t` + 显式 endian；**禁止** 本地 `struct` / `long` 上 wire
3. **C++ / Rust / DPDK / 内核** — 共享边界须 **同一 ABI**（HFT 常锁 Linux x64 LP64 + System V）
4. **MikanOS / 汇编** — 无编译器兜底时 **手写** calling convention

```c
#include <stdint.h>   /* 固定宽度 */
/* wire：显式 endian；别 struct 裸 send */
```

---

## 7. 极简总结

**ABI = 二进制世界里的底层通信协议** — 数据怎么存、函数怎么传参、模块怎么链接；硬件与程序强制遵守。  
**API = 源码世界里的开发协议** — 人与编译器之间的接口声明。

---

## 口述巩固 · 自测

1. 为什么说 ABI 是「协议」？没有它会怎样？  
2. API 和 ABI 分别约束谁、约束源码还是机器码？  
3. AAPCS 和 System V 各举一条「条款」。  
4. 为什么 Linux 的 `.so` 不能直接给 Windows 用？  
5. 为什么 Linux 和 Windows 上 `sizeof(long)` 常不同？  
6. 把 `struct Message` 直接 `send()` 会踩 ABI 的哪几条规则？  
7. Ch3 里 `%rdi` 传第一个参数 — 属于 ABI 的哪一类规定？

---

← [Ch2 导读](../README.md) · [§2.1.2 数据大小](./section-2.1.2-数据大小与sizeof.md) · [01 code](../../code/)
