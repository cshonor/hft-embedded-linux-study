# 5.4 条件后缀速查

> 来源：§5.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AArch64 所有条件后缀的完整含义、NZCV 标志组合、以及有符号/无符号的区分。

## 核心要点

### 完整条件后缀表

| 后缀 | 含义 | 条件 | 类型 | NZCV 组合 |
|------|------|------|------|-----------|
| EQ | Equal | Z=1 | 通用 | Z=1 |
| NE | Not Equal | Z=0 | 通用 | Z=0 |
| **有符号比较** | | | | |
| GT | Greater Than | N==V and Z=0 | 有符号 | N==V, Z=0 |
| GE | Greater or Equal | N==V | 有符号 | N==V |
| LT | Less Than | N≠V | 有符号 | N≠V |
| LE | Less or Equal | !(N==V and Z=0) | 有符号 | N≠V or Z=1 |
| **无符号比较** | | | | |
| HI | Higher | C=1 and Z=0 | 无符号 | C=1, Z=0 |
| HS | Higher or Same | C=1 | 无符号 | C=1 |
| LO | Lower | C=0 | 无符号 | C=0 |
| LS | Lower or Same | C=0 or Z=1 | 无符号 | C=0 or Z=1 |
| **标志检测** | | | | |
| MI | Minus/Negative | N=1 | 标志 | N=1 |
| PL | Plus/Positive | N=0 | 标志 | N=0 |
| VS | oVerflow Set | V=1 | 标志 | V=1 |
| VC | oVerflow Clear | V=0 | 标志 | V=0 |
| AL | Always | 无条件 | 特殊 | 总是真 |

> **别名关系**：HS = CS（Carry Set），LO = CC（Carry Clear）——同一条件的两个名字。

### 有符号 vs 无符号——最容易出错的地方

```
CMP x0, x1 的本质是 x0 - x1，结果设置 NZCV

有符号解释（LT/GE/GT/LE）：
  N = 结果符号位
  V = 有符号溢出标志
  N==V 表示结果非负（没有"符号翻转"）
  N≠V 表示结果为负（发生了"符号翻转"）

无符号解释（LO/HS/HI/LS）：
  C = 无符号借位的反相（C=1 表示无借位，C=0 表示有借位）
  C=1 → x0 >= x1（无符号）
  C=0 → x0 < x1（无符号）
```

### 具体例子对比

```
CMP x0, x1（x0 = 0x0000000000000003, x1 = 0xFFFFFFFFFFFFFFFF）
即 x0 = 3（有符号/无符号都是3）
   x1 = -1（有符号）或 18446744073709551615（无符号）

x0 - x1 = 3 - (-1) = 4（有符号视角）
x0 - x1 = 3 - 18446744073709551615 = 借位（无符号视角）

NZCV 结果：N=0, Z=0, C=0（有借位）, V=0

有符号判断：N==V (0==0) → GE 成立 → 3 >= -1 ✓ 正确
无符号判断：C=0 → LO 成立 → 3 < 巨大数 ✓ 正确

如果用 LT：N==V 为 false → LT 不成立 → 3 不小于 -1 ✓ 正确
如果用 LO：C=0 → LO 成立 → 3 小于 巨大数 ✓ 正确
如果误用 LT 判断地址大小：3 不小于 -1（有符号）→ 但无符号下 3 确实更小 → BUG！
```

### 条件后缀速记图

```
                    CMP x0, x1 后：

    有符号                          无符号
    ──────                          ──────
    x0 > x1  → GT                   x0 > x1  → HI
    x0 >= x1 → GE                   x0 >= x1 → HS (= CS)
    x0 < x1  → LT                   x0 < x1  → LO (= CC)
    x0 <= x1 → LE                   x0 <= x1 → LS

    通用：EQ (==), NE (!=)
    标志：MI (N=1), PL (N=0), VS (V=1), VC (V=0)
    特殊：AL (always)
```

### NZCV 各标志的含义

| 标志 | 全称 | CMP 后含义 |
|------|------|-----------|
| N | Negative | 结果符号位（1=负数） |
| Z | Zero | 结果是否为零（1=相等） |
| C | Carry | 无符号借位的反相（1=无借位=x0≥x1） |
| V | Overflow | 有符号溢出（1=溢出） |

### 条件后缀的推导

```
为什么 LT 的条件是 N≠V？

CMP x0, x1 做 x0 - x1：
- 如果结果为负（N=1）且没有溢出（V=0）→ 真的为负 → x0 < x1 ✓
- 如果结果为正（N=0）但溢出了（V=1）→ 实际应该为负 → x0 < x1 ✓
  （正溢出：大负数减小正数，结果"看起来正"但实际为负）
- 综合两种情况：N≠V 时 x0 < x1

为什么 LO 的条件是 C=0？
CMP 做 x0 - x1，如果 x0 < x1（无符号），会产生借位。
ARM 的 C 标志在减法中是"借位的反相"：
  有借位 → C=0 → x0 < x1 → LO 成立
  无借位 → C=1 → x0 >= x1 → HS 成立
```

## 与 C 的对照

| C 比较 | 有符号条件 | 无符号条件 | 说明 |
|--------|-----------|-----------|------|
| `a > b` (int) | B.GT | — | 默认有符号 |
| `a > b` (unsigned) | — | B.HI | 需要无符号 |
| `a >= b` (int) | B.GE | — | |
| `a >= b` (uint) | — | B.HS | |
| `a == b` | B.EQ | B.EQ | 通用 |
| `a != b` | B.NE | B.NE | 通用 |
| `a < 0` | B.MI | — | 只有有符号 |

## 常见错误

1. **地址比较用 LT/GE**：地址是无符号数，必须用 LO/HS。内核地址高位为1会被当成负数。
2. **混淆 HS 和 HI**：HS 是 ≥（含等于），HI 是 >（不含等于）。差一个 EQ。
3. **混淆 LE 和 LT**：LE 是 ≤（含等于），LT 是 <（不含等于）。LE = LT || EQ。

## HFT 关联

正确选择条件后缀避免逻辑 bug：
- 地址/大小比较用 HS/LO（无符号）—— 地址不可能为负
- 价格/数量比较用 GE/LT（有符号）—— 可能有负值（如价差）
- EQ/NE 通用，但注意浮点比较用专门的 FCMP
- MI/PL 直接检测符号位，比 CMP #0 更简洁

```asm
; HFT 正确用法
cmp order_qty, #0
b.eq skip              ; 数量=0 跳过（有符号/无符号都行）

cmp buffer_ptr, buffer_end
b.hs overflow          ; 地址比较用 HS（无符号 ≥）

cmp price_diff, #0
b.lt negative_spread   ; 价差<0 用 LT（有符号 <）
```

## 自测题

1. 比较两个内存地址 x0 和 x1，判断 x0 < x1 用哪个条件后缀？
<details><summary>答案</summary>
B.LO（无符号小于）。地址是无符号数，不能用 LT（有符号）。如果地址在内核空间（高位为1），LT 会把它当负数，比较结果错误。
</details>

2. HS 和 CS 是什么关系？
<details><summary>答案</summary>
完全等价。HS（Higher or Same）和 CS（Carry Set）都表示 C=1，是同一条件的两个别名。HS 用于无符号比较语境，CS 用于标志位语境。
</details>

3. 如何判断减法结果是否溢出（有符号）？
<details><summary>答案</summary>
检查 V 标志：B.VS（溢出）或 B.VC（无溢出）。V=1 表示有符号运算结果超出表示范围。例如 INT_MAX + 1 会导致 V=1。
</details>

4. `CMP x0, x1` 后 x0=3, x1=3，哪些条件后缀成立？
<details><summary>答案</summary>
Z=1, N=0, C=1(无借位), V=0。
- EQ（Z=1）✓
- NE（Z=0）✗
- GE（N==V, 0==0）✓
- LE（N≠V or Z=1, Z=1）✓
- HS（C=1）✓
- LS（C=0 or Z=1, Z=1）✓
- GT（N==V and Z=0, Z=1）✗
- HI（C=1 and Z=0, Z=1）✗
- LT（N≠V, 0≠0）✗
- LO（C=0）✗
</details>

5. 为什么 `B.LT` 的条件是 `N≠V` 而不是 `N=1`？
<details><summary>答案</summary>
N=1 只表示结果的符号位为1，但如果有溢出（V=1），符号位可能被"翻转"了——实际结果为正但符号位为负（负溢出），或实际为负但符号位为正（正溢出）。N≠V 综合考虑了溢出情况：N=1,V=0（真负）或 N=0,V=1（溢出导致符号翻转，实际也是负）→ 都表示 x0 < x1。
</details>

## 参考与延伸

- 原书 §5.4
- [4.2 NZCV](../../chapter-04-a64-arithmetic-shift/notes/02-nzcv.md)
- [NZCV 专篇](../../NZCV.md)
- [SIGNED-UNSIGNED 有符号无符号详解](../../SIGNED-UNSIGNED.md)
