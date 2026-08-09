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

| 模型 | 架构 | `int` | `long` | 指针 |
|------|------|-------|--------|------|
| **ILP32** | 32 位 | 32 | 32 | 32 |
| **LP64** | 64 位 | 32 | 64 | 64 |

| 禁止假设 | |
|----------|--|
| ❌ `int` == `long` | |
| ❌ 指针 == `int` | |

```c
/* 错：在 LP64 上截断指针 */
int x = (int)ptr;

/* 对：内核风格 */
unsigned long x = (unsigned long)ptr;
```

→ [02-CSAPP 数据表示](../../../../02-computer-systems/)



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

</details>
---
