# 4.1 算术指令

> 来源：§4.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

ADD/SUB/CMP 等算术指令，以及带进位/借位的 ADC/SBC。

## 核心要点

| 指令 | 作用 |
|------|------|
| ADD | 加法 |
| ADDS | 加法 + 设置 NZCV |
| ADC | 带进位加法 |
| SUB | 减法 |
| SUBS | 减法 + 设置 NZCV |
| CMP | 比较（≡ SUBS XZR） |
| CMN | 负数比较（≡ ADDS XZR） |

- CMP 本质是 `SUBS Xn, Xm, XZR`（丢弃结果，只设标志）
- S 后缀 = Set flags（设置 NZCV）
- ADC/SBC 用于多精度运算（如 128 位加减）

## HFT 关联

算术指令是所有计算的基础：
- ADDS/SUBS 设置的 NZCV 标志用于条件分支 → 避免额外 CMP
- 多精度运算（ADC/SBC）在加密/大数运算中使用，HFT 中较少直接用
- 条件选择指令 CSEL（5.2 节）利用 NZCV 无分支选择值 → 消除分支预测失败

## 自测题

1. `CMP x0, x1` 等价于什么指令？
<details><summary>答案</summary>
`SUBS XZR, x0, x1`——减法结果丢弃（写入 XZR），只设置 NZCV 标志。
</details>

2. `ADD` 和 `ADDS` 的区别是什么？
<details><summary>答案</summary>
ADD 只做加法，不修改 NZCV。ADDS 做加法并设置 NZCV 标志（N=结果为负、Z=结果为零、C=无符号进位、V=有符号溢出）。
</details>

3. 如何用 ADC 实现 128 位加法？
<details><summary>答案</summary>
```asm
adds x0, x0, x2    ; 低 64 位加，设置 C 标志
adc  x1, x1, x3    ; 高 64 位加 + 进位
```
先 ADDS 设置进位标志，再 ADC 把进位加到高位。
</details>

## 参考与延伸

- 原书 §4.1
- [4.2 NZCV 标志](02-nzcv.md)
- [5.2 条件选择指令](../../chapter-05-a64-compare-branch/notes/section-0-本章完整概述.md)
