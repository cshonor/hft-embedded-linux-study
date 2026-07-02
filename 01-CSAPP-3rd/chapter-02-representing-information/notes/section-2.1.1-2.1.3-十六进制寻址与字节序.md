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

| C 类型（典型 LP64 Linux x86-64） | 字节 |
|----------------------------------|------|
| `char` | 1 |
| `short` | 2 |
| `int` | 4 |
| `long` | 8 |
| `float` | 4 |
| `double` | 8 |
| 指针 | 8 |

- **`sizeof(T)`** — 编译期常量，随架构/ABI 变化（**可移植代码不要假设 `int` 就是 4 字节**）
- **字长 (word size)** — 机器一次处理的数据宽度；64 位机指针 8 字节

**HFT：** 协议 schema 必须写死 **字段宽度**（`int32`/`int64`/`uint64`），不能假设与本地 `long` 相同；跨语言（Rust/C++）对齐 [ABI / SBE / FIX 规范]。

### 2.1.3 寻址和字节顺序 (Byte Ordering)

多字节对象在内存中 **按字节地址从低到高** 存放；**两种常见顺序：**

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
