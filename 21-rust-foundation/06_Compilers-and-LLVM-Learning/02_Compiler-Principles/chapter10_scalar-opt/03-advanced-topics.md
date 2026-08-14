# 第 10 章 · 标量优化 · §3 高级话题

← [本章目录](./README.md) · 上一节：[02-core-transformations.md](./02-core-transformations.md) · 下一节：---

## 强度减弱（Strength Reduction）

在循环（尤其）里，用**更便宜**的运算替代**昂贵**运算，保持语义等价。

| 典型替换 | 昂贵 → 便宜 |
|----------|-------------|
| 归纳变量 | `i * stride`（乘法）→ 累加 `j += stride` |
| 地址计算 | `base + i*4` → `ptr += 4` 迭代 |
| 幂次 | `x * 2^k` → 移位 |

```text
// 概念
for (i = 0; i < n; i++)
  a[i * 4] = …
// 减弱后
p = &a[0];
for (i = 0; i < n; i++, p++)
  *p = …
```

常与 **LICM**、**归纳变量消除（IV elimination）** 联用。

**机器相关面**：目标是否有 `lea`、乘法的延迟 — 后端可能再做一轮。

---

## 优化组合与交互（Combined Optimizations）

单个 Pass 效果有限；**组合**才接近 `-O2`/`-O3` 质量。

| 交互 | 说明 |
|------|------|
| **正向协同** | 常量传播 → DCE → 更短 LIVE → 更好 regalloc |
| **互相破坏** | 内联膨胀 IR → 分析变慢；某 Pass 引入临时变量 → 掩盖 CSE |
| **不动点迭代** | 同一 Pass 包多轮直到收敛（如 LLVM `instcombine` 循环） |

```text
  CP ──→ DCE ──→ GVN ──→ LICM ──→ SR ──→ （再 CP…）
         ↑______________________________|
              多轮直到无明显变化
```

**1+1>2**：例子 — 传播常量后分支变常 → 删不可达 → 更多表达式变「可用」。

---

## 优化序列的选择（Choosing a Sequence）

构建现代优化器的**最大挑战之一**：Pass **顺序**与**是否重复**直接决定代码质量。

| 难题 | 原因 |
|------|------|
| **前提被破坏** | GVN 前若未建 SSA，效果差 |
| **相位 ordering** | 先 DCE 还是先 inline — 无 universal 最优 |
| **编译时间** | 多轮迭代 vs 单次流水线 |
| **调试可复现** | Pass 顺序写死在 `PassManager` |

**典型中端顺序（概念，非唯一）**：

```text
inline → simplifycfg → mem2reg(SSA) → instcombine → GVN → LICM → DCE → …
```

**LLVM / rustc**：顺序随版本演进；读 `opt -passes=print-pipeline` 或源码 `PassBuilder`。

**工程原则**：

1. **分析在前、变换在后**（或 interleaved 但需收敛论证）
2. **cheap cleanup Pass**（DCE、simplifycfg）穿插防 IR 膨胀
3. **benchmark 驱动**调序 — 无纯理论闭式解

---

## Part III 收束

| 章 | 角色 |
|----|------|
| ch8 | 优化问题与作用域 |
| ch9 | 数据流 + SSA 基础设施 |
| ch10 | 标量 Pass 兵器谱 + Pass 编排 |

→ **ch11 指令筛选**：从改进后的 IR **选机器指令** — 进入 Part IV 代码生成。

---

## HFT 联想

- 热路径 **`#[inline]` + LTO** → ch10 过程间 + 标量组合在链接期全程序生效。
- **`-C opt-level=3`** 即更长 Pass 序列 + 更多轮次 — 编译慢、运行时快。
