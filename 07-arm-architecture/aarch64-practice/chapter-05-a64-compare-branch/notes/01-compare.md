# 5.1 比较指令 CMP / CMN

> 来源：§5.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

CMP 和 CMN 两条比较指令的本质、NZCV 标志如何被设置、以及它们与 SUBS/ADDS 的等价关系。

## 核心要点

### 指令本质

| 指令 | 等价指令 | 运算 | 用途 |
|------|----------|------|------|
| CMP Xn, Xm | SUBS XZR, Xn, Xm | Xn - Xm | 比较大小 |
| CMP Xn, #imm | SUBS XZR, Xn, #imm | Xn - imm | 与立即数比较 |
| CMN Xn, Xm | ADDS XZR, Xn, Xm | Xn + Xm | 负数比较 |
| CMN Xn, #imm | ADDS XZR, Xn, #imm | Xn + imm | 与负立即数比较 |

关键点：**CMP 和 CMN 都把运算结果写入 XZR（零寄存器），等价于丢弃结果**，但 NZCV 标志照常更新。

### CMP 详解

CMP 执行减法 `Xn - Xm`，根据结果设置 NZCV：

```
示例 1：CMP x0, x1（x0=5, x1=3）
  5 - 3 = 2（正数）
  N=0（结果非负）, Z=0（非零）, C=1（无借位）, V=0（无溢出）
  → B.GT 跳转（5 > 3 成立）

示例 2：CMP x0, x1（x0=3, x1=5）
  3 - 5 = -2（0xFFFFFFFFFFFFFFFE，补码）
  N=1（结果为负）, Z=0, C=0（有借位）, V=0
  → B.LT 跳转（3 < 5 成立）

示例 3：CMP x0, x1（x0=5, x1=5）
  5 - 5 = 0
  N=0, Z=1（结果为零）, C=1（无借位）, V=0
  → B.EQ 跳转（相等）
```

### CMN 详解

CMN 执行加法 `Xn + Xm`，等价于 `CMP Xn, (-Xm)`：

```asm
; 以下两条等价：
CMN x0, #5       ; x0 + 5 → 设标志
CMP x0, #-5      ; x0 - (-5) = x0 + 5 → 设标志

; CMN 的用途：与负数比较时省去取负操作
; 想判断 x0 == -5：
;   方法1：MOV x1, #-5; CMP x0, x1   → 2 条指令
;   方法2：CMN x0, #5                 → 1 条指令
```

### CMP 的立即数范围

CMP 支持的立即数范围与 SUBS 相同（12 位编码）：

```
CMP x0, #imm12
  - imm12 范围：0 ~ 4095
  - 可选左移 12 位：imm12 << 12（即 4096 的倍数，最大 0xFFF000）
  - 超出范围需用 MOV 加载到寄存器再 CMP
```

```asm
; 合法：立即数在范围内
CMP x0, #100
CMP x0, #4095
CMP x0, #0x1000    ; = 4096 << 0... 不对，4096 = 0x1000 超出 12 位
                    ; 但 0x1000 = 1 << 12，所以编码为 imm=1, shift=12 → 合法

; 非法：需用寄存器
; CMP x0, #5000    ; 编译报错
MOV x1, #5000
CMP x0, x1
```

### CMP 不修改操作数

```asm
; CMP 执行后 x0 和 x1 的值不变，只有 NZCV 改变
CMP x0, x1
; x0 仍为原值，x1 仍为原值
; 只有 NZCV 被更新
```

## 与 C 的对照

| C 代码 | AArch64 汇编 |
|--------|-------------|
| `if (a > b)` | `cmp x0, x1; b.gt label` |
| `if (a == b)` | `cmp x0, x1; b.eq label` |
| `if (a != 0)` | `cbnz x0, label`（或 `cmp x0, #0; b.ne`）|
| `if (a < 0)` | `cmp x0, #0; b.mi label`（或 `tbnz x0, #63`）|
| `if (a == -5)` | `cmn x0, #5; b.eq label` |

## 常见错误

1. **混淆有符号/无符号**：CMP 设置的 NZCV 对有符号和无符号都有效，但后续 B.cond 必须选对——B.LT（有符号）vs B.LO（无符号）。
2. **以为 CMP 会修改寄存器**：CMP 丢弃结果到 XZR，源操作数不变。如果需要结果，用 SUBS。
3. **CMN 方向搞反**：`CMN x0, x1` 是 `x0 + x1`，不是 `x0 - x1`。判断 `x0 == -x1` 时用 `CMN x0, x1; B.EQ`。

## HFT 关联

比较是条件分支的基础：
- 价格比较 `cmp bid_price, ask_price; b.ge trade` 判断是否可套利
- 数量比较 `cmp order_qty, #0; b.eq skip` 过滤空订单
- CMN 较少使用，但在与负数比较时可以省去显式取负
- CMP 不修改寄存器 → 可以连续比较同一个值与多个阈值

```asm
; HFT 典型：价格区间检查
cmp x0, min_price
b.lo reject          ; 价格低于下限 → 拒绝
cmp x0, max_price
b.hi reject          ; 价格高于上限 → 拒绝
; x0 未被修改，可以继续使用
```

## 自测题

1. `CMP x0, #5` 执行后 x0 = 3，NZCV 各是什么？
<details><summary>答案</summary>
3 - 5 = -2（0xFFFFFFFFFFFFFFFE）。N=1（负），Z=0（非零），C=0（有借位），V=0（无符号溢出）。B.LT（N≠V = 1≠0）会跳转，3 < 5 成立。
</details>

2. CMN x0, x1 和 CMP x0, x1 的区别？
<details><summary>答案</summary>
CMP 做减法 x0 - x1。CMN 做加法 x0 + x1，等价于 CMP x0, (-x1)。CMN 用于和负数比较时避免显式取负：`CMN x0, #5` 等价于 `CMP x0, #-5`。
</details>

3. 为什么 CMP 要用 SUBS 而不是单独的比较指令？
<details><summary>答案</summary>
AArch64 没有"纯比较"指令。CMP 复用 SUBS 的硬件（结果写 XZR 丢弃），减少指令集复杂度。这是一种 RISC 设计哲学——用组合实现复杂操作。
</details>

4. 以下代码执行后，NZCV 各是什么？
```asm
MOV x0, #0x7FFFFFFFFFFFFFFF   ; INT64_MAX
MOV x1, #1
CMP x0, x1
```
<details><summary>答案</summary>
INT64_MAX - 1 = 0x7FFFFFFFFFFFFFFE（正数，无溢出）。N=0, Z=0, C=1（无借位），V=0。但如果是 `CMP x0, x1` 且 x0=INT64_MIN, x1=-1：INT64_MIN - (-1) = INT64_MIN + 1，但 x1=-1 的补码是 0xFFF...F，CMP 做 x0 - x1 = 0x800...0 - 0xFFF...F = 0x800...01 → 这会溢出，V=1。
</details>

5. 写出判断 `x0 == -1` 的两种方式（用 CMP 和用 CMN）。
<details><summary>答案</summary>
```asm
; 方法1：CMP（需要先把 -1 放入寄存器）
MOV x1, #-1
CMP x0, x1
B.EQ is_negative_one

; 方法2：CMN（1 条比较指令）
CMN x0, #1         ; x0 + 1，如果 x0 = -1 则结果为 0
B.EQ is_negative_one
```
CMN 方式更简洁：`x0 + 1 == 0` 当且仅当 `x0 == -1`。
</details>

## 参考与延伸

- 原书 §5.1
- [4.1 算术指令](../../chapter-04-a64-arithmetic-shift/notes/01-arithmetic.md)
- [4.2 NZCV](../../chapter-04-a64-arithmetic-shift/notes/02-nzcv.md)
- [5.4 条件后缀](04-condition-suffix.md)
