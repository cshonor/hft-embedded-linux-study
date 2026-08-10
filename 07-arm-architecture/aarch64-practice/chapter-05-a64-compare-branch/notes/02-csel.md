# 5.2 条件选择指令 CSEL/CSET

> 来源：§5.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

CSEL/CSET/CSINC 条件选择指令，实现无分支的条件赋值。

## 核心要点

| 指令 | 作用 |
|------|------|
| CSEL | 条件选择：cond ? x1 : x2 |
| CSET | 条件置 1：cond ? 1 : 0 |
| CSINC | 条件选择+自增：cond ? x1 : x2+1 |

```asm
cmp x0, x1
csel x2, x0, x1, ge   ; x2 = (x0 >= x1) ? x0 : x1  → max(x0, x1)
```

- **无分支** → 无分支预测失败风险
- 条件来自 NZCV 标志（前一条 CMP/SUBS 设置）
- CSET 等价于 CSEL Xn, XZR, #1（简化版）

## HFT 关联

无分支代码是 HFT 低延迟的核心技术：
- `csel` 实现 max/min/abs 无分支 → 消除分支预测失败（~20 cycles penalty）
- 条件赋值避免 if-else 产生的跳转 → 流水线不中断
- 编译器会自动把简单三元运算符转换为 CSEL
- 在热路径中手动用内联汇编确保 CSEL 生成

## 自测题

1. 用 CSEL 实现 max(x0, x1) 到 x2。
<details><summary>答案</summary>
```asm
cmp x0, x1
csel x2, x0, x1, ge   ; x0 >= x1 ? x0 : x1
```
</details>

2. CSET x0, eq 的作用是什么？
<details><summary>答案</summary>
如果 EQ 条件成立（Z=1）则 x0=1，否则 x0=0。等价于将条件标志转换为布尔值。常用于将 CMP 结果转为 0/1 值。
</details>

3. 为什么 CSEL 比分支更快？
<details><summary>答案</summary>
分支有预测失败风险：预测正确时 ~1 cycle，预测失败时 ~20 cycles（流水线冲刷）。CSEL 无条件执行，始终 ~1 cycle，延迟可预测。在 HFT 中延迟可预测性比平均延迟更重要。
</details>

## 参考与延伸

- 原书 §5.2
- [4.2 NZCV](../../chapter-04-a64-arithmetic-shift/notes/section-0-本章完整概述.md)
- [4.6 典型例子](../../chapter-04-a64-arithmetic-shift/notes/section-0-本章完整概述.md)
