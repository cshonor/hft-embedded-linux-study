## 2.1.1–2.1.3 十六进制、数据大小、寻址与字节顺序

> **本节一条线：** hex 怎么读 → 类型占几字节（`sizeof`/ABI）→ 地址怎么找字节 → 多字节内部怎么排（endian）。

---

### 2.1.1 十六进制表示法

- 二进制太长，十六进制 **每 1 位 hex = 4 bit**，便于读地址与位模式
- C 字面量：`0xFF`、`0xDEADBEEF`

| 十进制 | 十六进制 | 二进制（8 bit） |
|--------|----------|---------------|
| 255 | 0xFF | 11111111 |
| 16 | 0x10 | 00010000 |

**习惯：** 调试内存 dump、协议抓包、寄存器值 — 一律用 hex；**地址** 也常用 hex（如 `0x7ffd…`）。

**2 的幂速查（算对齐/缓冲区时常用）：**

| 2^n | 十进制 | hex |
|-----|--------|-----|
| 2^8 | 256 | 0x100 |
| 2^10 | 1024 | 0x400 |
| 2^16 | 65536 | 0x10000 |
| 2^32 | … | 0x100000000 |

---

### 2.1.2 数据大小

#### `sizeof(T)` 是什么？

- **编译期常量** — 编译器根据 **目标架构 + ABI** 在编译阶段填好，**不是**运行时 `malloc` 量出来的
- 同一份 `.c`，换 `-m32`/`-m64`、换 OS、换交叉编译目标，**sizeof 可以变**
- CSAPP 提醒：**可移植代码不要假设 `int` 一定是 4 字节**（老架构/部分嵌入式上 `int` 可能是 2 字节）

#### 典型 ABI 对照（64 位时代最容易混）

| 类型 | ILP32（32 位） | LP64（Linux/macOS x86-64） | LLP64（Windows x64） |
|------|----------------|----------------------------|----------------------|
| `int` | 4 | 4 | 4 |
| `long` | 4 | **8** | **4** |
| 指针 | 4 | 8 | 8 |
| `size_t` | 4 | 8 | 8 |

**要点：** 同样是 64 位 CPU，`sizeof(long)` 在 Linux 上常为 **8**，在 Windows x64 上常为 **4** — 差在 **ABI 命名（LP64 vs LLP64）**，不是「CPU 64 位」 alone 决定的。

#### LP64 Linux x86-64 常见宽度

| C 类型 | 字节 |
|--------|------|
| `char` | 1 |
| `short` | 2 |
| `int` | 4 |
| `long` | 8 |
| `float` | 4 |
| `double` | 8 |
| 指针 | 8 |

- **字长 (word size)** — 机器「自然」一次处理的数据宽度；64 位机里指针/`long`（LP64）常为 8 字节

#### 本机一眼确认

```c
#include <stdio.h>

int main(void) {
    printf("sizeof(char)=%zu int=%zu long=%zu void*=%zu\n",
           sizeof(char), sizeof(int), sizeof(long), sizeof(void*));
    return 0;
}
```

#### 跨平台怎么写

```c
#include <stdint.h>

int32_t  price_tick;   /* 协议字段：宽度写死 */
int64_t  qty;
uint64_t seq;
/* 避免：wire 格式用 int/long，换平台长度就变 */
```

#### HFT / 跨平台踩坑

| 坑 | 后果 |
|----|------|
| wire 协议用 `long` / 本地 `struct` 直接 `send()` | LLP64 vs LP64 长度不一致 |
| 假设 `int` 永远 4 字节 | 嵌入式 ABI 上布局全错 |
| Rust/C++ 共享内存未约定 layout | 宽度 + padding + endian 三重不一致 |

**HFT：** 二进制 schema 用 **SBE / 手写 offset + 固定宽度**；文本 FIX 也要分清类型与 scale，不能靠本地 `long`。

---

### 2.1.3 寻址和字节顺序 (Byte Ordering)

#### 寻址 (addressing)

- CPU 通过 **地址总线** 访问内存；x86-64 为 **按字节寻址 (byte-addressable)** — **每个字节** 有唯一地址
- 变量 `int x` 占 **4 个连续字节** 的地址，例如 `0x1000`、`0x1001`、`0x1002`、`0x1003`
- `&x` = 对象 **起始（最低）字节** 的地址
- 指针算术：`char* p; p+1` 前进 1 字节；`int* q; q+1` 前进 `sizeof(int)` 字节

**寻址回答：** 数据在 **哪几个字节地址** 上。

#### 字节顺序 (byte order / endianness)

多字节对象占多个连续地址时，**各字节在地址上的排列顺序**。

| | 英文 | 规则 | 典型平台/场景 |
|--|------|------|---------------|
| **小端** | Little-Endian | **最低有效字节** 在 **低地址** | x86-64、多数 ARM 默认 |
| **大端** | Big-Endian | **最高有效字节** 在 **低地址** | **网络字节序**、不少二进制协议 |

**字节序回答：** 同一数值的多个字节，**谁放在低地址**。

> **易混：** 寻址 ≠ 字节序。先确定占哪段地址，再谈这段里 `0x78` 和 `0x12` 谁先谁后。

#### 示例：`int x = 0x12345678`

从低地址 `&x` 起 4 字节：

| 相对地址 | 小端 | 大端 |
|----------|------|------|
| 低 | 0x78 | 0x12 |
| +1 | 0x56 | 0x34 |
| +2 | 0x34 | 0x56 |
| +3 | 0x12 | 0x78 |

换 `int a = 0x11223344`：**小端** 低地址起 `44 33 22 11`；**大端** 为 `11 22 33 44`（规则相同，仅数值不同）。

#### 和字符串的直觉对比（类比，不是定义）

| | 内存低地址 → 高地址 | 直觉 |
|--|---------------------|------|
| `"1234"` | `'1' '2' '3' '4'`（0x31…0x34） | 书写 **从左到右**，像「高位/左侧先出现」→ **接近大端直觉** |
| 小端 `0x12345678` | `78 56 34 12` | **低位字节先放低地址**，与字符串习惯 **相反** |

本质：**数值的字节排列**，不是「从左写还是从右写」那么简单。

#### 逐字节查看（x86 小端验证）

```c
int a = 0x11223344;
unsigned char *p = (unsigned char *)&a;
/* x86 小端期望打印: 44 33 22 11 */
for (size_t i = 0; i < sizeof a; i++)
    printf("%02x ", p[i]);
```

完整 demo → [01-CSAPP-3rd/code/ch02-endian-and-padding-demo.c](../../../01-CSAPP-3rd/code/ch02-endian-and-padding-demo.c)  
C 指针专练 → [02-c-programming/code/pointer-and-bytes.c](../../../02-c-programming/code/pointer-and-bytes.c)

#### 网络序转换

```c
#include <arpa/inet.h>   /* Linux */
uint32_t net  = htonl(host);  /* host → network (通常 big-endian) */
uint32_t host = ntohl(net);
```

#### 相关：Padding（在 Ch3，常与 endian 一起考）

**Padding** = 编译器为满足 **对齐 (alignment)** 在结构体里插入的 **空闲字节**。例如 `char` 后跟 `int` 时，常补 3 字节让 `int` 落在 4 字节边界 — 详见 [Ch3 §3.9 结构体与对齐](../../chapter-03-machine-level-programs/notes/section-3.9-结构体联合与对齐.md)。

wire 传 struct 时：**endian + padding + 类型宽度** 必须一起约定，不能只换 `ntoh*`。

#### HFT

- 线上字段常 **big-endian**；解析必须 `ntoh*` / 显式 swap / schema 代码生成
- **禁止** 把本地 `struct Message` 直接 `write(fd, &m, sizeof m)` — 字节序、对齐、版本都会炸
- 同机 mmap 共享：两端须同 endian、同 ABI、同 padding 规则

→ [Ch 11 网络编程](../../chapter-11-network-programming/) · [11 UNP](../../../11-UNP-Vol1/)

---

### 口述巩固 · 自测

1. `sizeof(long)` 在 Linux x64 和 Windows x64 各是多少？为什么同为 64 位却不同？
2. 「寻址」和「字节序」分别回答什么问题？
3. `0x11223344` 在 x86 小端内存里，低地址第一个字节是 `0x11` 还是 `0x44`？
4. 为什么协议字段用 `int32_t` 而不是 `int`？
5. 字符串 `"1234"` 的字节顺序为什么 **不能** 用来推断整数 endian？

---

← [本章导读](../README.md) · [Ch3 结构体对齐](../../chapter-03-machine-level-programs/notes/section-3.9-结构体联合与对齐.md)
