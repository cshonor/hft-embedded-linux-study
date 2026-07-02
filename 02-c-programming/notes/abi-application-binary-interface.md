# ABI · Application Binary Interface（应用二进制接口）

> **02 C / CSAPP 地基** · 解释 `sizeof` 为何随平台变、结构体布局、函数怎么调  
> **关联：** [Ch2 §2.1.2](../../01-CSAPP-3rd/chapter-02-representing-information/notes/section-2.1.1-2.1.3-十六进制寻址与字节序.md) · [Ch3 §3.7 System V ABI](../../01-CSAPP-3rd/chapter-03-machine-level-programs/notes/section-3.7-过程与栈帧.md) · [指针步长](./pointer-arithmetic-and-stride.md)

---

## 1. 全称

**Application Binary Interface** — **应用二进制接口**

---

## 2. 一句话核心

- **源代码** 是给人看的（API、语法、头文件）
- **ABI** 是 **编译后的二进制** 之间约定好的 **统一规则**

两套程序（`.o`、动态库、不同编译器产物）**不需要源码**，只要遵守 **同一套 ABI**，就能互相链接、调用、运行。

---

## 3. ABI 规定了什么（对应你已学的点）

| # | ABI 管什么 | 你已在哪里见过 |
|---|------------|----------------|
| 1 | **基础类型占几字节** | Ch2 §2.1.2 — LP64 下 `long` 8 字节，LLP64（Win x64）下 `long` 4 字节 → **`sizeof` 不同的根源** |
| 2 | **结构体对齐、padding** | Ch3 §3.9 · [ch02 demo](../../01-CSAPP-3rd/code/ch02-endian-and-padding-demo.c) |
| 3 | **函数调用约定** — 参数进哪些寄存器/栈、返回值放哪、谁清栈 | Ch3 §3.7 — System V AMD64：`%rdi,%rsi,…` 传参，`%rax` 返回值 |
| 4 | **指针宽度、符号命名、链接名修饰** | 动态链接 `dlsym`、C++ name mangling 另有一套，但底层仍靠 ABI 对齐 |
| 5 | **endian、syscall 号、动态链接格式** | Ch2 §2.1.3 字节序；ELF/Mach-O/PE 属 OS+ABI 生态 |

---

## 4. API vs ABI（别混）

| | **API** | **ABI** |
|--|---------|---------|
| **层面** | 源码 — 给程序员 | 二进制 — 给 CPU / 链接器 / 加载器 |
| **内容** | 头文件、函数名、参数 **类型** | 类型 **几字节**、struct **布局**、调用时 **寄存器/栈** |
| **例子** | `int foo(long x);` 声明 | `foo` 的第一个参数在 Linux x64 进 `%rdi`，`long` 占 8 字节（LP64） |

**同一套 C 源码：**

```text
Linux GCC 编译  →  遵循 System V AMD64 ABI  →  .elf
Windows MSVC    →  遵循 Microsoft x64 ABI   →  .exe
```

API 看起来一样，**ABI 不同 → 二进制不能互换**（不能拿 Linux 的 `.so` 到 Windows 直接 load）。

---

## 5. 典型 ABI 速查（64 位）

| 类型 | ILP32（32 位） | LP64（Linux/macOS x64） | LLP64（Windows x64） |
|------|----------------|-------------------------|----------------------|
| `int` | 4 | 4 | 4 |
| `long` | 4 | **8** | **4** |
| 指针 | 4 | 8 | 8 |

→ 本机验证：[ch02-endian-and-padding-demo.c](../../01-CSAPP-3rd/code/ch02-endian-and-padding-demo.c) 的 `demo_sizeof()`

---

## 6. 对你学习 / HFT 的作用

1. **解释 `sizeof(int/long)` 为何变** — 不是编译器随意，是 **目标 ABI** 写死的类型模型
2. **跨平台 & 二进制协议** — wire 格式用 `int32_t`/`int64_t` + 显式 endian，**不能** 用本地 `struct` 或 `long`
3. **HFT 工程** — C++ 引擎调 Rust、调 DPDK、调内核模块：共享边界必须 **同一 ABI**（常锁 Linux x64 LP64 + System V）
4. **自制 OS / 汇编** — 没有编译器替你兜底时，**手写** 就要自己遵守 calling convention（→ MikanOS、LKD）

**可移植三件套：**

```c
#include <stdint.h>   /* 固定宽度 */
/* 协议：显式 endian + 不用 struct 裸 send */
/* 调用边界：同 OS、同 ABI、同编译器族或明确 FFI 规则 */
```

---

## 7. 极简总结

**ABI = 二进制世界里的「普通话」** — 规定：

- 数据 **怎么存**（大小、对齐、endian）
- 函数 **怎么传参/返回**
- 模块 **怎么链接**

---

## 口述巩固 · 自测

1. API 和 ABI 分别给谁用？
2. 为什么 Linux 和 Windows 上 `sizeof(long)` 常不同？
3. 把 `struct Message` 直接 `send()` 会踩 ABI 的哪几条规则？
4. Ch3 里 `%rdi` 传第一个参数 — 属于 ABI 的哪一类规定？

---

← [02 导读](../README.md) · [Ch2 §2.1.2](../../01-CSAPP-3rd/chapter-02-representing-information/notes/section-2.1.1-2.1.3-十六进制寻址与字节序.md)
