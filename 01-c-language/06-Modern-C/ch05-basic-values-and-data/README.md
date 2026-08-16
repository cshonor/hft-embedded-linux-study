# Ch5 · Basic values and data（基本值和数据）

> **Level 1 · 相识** · 策略：**🟡 略读**（聚焦 C23 类型增量）
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

基本类型、字面量、初始化器、命名常量。K&R Ch2 已覆盖 C89 类型体系；本章重点是
**C99–C23 的类型增量**：`_BitInt(N)`、`constexpr`、`{}` 默认初始化、`0b` 前缀、`<stdbit.h>`。

## 一、基本类型全表（C23 版）

### 整数类型

| 类型 | 最小宽度 | 备注 |
|------|----------|------|
| `char` | 8 bit | 语义可为 signed/unsigned（实现定义），`char` 独立于其它两种 |
| `signed char` / `unsigned char` | 8 bit | 明确符号性 |
| `short` / `unsigned short` | 16 bit | 至少 16 |
| `int` / `unsigned int` | 16 bit | 实际通常 32 bit |
| `long` / `unsigned long` | 32 bit | Linux 64-bit: 64 bit; Windows 64-bit: 32 bit |
| `long long` / `unsigned long long` | 64 bit | C99 引入 |
| `_BitInt(N)` | N bit | **C23 新增**：精确位宽整数 |
| `bool` | — | C23 关键字（原 `_Bool`，C99） |

### `_BitInt(N)` — C23 精确位宽整数

```c
/* C23：精确位宽（不是stdint.h的定宽类型） */
_BitInt(7)  x = 50;    // 7位有符号：范围 -64..63
_BitInt(24) y = 0xFFFFFF;  // 24位：可精确匹配24位协议字段
unsigned _BitInt(48) z;    // 48位无符号：MAC地址
```

| 对比 | `uint32_t` | `_BitInt(32)` |
|------|-----------|---------------|
| 来源 | `<stdint.h>`（typedef） | C23 关键字 |
| 宽度 | exactly 32（如存在） | exactly 32 |
| 可用宽度 | 8/16/32/64（平台提供） | 任意 N（编译器限制内） |
| 用途 | 通用定宽 | 非标准宽度字段（如 24-bit 颜色、48-bit MAC） |

> HFT 场景：某些行情协议有 24-bit 字段或 48-bit 时间戳，`_BitInt` 可精确匹配，不需要手动拼凑。

### `size_t` / `ptrdiff_t` / `intptr_t`

| 类型 | 用途 | 备注 |
|------|------|------|
| `size_t` | `sizeof` 的返回类型、数组大小 | 无符号，`<stddef.h>` |
| `ptrdiff_t` | 指针减法结果 | 有符号，`<stddef.h>` |
| `intptr_t` | 可存指针的整数 | C99，`<stdint.h>`；用于指针↔整数转换 |
| `uintptr_t` | 无符号版 | 同上 |

> **HFT 注意**：`size_t` 是无符号的——`for (size_t i = n-1; i >= 0; i--)` 是死循环！用 `ssize_t` 或倒序迭代。

### 字符类型

| 类型 | C23 变化 |
|------|----------|
| `char` | C23 新增 `char8_t`（UTF-8 字符） |
| `char8_t` | `unsigned char` 的 typedef，用于 UTF-8 |
| `wchar_t` | 宽字符（宽度实现定义），HFT 基本不用 |

## 二、字面量

### 整数字面量前缀

```c
42          // int / long（根据值自动扩展）
42U         // unsigned
42L         // long
42LL        // long long（C99）
42ULL       // unsigned long long（C99）

0b101010    // C23：二进制字面量
0B101010    // 同上（大小写均可）
0x2A        // 十六进制
052         // 八进制（注意前导零）
```

### 整数字面量分隔符（C23）

```c
/* C23：单引号作为数字分隔符（提升可读性） */
int billion  = 1'000'000'000;
uint64_t mask = 0xFF'FF'FF'FF'00'00'00'00;
uint32_t ip   = 192'168'001'001;   // 注意：这不是IP地址，只是数字分隔
```

> 单引号在数字中间纯粹是视觉分隔，编译器忽略。HFT 场景：大常量、位掩码可读性大幅提升。

### 浮点字面量

```c
3.14        // double
3.14f       // float
3.14L       // long double
6.022e23    // 科学计数法

/* C23：十六进制浮点（C99 已有，C23 完善） */
0x1.8p1     // = 1.5 * 2^1 = 3.0
```

## 三、初始化器

### `{}` 默认初始化（C23）

```c
/* C23：空初始化器 → 零初始化 */
int arr[10] = {};       // 全零（C23 之前要写 {0}）
struct packet pkt = {};  // 全成员零初始化

/* C99/C17：等价写法 */
int arr2[10] = {0};     // 第一个元素显式0，其余零初始化
```

| 写法 | 标准 | 效果 |
|------|------|------|
| `= {0}` | C89+ | 零初始化（约定俗成的写法） |
| `= {}` | **C23** | 零初始化（更清晰） |
| `= { .field = 0 }` | C99+ | 指定初始化器 |

### 指定初始化器（C99）

```c
struct msg_hdr {
    uint16_t magic;
    uint8_t  type;
    uint8_t  flags;
    uint32_t seq;
};

/* C99 指定初始化器：只初始化需要的字段，其余零初始化 */
struct msg_hdr hdr = {
    .magic = 0xABCD,
    .type  = 0x01,
    // flags 和 seq 自动为 0
};

/* 数组指定初始化器 */
int lookup[256] = {
    ['A'] = 1,
    ['B'] = 2,
    ['C'] = 3,
    // 其余为 0
};
```

> DPDK 和内核大量使用指定初始化器：`struct file_operations`、`struct rte_pci_driver` 等。

## 四、命名常量

### 三种方式对比

```c
/* ① 宏（C89） */
#define MAX_ORDERS 65536
#define MAGIC       0xABCD

/* ② enum（C89） */
enum { MAX_ORDERS = 65536, MAGIC = 0xABCD };

/* ③ constexpr（C23） */
constexpr int max_orders = 65536;
constexpr uint16_t magic = 0xABCD;
```

| 特性 | `#define` | `enum` | `constexpr`（C23） |
|------|----------|--------|---------------------|
| 类型安全 | ❌ | ⚠️（总是 int） | ✅ |
| 调试器可见 | ❌ | ✅ | ✅ |
| 作用域 | 文件 | 块/文件 | 块/文件 |
| 取地址 | ❌ | ❌ | ✅ |
| 浮点常量 | ✅ | ❌ | ✅ |
| 用于 `switch` case | ✅ | ✅ | ✅ |
| 用于数组大小 | ✅ | ✅ | ✅ |
| 用于 `static` 初始化 | ✅ | ✅ | ✅ |

> **HFT 建议**：新代码用 `constexpr`；宏保留给条件编译（`#ifdef`）和字符串拼接。

## 五、`<stdbit.h>`（C23 新增）

C23 标准库新增位操作头文件，提供跨平台内建函数：

```c
#include <stdbit.h>

/* 查位 */
int n = stdc_count_ones(0xFF);      // = 8（popcount）
int z = stdc_count_zeros(0xFF);     // = 0
int f = stdc_first_leading_one(0x80); // = 7（MSB位置）

/* 旋转 */
unsigned r = stdc_rotl(0x0F, 4);    // = 0xF0（循环左移4位）

/* 字节序检测 */
unsigned endian = stdc_endian_sc(); // 返回当前字节序
```

| 函数 | 等价 GCC 内建 | 说明 |
|------|--------------|------|
| `stdc_count_ones(x)` | `__builtin_popcount(x)` | 统计 1 的个数（popcount） |
| `stdc_first_leading_one(x)` | `__builtin_clz` 取反 | 前导 0 计数 |
| `stdc_first_trailing_one(x)` | `__builtin_ctz` | 尾随 0 计数 |
| `stdc_rotl(x, n)` | 手写 | 循环左移 |

> HFT 场景：行情解析中位标志提取、哈希函数的 avalanche mixing。以前用 `__builtin_popcount`，C23 后可用标准接口。

## HFT / DPDK 关联

| 特性 | HFT 用途 |
|------|----------|
| `constexpr` | 消息结构 magic number、协议常量、缓冲区大小 |
| `{}` 初始化 | 清零消息结构、重置缓冲区 |
| `_BitInt(24/48)` | 非标准宽度协议字段（24-bit 量、48-bit 时间戳） |
| 数字分隔符 `1'000'000` | 大常量、位掩码可读性 |
| `<stdbit.h>` | popcount、前导零计数（哈希、位标志） |
| `0b` 前缀 | 位掩码定义：`0b10101010` 比 `0xAA` 更直观 |

## 自测题

<details><summary>1. <code>for (size_t i = n-1; i &gt;= 0; i--)</code> 有什么问题？</summary>

`size_t` 是无符号类型，`i >= 0` 永远为真——当 `i` 减到 0 再减 1 变成 `SIZE_MAX`，循环不会终止。
正确写法：用 `for (size_t i = n; i-- > 0; )` 或改用有符号的 `ssize_t`/`ptrdiff_t`。
</details>

<details><summary>2. C23 的 <code>{}</code> 和 C99 的 <code>{0}</code> 有什么区别？</summary>

语义上等价（都做零初始化），但 `{}` 更清晰——空初始化器明确表达"全部默认零"。
`{0}` 是"第一个元素为 0，其余递归零初始化"的约定俗成写法，C23 之前不支持空 `{}`。
</details>

<details><summary>3. <code>_BitInt(24)</code> 和 <code>uint32_t</code> 有什么区别？</summary>

`_BitInt(24)` 是 C23 新增的精确 24 位整数（只用 3 字节存储空间，`sizeof` 为 3）。
`uint32_t` 是 32 位无符号整数（4 字节）。
当协议字段恰好是 24 位时，`_BitInt(24)` 可以精确匹配，避免浪费空间或手动拼接字节。
但 `_BitInt` 的运算可能比标准宽度类型慢（非原生宽度），热路径需 benchmark。
</details>

<details><summary>4. <code>constexpr</code> 和 <code>enum</code> 何时各用哪个？</summary>

`enum`：简单整型常量、`switch` case 标签、状态机枚举值。
`constexpr`：需要类型安全（非 int 类型如 `uint16_t`）、浮点常量、需要取地址、需要块级作用域的场景。
新代码倾向 `constexpr`，但 `enum` 对于"一组相关整数常量"的可读性更好。
</details>
