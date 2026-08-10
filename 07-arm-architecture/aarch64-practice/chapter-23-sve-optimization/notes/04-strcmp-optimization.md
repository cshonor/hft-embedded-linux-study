# §23.4 实用示例：strcmp 优化

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

用 SVE 优化 strcmp——一次比较整条向量（16-256 字节），利用谓词检测 '\0' 和不等位置，避免逐字节循环。

## 核心要点

### 标量 strcmp 的问题

```c
// 标量版本：逐字节比较
int strcmp_c(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return *a - *b;
}
// 每个 byte：2 次加载 + 1 次比较 + 1 次分支 = ~4 条指令
// 100 字节字符串 = ~400 条指令
```

### SVE strcmp（简化概念）

```c
svbool_t pg = svptrue_b8();
while (1) {
    svuint8_t va = svld1_u8(pg, ptr_a);     // 加载 a 的一个向量
    svuint8_t vb = svld1_u8(pg, ptr_b);     // 加载 b 的一个向量
    svbool_t eq = svcmpeq_u8(pg, va, vb);    // 比较相等
    svbool_t za = svcmpeq_n_u8(pg, va, 0);   // a 是否有 '\0'
    if (!svptest_first(pg, za)) {
        // 有 '\0' 且全部相等 → 返回 0
        return 0;
    }
    if (!svptest_first(za, eq)) {
        // 不等 → 返回差值
        // ...
    }
    ptr_a += svcntb();
    ptr_b += svcntb();
}
```

### 性能对比

| 方面 | 标量 | SVE |
|------|------|-----|
| 每次迭代 | 1 字节 | svcntb() 字节（16-256） |
| 100 字节串 | ~400 条指令 | ~20-30 条指令 |
| 分支 | 每 byte 1 个 branch | 每 16-256 byte 1 个 branch |
| 加速比 | 1× | 10-20× |

> SVE 一次比较整条向量，不用逐字节循环。谓词自动检测 '\0' 和不等位置。

## HFT 关联

strcmp 优化在 HFT 中的间接价值：(1) 交易网关中 symbol 匹配（如 "AAPL" vs "GOOGL"）可用 SVE 向量化；(2) 协议解析中的字符串字段比较（如 FIX 协议的 Tag=Value）；(3) 日志系统的关键字过滤。但这些场景通常用哈希表/字典代替 strcmp，SVE strcmp 的价值更多在于理解谓词驱动的向量化思路——将逐元素操作提升为批量操作。

## 自测题

1. **SVE strcmp 相比标量 strcmp 的核心加速点是什么？**

<details>
<summary>答案</summary>

核心加速点：(1) **批量加载**——一次加载 16-256 字节（一个向量宽度），而不是逐字节；(2) **批量比较**——`svcmpeq_u8` 一次比较整个向量，生成谓词标记不等位置；(3) **谓词检测 '\0'**——`svcmpeq_n_u8(pg, va, 0)` 一次检测整个向量中是否有 '\0'，不需要逐字节检查；(4) **减少分支**——标量每字节一个 branch（可能预测失败），SVE 每 16-256 字节一个 branch。总加速 10-20 倍。
</details>

2. **`svptest_first` 的作用是什么？在 strcmp 中如何使用？**

<details>
<summary>答案</summary>

`svptest_first(pg1, pg2)` 检查 pg2 中第一个 active 通道（pg1=1 的通道中第一个）是否为 true。在 strcmp 中有两个用途：(1) `svptest_first(pg, za)` 检查 za（a 有 '\0' 的谓词）中第一个 active 通道是否为 true——如果是，说明 a 在当前向量中有 '\0'（字符串结束）；(2) `svptest_first(za, eq)` 检查在 '\0' 之前是否全部相等——如果不是，说明有不等字节。`svptest_first` 返回 bool，可以用于 if 判断，避免展开谓词到标量。
</details>

3. **SVE strcmp 对短字符串（如 4 字节的 "AAPL"）有优势吗？**

<details>
<summary>答案</summary>

**对极短字符串优势不明显**。SVE strcmp 每次加载一个向量（至少 16 字节），对 4 字节字符串会加载多余的 12 字节——虽然不错误（谓词会限制有效范围），但加载/比较/谓词操作的开销可能比标量的 4 次逐字节比较还高。SVE strcmp 的优势在**长字符串**（>16 字节）：一次比较 16+ 字节，加速比随长度增长。对 HFT 中的短 symbol 匹配（3-5 字节），标量或 NEON（4 字节一条指令）可能更快。实际 glibc 的 strcmp 实现会先处理头部几个字节，再切换到向量模式。
</details>

## 参考与延伸

- [§23.2 谓词](02-predicate.md) — 谓词寄存器和条件执行
- [§23.3 SVE vs NEON](03-sve-vs-neon.md) — SVE 和 NEON 的适用场景
