## ③ 特定数据类型

#### 不透明类型 · Opaque Types

| 示例 | 用途 |
|------|------|
| **`pid_t`**、**`atomic_t`**、**`dev_t`** | **隐藏内部格式** |

| 规则 | 不假设大小 · **不** 强转为 `int`/`long` 乱用 |

**完整清单（v6.6 常用）：**

| 类型 | 含义 | 底层 |
|------|------|------|
| `pid_t` | 进程 ID | `int` |
| `uid_t` / `gid_t` | 用户/组 ID | `unsigned int` |
| `dev_t` | 设备号 | `u32`（主 12 + 次 20，见 Ch 17.1） |
| `sector_t` | 磁盘扇区号 | `u64` 或 `unsigned long`（`CONFIG_LBDAF`） |
| `blkcnt_t` | 块计数 | `u64` |
| `loff_t` | 文件偏移（**总是 64 位**，哪怕 32 位系统） | `long long` |
| `gfp_t` | 分配标志 | `unsigned int` |
| `ktime_t` | 内核时间（纳秒） | `s64` |
| `cycles_t` | TSC 周期计数 | `u64` |
| `atomic_t` / `atomic64_t` | 原子计数器 | 结构体（内含 `int`/`s64`） |
| **`refcount_t`** | **引用计数**（v4.11+） | 结构体（内含 `unsigned int`） |

> **`atomic_t` 与 `refcount_t` 的分家（v4.11+）** 是一个值得记住的演进：

| | `atomic_t` | `refcount_t` |
|---|-----------|-------------|
| 用途 | 通用原子计数 | **专门做"对象引用计数"** |
| 溢出行为 | **回绕**（`INT_MAX + 1` → `INT_MIN`） | **饱和 + 警告**（永远停在 `UINT_MAX`，并 `WARN`） |
| 释放语义 | 无 | 归零时**明确语义**（`refcount_dec_and_test`） |

> 为什么分家？因为"引用计数归零就释放对象"这个模式太常见，
> 而 `atomic_t` 的**回绕**会让计数从 0 跳回 `UINT_MAX` → 对象**永不释放**，或更糟：
> 计数回绕成 1 后又被减到 0 → **二次释放（use-after-free）**。
> `refcount_t` 用饱和语义把这类漏洞变成一次 `WARN` + 拒绝，是**把一类安全漏洞变成可观测错误**的设计。
>
> **规则：** 只要是"引用计数"，用 `refcount_t`；只有真正的通用原子计数才用 `atomic_t`。

#### 明确大小的类型

| 场景 | 类型 |
|------|------|
| 硬件寄存器、网络包、磁盘结构 | **`u8` `s16` `u32` `s64`** 等 |

| 导出到用户空间 | **`__u32`** 等 **双下划线** 前缀 — 避免与用户头文件 **命名冲突** |

> **为什么 UAPI 头要加双下划线？** 因为 `include/uapi/linux/*.h` 会被**用户态程序**直接 `#include`。
> 如果它定义了无前缀的 `u32`，就可能和用户程序自己的 typedef 撞名（"redefinition" 编译错误）。
> 所以内核内部用 `u32`，**暴露给用户态的用 `__u32`**（`typedef unsigned int __u32;`）。
> 这是"命名空间卫生"的一个具体体现。

#### `char` 的符号性

| 事实 | C 标准 **未规定** `char` 有符号或无符号 |
|------|----------------------------------------|
| x86 | 通常 **signed char** |
| ARM / ARM64 / PowerPC | 通常 **unsigned char** |

| 要求 | 用 **`signed char`** 或 **`unsigned char`** 明示 |

**这个差异会怎样咬人：**

```c
char c = 0xFF;              /* x86: 值 = -1      ARM: 值 = 255  */
if (c == 0xFF) { ... }      /* x86: false!  ARM: true  */

char buf[256];
int i = buf[0];             /* x86: 可能是负数（符号扩展）→ 越界下标 */

/* 用 char 做查找表下标 */
static int table[256];
int v = table[c];           /* x86 上 c < 0 → 下标越界，读到 table[-1] 之前的内存 */
```

| 场景 | 必须写 |
|------|--------|
| 字节数据 / 缓冲区 | **`unsigned char *`** 或 `u8 *` |
| 小整数（可能为负） | **`signed char`** 或 `s8` |
| 字符处理 | `char`（但要 `isalpha((unsigned char)c)`） |

> **C 标准库也有这个坑**：`isalpha()`、`tolower()` 等函数的参数类型是 `int`，
> 但要求值必须在 `unsigned char` 范围内**或**是 EOF。
> 直接传一个 `signed char`（值为负）是**未定义行为**——glibc 里表现为"跳表越界读"。
> 所以标准写法是 `isalpha((unsigned char)c)`。

#### 还有两个容易忽略的

| 类型 | 注意 |
|------|------|
| **`bool`** | 内核用 `bool` + `true/false`（来自 `<stdbool.h>`），但 ABI 上仍是 `int`（`_Bool` → 1 字节）。**不要**假设 `sizeof(bool) == 1` 用于结构体布局 |
| **`void *` 算术** | `ptr + 1` 是 GCC 扩展，标准不支持；可移植写法是 `(char *)ptr + 1` 或用 `uintptr_t` |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核中的不透明类型有哪些？为什么不直接用底层类型？

<details><summary>答案</summary>

不透明类型：pid_t（进程ID）、gid_t（组ID）、uid_t（用户ID）、dev_t（设备号）。不直接用 int 是因为：不同架构/内核版本可能改变底层类型大小（pid_t 可能是 int 或 unsigned int）。用 typedef 隔离，代码不依赖具体类型大小。HFT 代码中传递这些值时也应该用对应类型而非 int。

</details>

**Q2.** `atomic_t` 和 `refcount_t` 都能做计数，什么时候必须用后者？

<details><summary>答案</summary>

**只要是"引用计数"（计数归零就释放对象），就用 `refcount_t`。**

核心差别在**溢出时的行为**：

| | `atomic_t` | `refcount_t` |
|---|-----------|-------------|
| 计数从 0 再减 | **回绕**成 `UINT_MAX` | **饱和**在 0，并 `WARN` 一次 |
| 计数加到溢出 | 回绕成 0 | 停在 `UINT_MAX`，`WARN` |
| 归零释放语义 | 要自己 `atomic_dec_and_test` | `refcount_dec_and_test`（语义明确） |
| 引入版本 | 一直有 | **v4.11+** |

**为什么这个差别是安全问题：**

```
场景：一个对象 refcount = 1
  ① 代码多减了一次（bug）
  ② atomic_t:  1 → 0 → 对象被释放 → 再减 → 0 → UINT_MAX（回绕）
     之后任何一次"加 1"都让它变成 0 → 又触发一次释放 → **double free / UAF**
  ③ refcount_t: 1 → 0 → 释放 → 再减 → 检测到已经是 0 → **WARN + 停在 0**
     攻击/ bug 无法把计数推回非零 → 无法触发第二次释放
```

历史上 `atomic_t` 的回绕被反复利用于内核漏洞利用（先 UAF 再二次释放）。
`refcount_t` 把这个"可利用的原语"降级成一次内核警告——**把一类安全漏洞变成可观测错误**，
这是内核安全加固里非常典型的一类思路。

**实用的选择规则：**

| 需求 | 用 |
|------|-----|
| 对象引用计数（归零释放） | **`refcount_t`** |
| 统计计数（包数、错误数，允许回绕/无所谓） | `atomic_t` / `atomic64_t` |
| 需要"减到 0 就做事" | `refcount_dec_and_test()` |
| 位操作、CAS 循环 | `atomic_t` |
| per-CPU 计数 | `local_t` / `percpu_counter` |

**用户态对照**：C++ 里 `std::shared_ptr` 的引用计数也是 `size_t` 的原子增减，
理论上同样能溢出（虽然需要 2^64 次，不现实）。真正对应 `refcount_t` 饱和语义的是
Rust 的 `Arc`——它在溢出时直接 `abort()`，同样是"宁可崩，不要 UAF"的思路。

</details>

**Q3.** 一段代码在 x86 上跑得好好的，迁到 ARM64 上出现数组越界，查下来是 `char` 的问题——具体是怎么回事？

<details><summary>答案</summary>

典型写法：

```c
static const int price_table[256];       /* 用字节值当下标 */

unsigned char *p = (unsigned char *)buf; /* 看着没问题 */
char c = *p;                             /* ← 这里！ */
int v = price_table[c];                  /* x86: c 可能是负数 → 下标越界 */
```

**根因：** C 标准**没有规定** `char` 默认是有符号还是无符号，这是 **implementation-defined**：

| 平台 | `char` 默认 | `char c = 0xFF` 的值 |
|------|------------|---------------------|
| x86 / x86_64 (GCC/Clang) | **signed** | **-1** |
| **ARM / ARM64 / PowerPC** | **unsigned** | **255** |

代码在 x86 上开发测试时，`c = 0xFF` 变成 `-1`，于是 `price_table[-1]` —— 读到了数组**前面**的内存。
在 x86 上那块内存恰好没映射或内容无害，看起来"没事"；
到了 ARM64 上 `c = 255`，`price_table[255]` 是合法访问，**反而正常**。

等等，那为什么是 ARM64 上出问题？因为**方向相反**的情形更常见：

```c
/* 另一类：用 char 存小数值并比较 */
char c = get_byte();
if (c == 0xFF) { ... }      /* x86: -1 == 255 → false（永远进不来！）
                               ARM: 255 == 255 → true */
```
这种 bug 在 x86 上表现为"这个分支永远不生效"，在 ARM64 上突然生效 → 行为不一致。

**还有一类更危险：用 `char` 做循环/比较后当无符号用**
```c
char c = 0x80;                     /* x86: -128 */
while (c < 200) { ...; c++; }      /* x86: -128...127 → 到 127 后溢出成 -128 → **死循环**
                                      ARM: 128...255 → 正常结束 */
```

**三条防御规则：**
1. **字节数据一律 `unsigned char` 或 `u8`**，不要用 `char`；
2. 编译器开 **`-Wchar-subscripts`**（GCC/Clang 都支持，专门抓"用 char 做下标"）；
3. 跨平台 CI：**至少在 x86_64 和 aarch64 上都跑一遍测试**。符号性差异不会在单平台上暴露。

> 补充：C 标准库的 `isalpha()/tolower()` 系列同样要求传 `unsigned char` 范围内的值或 EOF，
> 传负数是 UB。正确写法 `isalpha((unsigned char)c)`。

</details>

</details>
---
