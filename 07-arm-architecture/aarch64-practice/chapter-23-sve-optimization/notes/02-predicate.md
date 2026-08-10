# §23.2 谓词（Predicate）⭐

> **来源：** [Ch23 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

谓词（Predicate）是 SVE 的核心创新——16 个谓词寄存器 P0-P15，每个通道一个 bit 控制是否执行，实现无 branch 的条件计算。

## 核心要点

### 谓词寄存器

```
谓词 P: | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0 |
向量 A: | a0| a1| a2| a3| a4| a5| a6| a7|
ADD  : |a0+1| |a2+1|a3+1| |a5+1| | |  ← 只有 pg=1 的通道被修改
结果:  | a0'|a1| a2'|a3'|a4| a5'|a6|a7|
```

### 谓词使用示例

```c
// SVE：用谓词控制哪些通道参与计算
svbool_t pg = svcmpgt_n_s32(svptrue_b32(), va, 0);  // pg = (va > 0)
svint32_t result = svadd_n_s32_x(pg, va, 1);        // 只对 pg=true 的通道 +1
```

### 谓词生成指令

| 指令 | 行为 | C intrinsic |
|------|------|-------------|
| `PTRUE` | 全 true | `svptrue_b32()` |
| `WHILELT` | i < n 的前缀 | `svwhilelt_b32_u64(0, n)` |
| `CMPGT` | 大于比较 | `svcmpgt_n_s32(pg, va, 0)` |

### 谓词后缀

| 后缀 | 含义 | 行为 |
|------|------|------|
| `_x` | 不影响 inactive 通道 | inactive 通道保持原值 |
| `_z` | inactive 清零 | inactive 通道 = 0 |
| `_m` | inactive 保留旧值 | inactive 通道 = 原目标值 |

> NEON 没有谓词，需要用 branch 或位掩码模拟条件执行，效率低且可能产生分支预测失败。

## HFT 关联

谓词驱动的条件计算对 HFT 有吸引力：(1) 订单簿过滤——"只对价格 > VWAP 的订单做操作"可以用谓词一次过滤整个向量，无 branch；(2) 风控规则批量检查——同时对多个持仓检查风控条件，谓词标记违规项。branch-free 代码避免了分支预测失败的流水线冲刷（~15 周期），对延迟敏感场景有价值。但 SVE 目前只在少数 ARM 服务器芯片上可用，HFT 实际部署仍以 NEON 为主。

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
