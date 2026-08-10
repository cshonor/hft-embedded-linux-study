# 4.7 易错坑

> 来源：§4.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 4 大易错点

1. **有符号 vs 无符号比较**用错条件后缀（LT vs LO）
2. **ASR vs LSR** 选择错误（有符号用 ASR，无符号用 LSR）
3. **SUBS 的 C 标志是借位取反**（C=1 表示无借位/大于等于）
4. **UBFX vs SBFX** 选错（有符号字段用 SBFX）

## 自测题

1. 比较两个地址 x0 和 x0 应该用 B.LT 还是 B.LO？
<details><summary>答案</summary>
B.LO（无符号小于）。地址是无符号数，不能用有符号比较 LT。如果地址高位为 1（如内核地址 0xFFFF...），有符号比较会把它们当成负数，结果错误。
</details>

2. `-8 >> 1` 用 LSR 结果是什么？应该用什么？
<details><summary>答案</summary>
LSR 逻辑右移补 0 → -8(0xFFFFFFFFFFFFFFF8) >> 1 = 0x7FFFFFFFFFFFFFFC（大正数，错误）。应该用 ASR 算术右移补符号位 → 0xFFFFFFFFFFFFFFFC（-4，正确）。
</details>

3. SUBS x0, x1, x2 后 x1 < x2（无符号），C 标志是什么？
<detail><summary>答案</summary>
C = NOT(Borrow) = NOT(1) = 0。因为有借位（x1 < x2）。所以 B.LO(C=0) 会跳转，B.HS(C=1) 不跳转。
</details>

## 参考与延伸

- 原书 §4.7
- [4.2 NZCV](02-nzcv.md)
- [NZCV 专篇](../../NZCV.md)
