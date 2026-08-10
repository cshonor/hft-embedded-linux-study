# §23.2 谓词（Predicate）⭐

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

谓词（Predicate）是 SVE 的核心创新——16 个谓词寄存器 P0-P15，每个通道一个 bit 控制是否执行，实现无 branch 的条件计算。

## 核心要点

### 谓词寄存器

```
谓词 P: | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0 |  (每通道 1 bit)
向量 A: | a0| a1| a2| a3| a4| a5| a6| a7|
ADD #1: |a0+1|a1|a2+1|a3+1|a4|a5+1|a6|a7|  ← 只有 pg=1 的通道被修改
                                      ↑ pg=0 的通道不变
```

### 谓词使用示例

```c
#include <arm_sve.h>

// SVE：用谓词控制哪些通道参与计算
// 只对大于 0 的元素加 1，其余保持不变
svint32_t conditional_add(svint32_t va) {
    // 全 true 谓词
    svbool_t all_true = svptrue_b32();
    // 比较：va > 0 → 生成谓词
    svbool_t pg = svcmpgt_n_s32(all_true, va, 0);
    // 只对 pg=true 的通道 +1（_m = merge，inactive 保留旧值）
    svint32_t result = svadd_n_s32_m(pg, va, 1);
    return result;
}
```

### 谓词生成指令

| 指令 | 行为 | C intrinsic | 用途 |
|------|------|-------------|------|
| `PTRUE` | 全 true | `svptrue_b32()` | 全通道激活 |
| `WHILELT` | i < n 前缀 | `svwhilelt_b32_s64(0, n)` | 循环控制 |
| `CMPGT` | 大于比较 | `svcmpgt_n_s32(pg, va, 0)` | 条件过滤 |
| `CMPEQ` | 等于比较 | `svcmpeq_n_u8(pg, va, 0)` | 查找匹配 |
| `CMPNE` | 不等比较 | `svcmpne_u8(pg, va, vb)` | 查找不同 |
| `BRK` | 谓词中断 | `svbrkb_z` | 找第一个 false |
| `PTEST` | 测试谓词 | `svptest_first` | 检查结果 |

### 谓词后缀

| 后缀 | 含义 | inactive 通道行为 | 适用场景 |
|------|------|------------------|---------|
| `_x` | unspecified | 未定义（最快） | 只关心 active 结果 |
| `_z` | zero | 清零 | 需要 inactive=0 |
| `_m` | merge | 保留目标旧值 | 需要保留旧值 |

```c
// 三种后缀对比
svbool_t pg = svcmpgt_n_s32(svptrue_b32(), va, 0);

// _x: inactive 未定义（可能垃圾值）
svint32_t r1 = svadd_n_s32_x(pg, va, 1);
// pg=1: r1[i] = va[i]+1, pg=0: r1[i] = ???（不保证）

// _z: inactive 清零
svint32_t r2 = svadd_n_s32_z(pg, va, 1);
// pg=1: r2[i] = va[i]+1, pg=0: r2[i] = 0

// _m: inactive 保留旧值
svint32_t r3 = svadd_n_s32_m(pg, va, 1);
// pg=1: r3[i] = va[i]+1, pg=0: r3[i] = va[i]（原值）
```

### 谓词 vs NEON 位掩码对比

```c
// NEON 模拟条件执行（位掩码）
float32x4_t va = vld1q_f32(data);
// 比较生成掩码（全1或全0）
uint32x4_t mask = vcgtq_f32(va, vdupq_n_f32(0.0f));
// 位选择：mask=1 取 va+1, mask=0 取 va
float32x4_t result = vbslq_f32(mask,
                                vaddq_f32(va, vdupq_n_f32(1.0f)),
                                va);
// 3 条指令：VCMP + VADD + VBSL

// SVE 谓词版本
svfloat32_t sv = svld1_f32(svptrue_b32(), data);
svbool_t pg = svcmpgt_n_f32(svptrue_b32(), sv, 0.0f);
svfloat32_t sr = svadd_n_f32_m(pg, sv, 1.0f);
// 2 条指令（谓词嵌入 ADD）：CMPGT + ADD
```

| 维度 | NEON 位掩码 | SVE 谓词 |
|------|-----------|---------|
| 指令数 | 3+（CMP+OP+BSL） | 1-2（谓词嵌入运算） |
| 条件分支 | 可能需要 branch | 无 branch |
| 分支预测 | 可能失败(~15周期) | 无失败风险 |
| 精确控制 | 全 lane 或位运算 | 每 lane 独立控制 |
| 学习曲线 | 低（标准 C 运算） | 中（新概念） |

## HFT 关联

谓词驱动的条件计算对 HFT 有吸引力：(1) 订单簿过滤——"只对价格 > VWAP 的订单做操作"可以用谓词一次过滤整个向量，无 branch；(2) 风控规则批量检查——同时对多个持仓检查风控条件，谓词标记违规项。

```c
// HFT 订单簿谓词过滤（未来 SVE 版本）
#include <arm_sve.h>

// 过滤价格 > vwap 的订单，返回匹配数量
int64_t hft_filter_orders_sve(
    const float *prices, int64_t n, float vwap,
    float *filtered) {

    svfloat32_t vwap_v = svdup_f32(vwap);
    int64_t i = 0, out_i = 0;

    while (i < n) {
        svbool_t pg = svwhilelt_b32_s64(i, n);
        svfloat32_t pv = svld1_f32(pg, prices + i);

        // 谓词过滤：价格 > vwap
        svbool_t match = svcmpgt_f32(pg, pv, vwap_v);

        // 压缩存储：只存 match=true 的
        // svcompact 将 active 通道连续排列
        svfloat32_t filtered_v = svcompact_f32(match, pv);

        // 存储匹配的元素
        int64_t match_cnt = svcntp_b32(match, svcntw());
        svst1_f32(svptrue_b32(), filtered + out_i, filtered_v);
        out_i += match_cnt;

        i += svcntw();
    }
    return out_i;
}

// HFT 风控批量检查
void hft_risk_check_sve(const float *positions,
                          int64_t n, float limit) {
    svfloat32_t lim = svdup_f32(limit);
    int64_t i = 0;

    while (i < n) {
        svbool_t pg = svwhilelt_b32_s64(i, n);
        svfloat32_t pos = svld1_f32(pg, positions + i);
        // 检查 |position| > limit
        svbool_t violation = svcmpgt_f32(pg,
            svabs_f32_x(pg, pos), lim);
        if (svptest_first(svptrue_b32(), violation)) {
            // 有违规！发出警告
            raise_alert();
        }
        i += svcntw();
    }
}
```

## 自测题

1. **谓词寄存器如何实现无 branch 的条件计算？和 NEON 的位掩码方法相比有什么优势？**

<details>
<summary>答案</summary>

谓词寄存器每个通道 1 bit，直接控制该通道是否执行运算。`ADD Z0.S, P0/M, Z0.S, #1` 只对 P0=1 的通道加 1，P0=0 的通道不变。与 NEON 位掩码方法相比的优势：(1) **无需额外指令**——谓词直接嵌入运算指令，不需要 AND/OR/MASK 操作；(2) **无 branch**——不需要 CMP+BEQ 跳转，避免分支预测失败；(3) **精确控制**——位掩码需要先计算掩码再应用（3+ 条指令），谓词一条指令完成。NEON 模拟条件执行需要 `VCMP → VBSL` 或 branch，效率低 2-3 倍。
</details>

2. **`_x`、`_z`、`_m` 三种谓词后缀有什么区别？**

<details>
<summary>答案</summary>

控制 inactive 通道（谓词=0 的通道）的行为：`_x`（unspecified）—— inactive 通道的值**未定义**（可能保留旧值也可能改变，取决于实现），速度最快但不安全；`_z`（zero）—— inactive 通道**清零**，适合需要"不满足条件的设为 0"的场景；`_m`（merge）—— inactive 通道**保留目标寄存器的旧值**，最安全但可能稍慢（需要读旧值）。选择规则：如果只关心 active 通道的值用 `_x`；需要 inactive 为 0 用 `_z`；需要保留旧值用 `_m`。
</details>

3. **`svwhilelt_b32_u64(0, n)` 生成什么样的谓词？**

<details>
<summary>答案</summary>

生成一个**前缀为 true 的谓词**：第 0 到 n-1 通道为 true，第 n 及之后为 false。例如 n=5、向量长度=8：`P = |1|1|1|1|1|0|0|0|`。这用于处理**尾部不足一个向量**的情况：当剩余元素 n 小于向量通道数时，只有前 n 个通道有效。这是 SVE "向量长度无关编程"的关键机制——循环中每次迭代用 `svwhilelt` 重新计算有效通道，不需要特殊处理尾部。
</details>

## 参考与延伸

- [§23.1 SVE 核心特性](01-sve-features.md) — SVE 整体架构
- [§23.4 strcmp 优化](04-strcmp-optimization.md) — 谓词在字符串处理中的应用
