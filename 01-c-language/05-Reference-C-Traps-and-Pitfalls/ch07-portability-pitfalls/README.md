# 第 7 章 可移植性缺陷

**Portability Pitfalls** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

[ch06 预处理器](../ch06-preprocessor/) 之后，本章聚焦 **换 CPU/OS/编译器后行为突变**：整型宽度、`char` 符号性、字节序、对齐、指针宽度、右移、OS API、浮点、NULL、平台宏 —— 嵌入式、跨架构驱动、多端工具链高频区。

## 小节索引

| 节 | 主题 |
|----|------|
| [7.1](./7.1-整型宽度.md) | `int`/`long` 不固定 → `stdint.h` |
| [7.2](./7.2-char符号性.md) | `char` signed vs unsigned |
| [7.3](./7.3-大小端.md) | `htonl` / 勿强转指针 |
| [7.4](./7.4-结构体对齐.md) | padding、pack、offsetof |
| [7.5](./7.5-指针宽度.md) | `uintptr_t` |
| [7.6](./7.6-有符号右移.md) | 算术 vs 逻辑右移 |
| [7.7](./7.7-OS与库差异.md) | POSIX vs Win32 vs RTOS |
| [7.8](./7.8-浮点差异.md) | FPU、NaN、比较 |
| [7.9](./7.9-NULL与地址空间.md) | NULL 解引用与平台 |
| [7.10](./7.10-平台宏.md) | `__x86_64__` / `_WIN32` 封装 |

## 跨平台编码规范

1. 协议/寄存器：**`stdint.h` 定宽类型**
2. 二进制字节：**`unsigned char` / `uint8_t`**
3. 跨设备传输：**网络字节序**，禁止 `*(uint32_t*)buf`
4. 硬件布局：**手动对齐** + `static_assert(offsetof)`
5. 指针整数化：**`uintptr_t`**
6. 移位：**无符号** 或显式掩码
7. 多 OS：**标准 C** + 薄平台抽象层
8. 不依赖默认 char 符号、对齐、有符号 `>>`

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch06 预处理器](../ch06-preprocessor/) |
| **后置** | [ch08 建议与答案](../ch08-advice-and-answers/) |
| **交叉** | [Expert C ch07 内存布局](../03-Advanced-Expert-C-Programming/ch07-the-shapes-that-memory-takes/) |

## Demo

```bash
cd demo && make all
./demo01_stdint/main
./demo02_char_sign/main
./demo03_endian/main
./demo04_padding/main
./demo05_uintptr/main
./demo06_shift/main
./demo07_platform/main
```

## 面试题

1. LP64 下 `long` 几字节？为何时间戳用 `int64_t`？
2. `char c=0x80; c<0` 为何跨平台不一致？
3. 小端机如何读网络大端 `uint32_t`？
4. `struct { char a; int b; }` 为何 sizeof 常是 8？
5. 为何不能用 `int` 存 64 位指针？

## 章节自测

> 可移植性陷阱：换 CPU/OS 后行为突变。看代码 → 想答案 → 点开验证。

### Q1: 整型宽度不固定

```c
long timestamp = 1700000000;  // Unix 时间戳

// 32 位 Linux (ILP32)：sizeof(long) = ?
// 64 位 Linux (LP64)：sizeof(long) = ?
// 64 位 Windows (LLP64)：sizeof(long) = ?

printf("%ld\n", timestamp);
```

> 三个平台上 `sizeof(long)` 分别是多少？哪个可能出问题？

<details>
<summary>答案与复习指引</summary>

**答案：**
- 32 位 Linux (ILP32)：`long` = 4 字节
- 64 位 Linux (LP64)：`long` = **8** 字节
- 64 位 Windows (LLP64)：`long` = **4** 字节

2038 年问题：`time_t` 在 32 位系统上是 `long`（4 字节）→ 2038-01-19 溢出。64 位 Linux 上 `time_t` = 8 字节，安全。

**规则：** 协议/寄存器用 `int32_t`/`int64_t`（`stdint.h` 定宽类型），不用 `int`/`long`。

**复习：** → [7.1 整型宽度](./7.1-整型宽度.md)

</details>

### Q2: char 符号性

```c
char c = 0x80;
if (c < 0)
    printf("negative\n");
else
    printf("non-negative\n");
```

> 在 x86 和 ARM 上输出可能不同吗？

<details>
<summary>答案与复习指引</summary>

**答案：** **是的**。`char` 的符号性是**实现定义**的：
- x86（gcc 默认）：`char` 是 **signed** → `c = 0x80` = -128 → 输出 `negative`
- ARM（gcc 默认）：`char` 是 **unsigned** → `c = 0x80` = 128 → 输出 `non-negative`

**规则：** 处理二进制数据用 `unsigned char` 或 `uint8_t`；处理 ASCII 字符也用 `unsigned char` 传给 ctype 函数。

**复习：** → [7.2 char 符号性](./7.2-char符号性.md)

</details>

### Q3: 大小端

```c
uint32_t val = 0x01020304;
char *p = (char *)&val;

printf("%02x %02x %02x %02x\n", p[0], p[1], p[2], p[3]);
```

> 在小端机和大端机上分别输出什么？

<details>
<summary>答案与复习指引</summary>

**答案：**
- **小端**（x86）：`04 03 02 01`（低位字节在低地址）
- **大端**（网络字节序/某些 ARM）：`01 02 03 04`（高位字节在低地址）

**网络通信：** 发送方必须用 `htonl()`/`htons()` 转为网络字节序（大端），接收方用 `ntohl()`/`ntohs()` 转回。**禁止** `*(uint32_t*)buf` 直接读取网络缓冲区。

**复习：** → [7.3 大小端](./7.3-大小端.md)

</details>

### Q4: 结构体对齐 padding

```c
struct Packed {
    char a;     // 1 byte
    int b;      // 4 bytes
    char c;     // 1 byte
};

struct Aligned {
    char a;
    char c;
    int b;
};

printf("%zu %zu\n", sizeof(struct Packed), sizeof(struct Aligned));
```

> 两个 sizeof 分别是多少？（假设 4 字节对齐）

<details>
<summary>答案与复习指引</summary>

**答案：** `12 8`。

`struct Packed`（a + 3padding + b + c + 3padding = 12）：
```
[a][pad][pad][pad][b][b][b][b][c][pad][pad][pad]
```

`struct Aligned`（a + c + 2padding + b = 8）：
```
[a][c][pad][pad][b][b][b][b]
```

**规则：** 按对齐要求从大到小排列成员（double → int → short → char），减少 padding。协议头用 `__attribute__((packed))` 但注意可能引发未对齐访问。

**复习：** → [7.4 结构体对齐](./7.4-结构体对齐.md)

</details>

### Q5: 有符号右移

```c
int x = -8;           // 0xFFFFFFF8
int y = x >> 1;       // 算术右移？逻辑右移？

unsigned z = 0xFFFFFFF8u;
unsigned w = z >> 1;  // 逻辑右移

printf("y=%d w=%u\n", y, w);
```

> `y` 和 `w` 分别是多少？`y` 的行为是 UB 吗？

<details>
<summary>答案与复习指引</summary>

**答案：**
- `w = 0x7FFFFFFC` = 2147483644（无符号右移是**逻辑右移**，高位补 0，标准保证）
- `y`：**实现定义**（不是 UB）。大多数编译器对有符号数做**算术右移**（高位补符号位 1）→ `y = -4`。但标准允许逻辑右移。

**规则：** 需要确定性行为时用 `unsigned` 做位移，或用显式掩码。

**复习：** → [7.6 有符号右移](./7.6-有符号右移.md)

</details>
