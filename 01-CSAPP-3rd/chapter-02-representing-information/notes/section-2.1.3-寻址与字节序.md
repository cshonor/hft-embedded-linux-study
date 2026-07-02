## 2.1.3 寻址和字节顺序 (Byte Ordering)

> **Ch2 §2.1 一条线：** [§2.1.1 十六进制](./section-2.1.1-十六进制表示法.md) → [§2.1.2 数据大小](./section-2.1.2-数据大小与sizeof.md) → **§2.1.3 本节**

---

### 寻址 (addressing)

- CPU 通过 **地址总线** 访问内存；x86-64 为 **按字节寻址 (byte-addressable)** — **每个字节** 有唯一地址
- 变量 `int x` 占 **4 个连续字节** 的地址，例如 `0x1000`–`0x1003`
- `&x` = 对象 **起始（最低）字节** 的地址
- 指针算术：`char* p; p+1` 前进 1 字节；`int* q; q+1` 前进 `sizeof(int)` 字节 — 见 [§3.8 指针步长](../../chapter-03-machine-level-programs/notes/section-3.8-指针步长详解.md)

**寻址回答：** 数据在 **哪几个字节地址** 上。

### 字节顺序 (byte order / endianness)

多字节对象占多个连续地址时，**各字节在地址上的排列顺序**。

| | 英文 | 规则 | 典型 |
|--|------|------|------|
| **小端** | Little-Endian | **最低有效字节** 在 **低地址** | x86-64、多数 ARM |
| **大端** | Big-Endian | **最高有效字节** 在 **低地址** | **网络字节序**、不少协议 |

> **术语：** byte order / **endianness**

**字节序回答：** 同一数值的多个字节，**谁放在低地址**。

> **易混：** 寻址 ≠ 字节序。先确定占哪段地址，再谈 `0x78` 和 `0x12` 谁先谁后。

### 示例：`int x = 0x12345678`

| 相对地址 | 小端 | 大端 |
|----------|------|------|
| 低 | 0x78 | 0x12 |
| +1 | 0x56 | 0x34 |
| +2 | 0x34 | 0x56 |
| +3 | 0x12 | 0x78 |

`0x11223344`：**小端** `44 33 22 11`；**大端** `11 22 33 44`。

### 和字符串的直觉对比（类比，不是定义）

| | 内存低→高 | 直觉 |
|--|-----------|------|
| `"1234"` | `31 32 33 34` | 书写从左到右，**接近大端直觉** |
| 小端 `0x12345678` | `78 56 34 12` | **低位先放低地址**，与字符串 **相反** |

### 逐字节查看

```c
int a = 0x11223344;
unsigned char *p = (unsigned char *)&a;
/* x86 小端: 44 33 22 11 */
for (size_t i = 0; i < sizeof a; i++)
    printf("%02x ", p[i]);
```

**动手：**

| demo | 内容 |
|------|------|
| [ch02-endian-and-padding-demo.c](../../code/ch02-endian-and-padding-demo.c) | sizeof + endian + padding 预告 |
| [pointer-and-bytes.c](../../code/pointer-and-bytes.c) | `unsigned char *` 逐字节 |
| [pointer-stride-demo.c](../../code/pointer-stride-demo.c) | `char*` vs `int*` 步长 |

### 网络序转换

```c
#include <arpa/inet.h>   /* Linux */
uint32_t net  = htonl(host);
uint32_t host = ntohl(net);
```

### 相关：Padding（Ch3）

wire 传 struct：**endian + padding + 类型宽度** 须一起约定 → [§3.9 结构体与对齐](../../chapter-03-machine-level-programs/notes/section-3.9-结构体联合与对齐.md)

### HFT

- 线上字段常 **big-endian**；须 `ntoh*` / schema 生成
- **禁止** `write(fd, &m, sizeof m)` 裸发本地 struct
- 同机 mmap：须同 endian、同 ABI、同 padding

→ [Ch 11 网络编程](../../chapter-11-network-programming/) · [11 UNP](../../../11-UNP-Vol1/)

---

## 口述巩固 · 自测

1. 「寻址」和「字节序」分别回答什么问题？
2. `0x11223344` 在 x86 小端，低地址第一字节是 `0x11` 还是 `0x44`？
3. 字符串 `"1234"` 为何不能用来推断整数 endian？
4. `htonl` 解决的是哪一层问题？

---

→ 下一节：[§2.1.4 字符串与位操作](./section-2.1.4-2.1.9-字符串布尔与C位操作.md) · ← [§2.1.2](./section-2.1.2-数据大小与sizeof.md) · [Ch2 导读](../README.md)
