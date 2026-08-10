# 5.1 比较指令

> 来源：§5.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

CMP 和 CMN 比较指令的本质和区别。

## 核心要点

| 指令 | 本质 | 用途 |
|------|------|------|
| CMP | SUBS XZR, Xn, Xm | 比较大小（减法） |
| CMN | ADDS XZR, Xn, Xm | 负数比较（加法） |

- CMP x0, x1 ≡ SUBS XZR, x0, x1（丢弃结果，设 NZCV）
- CMN x0, x1 ≡ ADDS XZR, x0, x1
- CMP 比较的是 x0 - x1；CMN 比较的是 x0 - (-x1)

## HFT 关联

比较是条件分支的基础：
- 价格比较 `cmp bid_price, ask_price; b.ge trade` 判断是否可套利
- 数量比较 `cmp order_qty, #0; b.eq skip` 过滤空订单
- CMN 较少使用，但在与负数比较时可以省去显式取负

## 自测题

1. `CMP x0, #5` 执行后 x0 = 3，NZCV 各是什么？
<details><summary>答案</summary>
3 - 5 = -2(0xFFFFFFFFFFFFFFFE)。N=1(负), Z=0(非零), C=0(有借位), V=0(无符号溢出)。B.LT(N≠V=1≠0)会跳转，3 < 5 成立。
</details>

2. CMN x0, x1 和 CMP x0, x1 的区别？
<details><summary>答案</summary>
CMP 做减法 x0-x1。CMN 做加法 x0+x1，等价于 CMP x0, (-x1)。CMN 用于和负数比较时避免显式取负：`CMN x0, #5` 等价于 `CMP x0, #-5`。
</details>

3. 为什么 CMP 要用 SUBS 而不是单独的比较指令？
<details><summary>答案</summary>
AArch64 没有"纯比较"指令。CMP 复用 SUBS 的硬件（结果写 XZR 丢弃），减少指令集复杂度。这是一种 RISC 设计哲学——用组合实现复杂操作。
</details>

## 参考与延伸

- 原书 §5.1
- [4.2 NZCV](../../chapter-04-a64-arithmetic-shift/notes/section-0-本章完整概述.md)
- [5.4 条件后缀](04-condition-suffix.md)
