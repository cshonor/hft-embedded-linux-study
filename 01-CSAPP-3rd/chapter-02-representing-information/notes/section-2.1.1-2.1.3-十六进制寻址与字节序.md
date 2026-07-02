## 2.1.1–2.1.3 十六进制、数据大小、寻址与字节顺序

### 2.1.1 十六进制表示法

- 二进制太长，十六进制 **每 1 位 hex = 4 bit**，便于读地址与位模式
- C 字面量：`0xFF`、`0xDEADBEEF`

| 十进制 | 十六进制 | 二进制（8 bit） |
|--------|----------|---------------|
| 255 | 0xFF | 11111111 |
| 16 | 0x10 | 00010000 |

**习惯：** 调试内存、协议 dump、寄存器值 — 一律 hex。

### 2.1.2 数据大小

**`sizeof(T)` 是什么？**

- **编译期常量** — 编译器按 **目标架构 + ABI** 填好，不是运行时量出来的
- 同一份源码，换 `-m32` / `-m64`、换 OS、换编译器目标，**sizeof 结果可以变**
- **可移植代码不要假设 `int` 一定是 4 字节** — 老平台/嵌入式上 `int` 可能是 2 字节

**典型 ABI 对照（64 位时代最容易混）：**

| 类型 | ILP32（32 位） | LP64（Linux/macOS x86-64） | LLP64（Windows x64） |
|------|----------------|----------------------------|----------------------|
| `int` | 4 | 4 | 4 |
| `long` | 4 | **8** | **4** ← 和 Linux 不同 |
| 指针 | 4 | 8 | 8 |
| `size_t` | 4 | 8 | 8 |

→ 聊天里说的没错：**64 位 Linux 上 `sizeof(long)==8`，64 位 Windows 上 `sizeof(long)==4`**，不是 CPU「位数」单独决定的，是 **ABI 命名规则（LP64 vs LLP64）**。

| C 类型（典型 LP64 Linux x86-64） | 字节 |
|----------------------------------|------|
| `char` | 1 |
| `short` | 2 |
| `int` | 4 |
| `long` | 8 |
| `float` | 4 |
| `double` | 8 |
| 指针 | 8 |

- **字长 (word size)** — 机器一次处理的数据宽度；64 位机指针通常 8 字节

**跨平台怎么写：**

```c
#include <stdint.h>

int32_t  price_tick;   /* 协议/HFT：字段宽度写死 */
int64_t  qty;
uint64_t seq;
/* 避免：协议里用 int/long，换平台就炸 */
```

**HFT 里 sizeof/ABI 真会踩坑的场景：**

| 坑 | 后果 |
|----|------|
|  wire 协议用 `long` / 本地 `struct` 直接 `send()` | Windows LLP64 vs Linux LP64 长度不一致 |
|  假设 `int` 永远 4 字节 | 某些嵌入式 ABI 上结构体布局全错 |
|  Rust/C++ 共享内存没约定 layout | `long` 宽度、padding、endian 三重不一致 |

**HFT：** 协议 schema 必须写死 **字段宽度**（`int32`/`int64`/`uint64`），不能假设与本地 `long` 相同；跨语言对齐 [ABI / SBE / FIX 规范]。

### 2.1.3 寻址和字节顺序 (Byte Ordering)

**寻址 (addressing)：**

- CPU 通过 **地址** 访问内存；x86-64 是 **按字节寻址 (byte-addressable)** — 每个字节有唯一地址
- `&x` 给出对象 **起始字节** 的地址；`int` 占 4 个 **连续** 字节地址，如 `0x1000`–`0x1003`
- 指针算术：`p+1` 对 `char*` 前进 1 字节，对 `int*` 前进 `sizeof(int)` 字节

**字节顺序 (byte order / endianness)** — 多字节对象占多个连续地址时，**各字节谁先谁后：**

**小端 (little-endian)：** 最低有效字节在低地址 — **x86-64、多数 ARM 默认**

**大端 (big-endian)：** 最高有效字节在低地址 — **网络字节序 (network byte order)、不少协议**

> **术语：** 英文常叫 **byte order** / **endianness**；CSAPP 用 **Little-Endian** / **Big-Endian**。

示例：`int x = 0x12345678`，地址 `&x` 起 4 字节：

| 地址 | 小端 | 大端 |
|------|------|------|
| 低 | 0x78 | 0x12 |
| +1 | 0x56 | 0x34 |
| +2 | 0x34 | 0x56 |
| +3 | 0x12 | 0x78 |

换 `0x11223344` 同理 — 小端低地址起为 `44 33 22 11`，大端为 `11 22 33 44`（与聊天里用的例子同一规则，只是 hex 不同）。

**和字符串的直觉对比（类比，不是定义）：**

| | 内存低地址 → 高地址 | 像什么 |
|--|---------------------|--------|
| 字符串 `"1234"` | `'1' '2' '3' '4'`（0x31…0x34） | 书写顺序「从左到右」，**接近大端「高位/左侧先出现」** |
| 小端整数 `0x12345678` | `78 56 34 12` | **低位字节先放低地址**，和字符串习惯 **相反** |

本质仍是：**多字节数值里，各字节按什么顺序占地址** — 不是单纯的「从左写还是从右写」。

**动手验证（x86 上一眼看到小端）：**

→ [02-c-programming/code/endian_and_padding_demo.c](../../../02-c-programming/code/endian_and_padding_demo.c) — 用 `(unsigned char *)&a` 逐字节打印 `0x11223344`。

```c
// 网络序 ↔ 主机序
uint32_t htonl(uint32_t hostlong);
uint32_t ntohl(uint32_t netlong);
```

**Padding（对齐填充）不在本节：** 属于 **Ch3 结构体布局**（`offsetof` / `sizeof struct`）；同一 demo 文件后半有 padding 示例 → [Ch3 3.9](../../chapter-03-machine-level-programs/notes/section-3.9-结构体联合与对齐.md)。

**HFT：**

- **线上协议** 常规定 big-endian；本机解析必须 `ntoh*` 或显式 swap
- **同机 IPC / mmap 共享 struct** — 两端必须同 endian + 同 padding，否则 silent corruption
- **错误用法：** 把 `struct Message` 直接 `send()` 而不序列化 — 字节序 + 对齐 + 版本都会炸

→ 网络编程：[Ch 11](../../chapter-11-network-programming/) · [11 UNP](../../../11-UNP-Vol1/) · [02 C demo](../../../02-c-programming/code/endian_and_padding_demo.c)

---

← [本章导读](../README.md)
