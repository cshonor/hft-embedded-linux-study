## ② 字长和数据类型 · Word Size

| 概念 | 说明 |
|------|------|
| **字长（word）** | CPU **一次处理** 的数据宽度 |
| Linux | 支持 **32 位** 与 **64 位** 架构 |

#### Linux 上的 C 类型习惯

| 类型 | 规则 |
|------|------|
| **`long`、指针** | 大小 **= 机器字长** |
| **`int`** | 目前所有支持架构上均为 **32 位** |

#### 数据模型

C 语言只保证 `sizeof(char) ≤ sizeof(short) ≤ sizeof(int) ≤ sizeof(long) ≤ sizeof(long long)`，
**具体多宽交给实现**。于是历史上出现了几种"约定"：

| 模型 | 代表平台 | `int` | `long` | 指针 | `long long` |
|------|---------|-------|--------|------|-------------|
| **ILP32** | 32 位 x86、ARM32 | 32 | 32 | 32 | 64 |
| **LP64** | **x86_64、ARM64（Linux/macOS）** | 32 | **64** | 64 | 64 |
| **LLP64** | **Windows x64（MSVC）** | 32 | **32** | 64 | 64 |
| **SILP64** | 早期 Cray 等 | 64 | 64 | 64 | 64 |

> **LP64 vs LLP64 是跨 Linux / Windows 移植最经典的坑**：
> 同一句 `long x = ptr;` 在 Linux 64 位上安全，在 Windows 64 位上**截断**（long 只有 32 位）。
> 这解释了为什么跨平台代码一律用 `uintptr_t` 或 `intptr_t`——**它们是唯一保证"能装下指针的整数"**。

**为什么 `long` 要跟着字长变（LP64 而不是 LLP64）？**

| 理由 | 说明 |
|------|------|
| 历史 | 早期 `long` 就是"机器字长"的同义词（PDP-11 上 `long` = 32 位 = 字长） |
| 性能 | 让"最常用的整数类型"与寄存器同宽，避免零扩展/符号扩展指令 |
| 简化 | `sizeof(long) == sizeof(void *)` 让"用整数装指针"这个常见技巧变成可能 |

> 这个决定在 1990 年代是合理的，今天则被视为历史包袱——
> 它导致 Linux 上 `long` 的宽度**依赖架构**，所以内核才要强制用 u32/u64。

#### 内核的探测宏

```c
BITS_PER_LONG       /* 32 或 64 —— 编译期常量 */
__SIZEOF_POINTER__  /* 编译器内置的指针大小 */
sizeof(long) == sizeof(void *)   /* LP64 平台上的不变量 */
```

| 禁止假设 | |
|----------|--|
| ❌ `int` == `long` | |
| ❌ 指针 == `int` | |

```c
/* 错：在 LP64 上截断指针 */
int x = (int)ptr;

/* 对：内核风格 */
unsigned long x = (unsigned long)ptr;

/* 更对：明确"我就是要装指针" */
uintptr_t x = (uintptr_t)ptr;
```

#### 陷阱清单（按踩坑频率排序）

| # | 陷阱 | 后果 | 正确做法 |
|---|------|------|---------|
| 1 | **指针装进 int** | LP64 上高 32 位丢失 | `uintptr_t` |
| 2 | **打印格式不匹配** | `printf("%d", sizeof(x))` 是 UB | 用 `%zu`（size_t）、`%p`（指针）、`PRIu64`（u64） |
| 3 | **移位溢出** | `1 << 40` 在 `int` 上是 UB | `1UL << 40` 或 `BIT_ULL(40)` |
| 4 | **结构体成员顺序依赖** | 不同模型下 `sizeof` 不同 → 线上格式不兼容 | 固定宽度类型 + 显式序列化 |
| 5 | **有符号/无符号比较** | `-1 < sizeof(x)` 恒为 false | 统一符号性 |
| 6 | **位掩码常量** | `0xFFFFFFFF` 是 `unsigned int`，与 u64 运算要小心 | `U64_MAX` / `GENMASK_ULL` |

> 内核里这些坑有专门的检查：构建时开 **`make C=1`（sparse）** 会警告 `__bitwise` 类型混用、
> 用户指针未标注 `__user` 等问题；`W=1` 会打开额外编译告警。

#### 内核类型 ↔ 用户态类型对照

| 内核 | 用户态（`<stdint.h>`） | 位宽 |
|------|----------------------|------|
| `u8` / `s8` | `uint8_t` / `int8_t` | 8 |
| `u16` / `s16` | `uint16_t` / `int16_t` | 16 |
| `u32` / `s32` | `uint32_t` / `int32_t` | 32 |
| `u64` / `s64` | `uint64_t` / `int64_t` | 64 |
| `size_t` | `size_t` | 字长 |
| `uintptr_t` | `uintptr_t` | 字长 |

> **HFT 纪律：** 订单号、价格、时间戳、序号这些**有明确位宽语义**的字段，
> 一律用 `uint64_t`/`int64_t`，**永远不要**用 `unsigned long`。
> 前者在所有平台上都是 64 位，后者在 32 位平台（或 Windows）上会悄悄变窄——
> 而这类 bug 只在极端值下暴露，上线后极难查。

→ [02-CSAPP 数据表示](../../../02-computer-systems/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 为什么内核不直接用 int/long 而要用 u32/u64？

<details><summary>答案</summary>

int/long 大小依赖架构：x86 int=32bit, long=32bit；x86_64 int=32bit, long=64bit；ARM64 int=32bit, long=64bit。代码 `long x = 1<<32` 在 32 位溢出在 64 位正常。内核用 u8/u16/u32/u64 明确指定位数，保证跨架构一致。HFT 代码同样应该用 stdint.h 的 uint32_t/uint64_t。

</details>

**Q2.** size_t 和 ssize_t 的区别？为什么 syscall 返回 ssize_t？

<details><summary>答案</summary>

size_t = 无符号大小（unsigned），ssize_t = 有符号大小（signed）。syscall 如 read() 返回 ssize_t：正数=读取字节数，0=EOF，-1=错误。如果返回 size_t，-1 会被解释为 0xFFFFFFFFFFFFFFFF（巨大正数），无法区分错误。HFT 代码中处理 IO 返回值必须检查 < 0 而非 == 0xFFFFFFFF。

</details>

**Q3.** 64 位 Linux 上 `long` 是 64 位，Windows 上是 32 位——这段代码跨平台有什么问题？怎么改？

<details><summary>答案</summary>

```c
/* 有问题的写法 */
unsigned long id = (unsigned long)ptr;          /* Windows x64 上截断！ */
printf("size = %d\n", sizeof(buf));             /* %d 配 size_t 是 UB  */
uint64_t mask = 1 << 40;                        /* 1 是 int，移位溢出 = UB */
```

逐个说：

**① `unsigned long` 装指针**
```c
uintptr_t id = (uintptr_t)ptr;    /* C99 起保证"能无损装下指针的整数" */
```
`uintptr_t` 的存在就是为了解决这个跨模型问题——它在 LP64 上是 64 位，在 ILP32 上是 32 位，**永远刚好**。
`unsigned long` 只在 LP64 上碰巧够用。

**② 打印格式**
| 类型 | 正确格式 | 错误 |
|------|---------|------|
| `size_t` | `%zu` | `%d` / `%lu` |
| 指针 | `%p`（且要转 `void *`） | `%x` / `%lu` |
| `uint64_t` | `PRIu64`（`<inttypes.h>`） | `%llu`（平台相关） |
| `ptrdiff_t` | `%td` | `%d` |

格式串不匹配是**未定义行为**，不是"打印出来不好看"——编译器优化后可能打印出完全错误的值。

**③ 移位**
```c
uint64_t mask = 1UL << 40;      /* 但 1UL 在 Windows 上仍是 32 位！ */
uint64_t mask = UINT64_C(1) << 40;   /* 最稳：显式 64 位常量 */
uint64_t mask = (uint64_t)1 << 40;   /* 等效写法 */
```
`1` 的类型是 `int`（32 位）。左移 40 位 = **未定义行为**，在 x86 上表现为
"移位量对 32 取模" → 实际移了 8 位，得到 `0x100` 而不是 `0x10000000000`。
这类 bug 静默发生，是位掩码类错误的经典来源。

**④ HFT 具体建议**
- 订单 ID、成交号、序号：**`uint64_t`** 显式固定；
- 价格：定点化后用 `int64_t`（不要用 `long`，也不要用 double）；
- 时间戳：**`uint64_t`** 纳秒（或 `int64_t`）；
- 哈希、位图：`uint64_t` + `UINT64_C()` 常量；
- 构建时开 `-Wall -Wextra -Wconversion`，并**在 CI 里同时跑 x86_64 和 ARM64**（或至少跑一次 32 位交叉编译），
  让模型差异在编译期就暴露，而不是在生产环境靠溢出暴露。

</details>

</details>
---
