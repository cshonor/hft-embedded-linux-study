# 5.4 条件后缀速查

> 来源：§5.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

所有条件后缀的含义速查表，区分有符号/无符号比较。

## 核心要点

| 后缀 | 含义 | 条件 | 类型 |
|------|------|------|------|
| EQ | Equal | Z=1 | 通用 |
| NE | Not Equal | Z=0 | 通用 |
| LT | Less Than | N≠V | 有符号 |
| LE | Less or Equal | !(N==V and Z==0) | 有符号 |
| GT | Greater Than | N==V and Z==0 | 有符号 |
| GE | Greater or Equal | N==V | 有符号 |
| LO | Lower | C=0 | 无符号 |
| LS | Lower or Same | C=0 or Z=1 | 无符号 |
| HI | Higher | C=1 and Z=0 | 无符号 |
| HS | Higher or Same | C=1 | 无符号 |
| MI | Minus/Negative | N=1 | 标志 |
| PL | Plus/Positive | N=0 | 标志 |
| VS | oVerflow Set | V=1 | 标志 |
| VC | oVerflow Clear | V=0 | 标志 |

> **HS = CS（Carry Set）, LO = CC（Carry Clear）**——同一条件的两个别名。

## HFT 关联

正确选择条件后缀避免逻辑 bug：
- 地址/大小比较用 HS/LO（无符号）—— 地址不可能为负
- 价格/数量比较用 GE/LT（有符号）—— 可能有负值（如价差）
- EQ/NE 通用，但注意浮点比较用专门的 FCMP
- MI/PL 直接检测符号位，比 CMP #0 更简洁

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

## 参考与延伸

- 原书 §5.4
- [4.2 NZCV](../../chapter-04-a64-arithmetic-shift/notes/section-0-本章完整概述.md)
- [NZCV 专篇](../../NZCV.md)
