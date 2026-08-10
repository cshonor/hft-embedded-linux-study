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
// 分支预测失败 ~15 周期/次
```

### SVE strcmp（简化概念）

```c
#include <arm_sve.h>

int strcmp_sve(const char *a, const char *b) {
    svbool_t pg = svptrue_b8();  // 全 true 谓词

    while (1) {
        // 一次加载一个向量宽度的字节（16-256 字节）
        svuint8_t va = svld1_u8(pg, (const uint8_t *)a);
        svuint8_t vb = svld1_u8(pg, (const uint8_t *)b);

        // 批量比较相等
        svbool_t eq = svcmpeq_u8(pg, va, vb);

        // 检测 '\0' 位置（字符串结束）
        svbool_t za = svcmpeq_n_u8(pg, va, 0);

        // 检查是否在 '\0' 之前全部相等
        svbool_t diff_before_null = svcmpne_u8(pg, va, vb);
        // 取 '\0' 之前的部分
        // ...

        if (!svptest_first(svptrue_b8(), za)) {
            // 有 '\0' → 字符串结束
            if (svptest_first(svptrue_b8(), eq)) {
                return 0;  // 全相等（到 '\0' 为止）
            }
        }

        if (!svptest_first(za, eq)) {
            // 在 '\0' 之前有不等的字节
            // 找到第一个不等位置，返回差值
            // ...提取第一个不等 lane 的值
        }

        a += svcntb();
        b += svcntb();
    }
}
```

### strcmp 流程图

```
标量版本:
┌──────────────────────────────────────────┐
│ while (*a && *a == *b):                   │
│   a++; b++;                              │  ← 每字节一个分支
│ return *a - *b;                          │
└──────────────────────────────────────────┘
  100 字节 → ~100 次循环 → ~400 条指令

SVE 版本 (VQ=1, 128bit):
┌──────────────────────────────────────────┐
│ 一次加载 16 字节 → 16 路并行比较          │
│ 谓词检测 '\0' 和不等位置                  │
│ 全相等 → 下一批 16 字节                   │
│ 有不等 → 返回差值                         │
└──────────────────────────────────────────┘
  100 字节 → 7 次迭代 → ~30 条指令
```

### 性能对比

| 方面 | 标量 | NEON | SVE |
|------|------|------|-----|
| 每次迭代 | 1 字节 | 16 字节 | svcntb() 字节 |
| 100 字节串 | ~400 条指令 | ~40 条 | ~20-30 条 |
| 分支 | 每 byte 1 个 | 每 16 byte 1 个 | 每 16-256 byte 1 个 |
| '\0' 检测 | 逐字节 | 需额外指令 | 谓词一条指令 |
| 尾部处理 | 内置 | 手动 | 自动（谓词） |
| 加速比 | 1× | ~10× | 10-20× |

### NEON strcmp 对比

```c
// NEON 版本 strcmp（简化）
#include <arm_neon.h>
int strcmp_neon(const char *a, const char *b) {
    while (1) {
        uint8x16_t va = vld1q_u8((const uint8_t *)a);
        uint8x16_t vb = vld1q_u8((const uint8_t *)b);

        // 比较
        uint8x16_t cmp = vceqq_u8(va, vb);
        // 检测 '\0'
        uint8x16_t zero = vdupq_n_u8(0);
        uint8x16_t has_null = vceqq_u8(va, zero);

        // 提取结果到标量
        uint64_t cmp_bits = vget_lane_u64(
            vreinterpret_u64_u8(vshrn_n_u16(
                vreinterpretq_u16_u8(cmp), 4)), 0);
        uint64_t null_bits = vget_lane_u64(
            vreinterpret_u64_u8(vshrn_n_u16(
                vreinterpretq_u16_u8(has_null), 4)), 0);

        // 复杂的位操作找到第一个不等或 '\0'
        // ...（比 SVE 谓词方法复杂得多）

        a += 16;
        b += 16;
    }
}
// NEON 需要 5+ 条指令提取比较结果到标量
// SVE 用 svptest_first 一条指令完成
```

### SVE strcmp 的关键优势

| 技术点 | NEON 做法 | SVE 做法 | 优势 |
|--------|----------|---------|------|
| 批量比较 | VCEQ + 位提取 | svcmpeq + 谓词 | 少 3+ 条指令 |
| '\0' 检测 | VCEQ(vs,0) + 位提取 | svcmpeq_n + 谓词 | 同上 |
| 第一个不同 | CLZ/CTZ 位操作 | svptest_first | 直接返回 bool |
| 尾部处理 | 手动标量 | svwhilelt 自动 | 无额外代码 |
| 向量长度 | 固定 16 字节 | 可变 16-256 字节 | 未来自动加速 |

## HFT 关联

strcmp 优化在 HFT 中的间接价值：(1) 交易网关中 symbol 匹配（如 "AAPL" vs "GOOGL"）可用 SVE 向量化；(2) 协议解析中的字符串字段比较（如 FIX 协议的 Tag=Value）；(3) 日志系统的关键字过滤。

```c
// HFT symbol 匹配（SVE 未来版本）
#include <arm_sve.h>

// 查找匹配的 symbol，返回索引
int64_t hft_find_symbol_sve(
    const char *symbols, int64_t count,
    const char *target) {
    // symbols 是 count 个定长字符串（如 8 字节）
    // target 是要查找的字符串

    int64_t i = 0;
    svbool_t pg = svwhilelt_b8_s64(i, count * 8);

    while (svptest_first(svptrue_b8(), pg)) {
        // 加载目标字符串
        svuint8_t tv = svld1_u8(svptrue_b8(),
                                 (const uint8_t *)target);

        // 加载一批 symbol
        // (简化：实际需要 gather 加载非连续地址)

        // 比较并生成谓词
        // svbool_t match = svcmpeq_u8(pg, sv, tv);
        // if (svptest_first(svptrue_b8(), match))
        //     return i / 8;  // 找到匹配

        i += svcntb();
        pg = svwhilelt_b8_s64(i, count * 8);
    }
    return -1;  // 未找到
}

// 注意：HFT 实际中通常用哈希表做 symbol 查找（O(1)）
// SVE strcmp 的价值在于理解谓词向量化思路
// 在协议解析、日志过滤等非关键路径可用
```

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
