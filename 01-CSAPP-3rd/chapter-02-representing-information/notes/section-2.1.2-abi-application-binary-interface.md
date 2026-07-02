# ABI · Application Binary Interface（应用二进制接口）

> **01 CSAPP · Ch2 §2.1.2 延伸阅读** · 解释 `sizeof` 为何随平台变、结构体布局、函数怎么调  
> **关联：** [§2.1.1–2.1.3 数据大小与字节序](./section-2.1.1-2.1.3-十六进制寻址与字节序.md) · [§3.7 调用约定](../../chapter-03-machine-level-programs/notes/section-3.7-过程与栈帧.md) · [§3.8 指针步长](../../chapter-03-machine-level-programs/notes/section-3.8-指针步长详解.md)

---

## 1. 全称

**Application Binary Interface** — **应用二进制接口**

---

## 2. 一句话核心

- **源代码** 是给人看的（API、语法、头文件）
- **ABI** 是 **编译后的二进制** 之间约定好的 **统一规则**

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

## 4. API vs ABI（别混）

| | **API** | **ABI** |
|--|---------|---------|
| **层面** | 源码 — 给程序员 | 二进制 — 给 CPU / 链接器 / 加载器 |
| **内容** | 头文件、函数名、参数 **类型** | 类型 **几字节**、struct **布局**、调用时 **寄存器/栈** |
| **例子** | `int foo(long x);` 声明 | `foo` 的第一个参数在 Linux x64 进 `%rdi`，`long` 占 8 字节（LP64） |

**同一套 C 源码：**

```text
Linux GCC 编译  →  System V AMD64 ABI  →  .elf
Windows MSVC    →  Microsoft x64 ABI   →  .exe
```

API 看起来一样，**ABI 不同 → 二进制不能互换**。

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

**ABI = 二进制世界里的统一协议** — 数据怎么存、函数怎么传参、模块怎么链接。

---

## 口述巩固 · 自测

1. API 和 ABI 分别给谁用？
2. 为什么 Linux 和 Windows 上 `sizeof(long)` 常不同？
3. 把 `struct Message` 直接 `send()` 会踩 ABI 的哪几条规则？
4. Ch3 里 `%rdi` 传第一个参数 — 属于 ABI 的哪一类规定？

---

← [Ch2 导读](../README.md) · [01 CSAPP 动手代码](../../code/)
