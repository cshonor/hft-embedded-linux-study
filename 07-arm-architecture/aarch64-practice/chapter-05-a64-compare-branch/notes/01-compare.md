# 5.1 比较指令 CMP / CMN

> 来源：§5.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

CMP 和 CMN 的本质、NZCV 如何被设置、与 SUBS/ADDS 的等价关系。

## 指令本质

| 指令 | 等价 | 运算 | 用途 |
|------|------|------|------|
| `CMP Xn, Xm` | `SUBS XZR, Xn, Xm` | Xn - Xm | 比较大小 |
| `CMP Xn, #imm` | `SUBS XZR, Xn, #imm` | Xn - imm | 与立即数比较 |
| `CMN Xn, Xm` | `ADDS XZR, Xn, Xm` | Xn + Xm | 负数比较 |
| `CMN Xn, #imm` | `ADDS XZR, Xn, #imm` | Xn + imm | 与负数比较 |

CMP/CMN 把结果写入 XZR（丢弃），但 NZCV 照常更新。源操作数不变。

## CMP 设置的 NZCV

| 操作 | N | Z | C | V | 成立的条件 |
|------|---|---|---|---|-----------|
| 5 - 3 = 2 | 0 | 0 | 1 | 0 | B.GT（5 > 3）|
| 3 - 5 = -2 | 1 | 0 | 0 | 0 | B.LT（3 < 5）|
| 5 - 5 = 0 | 0 | 1 | 1 | 0 | B.EQ（相等）|

## CMN — 负数比较的捷径

`CMN x0, #5` = `CMP x0, #-5`（做加法 x0+5，省去取负）。

```asm
; 判断 x0 == -1
CMN x0, #1        ; x0 + 1，若 x0=-1 则结果=0 → Z=1
B.EQ is_neg_one
```

## 立即数范围

CMP 的立即数：0~4095，或 4096 的倍数（imm<<12）。超出范围需用寄存器。

```asm
CMP x0, #100        ; OK
CMP x0, #4095       ; OK
; CMP x0, #5000     ; 报错，超出范围
MOV x1, #5000
CMP x0, x1          ; 改用寄存器
```

## 与 C 的对照

| C 代码 | AArch64 |
|--------|---------|
| `if (a > b)` | `cmp x0, x1; b.gt label` |
| `if (a == b)` | `cmp x0, x1; b.eq label` |
| `if (a == -5)` | `cmn x0, #5; b.eq label` |
| `if (a < 0)` | `cmp x0, #0; b.mi label` |

## 常见错误

1. **有符号/无符号混用**：CMP 对两种都有效，但 B.cond 要选对——B.LT（有符号）vs B.LO（无符号）。
2. **以为 CMP 改寄存器**：结果丢弃到 XZR，源操作数不变。需要结果用 SUBS。
3. **CMN 方向搞反**：`CMN x0, x1` 是 `x0 + x1`，不是 `x0 - x1`。

## HFT 关联

CMP 不修改寄存器，可以连续比较同一个值与多个阈值：

```asm
cmp x0, min_price
b.lo reject            ; 低于下限
cmp x0, max_price
b.hi reject            ; 高于上限
; x0 未被修改，继续使用
```

## 自测题

1. `CMP x0, #5`（x0=3）后 NZCV 各是什么？
<details><summary>答案</summary>
3-5=-2。N=1, Z=0, C=0（有借位）, V=0。B.LT 跳转。
</details>

2. CMN x0, x1 和 CMP x0, x1 的区别？
<details><summary>答案</summary>
CMP 做减法 x0-x1。CMN 做加法 x0+x1，等价于 CMP x0, (-x1)。
</details>

3. 写出判断 `x0 == -1` 的两种方式。
<details><summary>答案</summary>
```asm
; 方法1：CMP
MOV x1, #-1
CMP x0, x1
B.EQ label

; 方法2：CMN（更简洁）
CMN x0, #1        ; x0+1=0 则 x0=-1
B.EQ label
```
</details>

4. 为什么 CMP 用 SUBS 而不是单独的比较指令？
<details><summary>答案</summary>
AArch64 没有"纯比较"指令。CMP 复用 SUBS 硬件（结果写 XZR 丢弃），RISC 设计哲学——用组合实现，减少指令集复杂度。
</details>

## 参考与延伸

- 原书 §5.1
- [4.1 算术指令](../../chapter-04-a64-arithmetic-shift/notes/01-arithmetic.md)
- [4.2 NZCV](../../chapter-04-a64-arithmetic-shift/notes/02-nzcv.md)
- [5.4 条件后缀](04-condition-suffix.md)
