# 4.2 NZCV 四个条件标志

> 来源：§4.2 · 精读 · [章总览](section-0-本章完整概述.md) · [NZCV 专篇](../../NZCV.md)

## 本节讲什么

NZCV 四个条件标志的含义，以及无符号/有符号比较的区别。

## 核心要点

| 标志 | 含义 | 设置条件 |
|------|------|----------|
| N | Negative | 结果最高位为 1 |
| Z | Zero | 结果全 0 |
| C | Carry | 无符号进位/借位**取反** |
| V | oVerflow | 有符号溢出 |

关键区分：
- **无符号比较**看 C（Carry）：C=1 表示 x0 ≥ x1
- **有符号比较**看 N⊕V：N==V 表示 x0 ≥ x1

条件后缀：
- EQ(Z=1)、NE(Z=0)
- LT(有符号小于)、GE(有符号大于等于)
- LO(无符号小于)、HS(无符号大于等于)

## HFT 关联

NZCV 是条件执行的基础：
- 比较价格 `cmp x0, x1; b.gt buy` 判断是否触发交易
- 无符号比较用于地址/大小比较（地址永远 ≥ 0）
- 有符号比较用于价格/数量比较（可为负）
- 条件后缀选择错误（LT vs LO）会导致隐蔽的逻辑 bug

## 自测题

1. 有符号 -1 和无符号 0 比较，CMP 设置的标志是什么？用 `B.LT` 和 `B.LO` 分别会跳转吗？
<details><summary>答案</summary>
-1 (0xFFFFFFFF) - 0 = 0xFFFFFFFF。N=1, Z=0, C=1(无借位), V=0(无溢出)。
B.LT (N≠V) → 1≠0 → 跳转（-1 < 0 成立）。
B.LO (C=0) → C=1 → 不跳转（无符号看 -1 是大数 0xFFFFFFFF，不小于 0）。
</details>

2. C 标志在减法中为什么是"借位取反"？
<details><summary>答案</summary>
ARM 的 SUBS 中 C = NOT(Borrow)。即无借位时 C=1，有借位时 C=0。这样 HS(C=1) = 大于等于，LO(C=0) = 小于，语义一致。
</details>

3. 如何同时判断"结果为零且无溢出"？
<details><summary>答案</summary>
检查 Z=1 AND V=0。没有单一条件后缀组合这两个。可以用两个条件分支：`B.NE skip; B.VS skip`（都不跳则满足条件）。或者用 MRS 读 NZCV 再位测试。
</details>

## 参考与延伸

- 原书 §4.2
- [NZCV 专篇](../../NZCV.md)
- [5.4 条件后缀速查](../../chapter-05-a64-compare-branch/notes/section-0-本章完整概述.md)
