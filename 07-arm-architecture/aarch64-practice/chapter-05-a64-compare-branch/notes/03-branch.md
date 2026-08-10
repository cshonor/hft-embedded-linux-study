# 5.3 跳转指令全览

> 来源：§5.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

B/BL/BR/BLR/RET/ERET 等跳转指令的分类和使用场景。

## 核心要点

| 指令 | 作用 | 保存 LR |
|------|------|---------|
| B | 无条件跳转 | 否 |
| B.cond | 条件跳转 | 否 |
| BL | 调用函数 | 是(X30) |
| BR | 寄存器跳转 | 否 |
| BLR | 寄存器调用 | 是(X30) |
| RET | 函数返回 | 否(用 X30) |
| ERET | 异常返回 | 否(用 ELR) |

- BL 把下一条指令地址存入 X30(LR)，然后跳转
- 嵌套 BL 必须先 STP x29,x30 压栈（否则 LR 被覆盖）
- RET 默认用 X30 返回，也可 `ret x1` 指定其他寄存器

## HFT 关联

跳转指令的延迟差异影响热路径：
- B/BL 是 PC 相对跳转，可被分支预测器缓存 → 预测正确 ~0-1 cycle
- BR/BLR 是间接跳转（寄存器目标），预测更难 → 可能 ~5+ cycles
- 函数指针/虚函数调用用 BLR → 延迟不稳定
- 尾调用优化用 B 替代 BL+RET → 省一层栈帧和返回开销

## 自测题

1. BL 和 B 的区别？
<detail><summary>答案</summary>
BL（Branch with Link）在跳转前把下一条指令地址存入 X30(LR)，用于函数返回。B（Branch）不保存返回地址，用于无条件跳转/循环。
</details>

2. 为什么嵌套函数调用必须保存 X30？
<detail><summary>答案</summary>
BL 会覆盖 X30 为新的返回地址。如果外层函数不保存 X30，内层 BL 返回后 X30 已变，外层 RET 会跳到错误地址。所以非叶子函数入口必须 STP x29, x30 压栈保存。
</details>

3. `ret x1` 和 `ret`（默认）的区别？
<detail><summary>答案</summary>
`ret` 默认用 X30(LR) 作为返回地址。`ret x1` 用 x1 作为返回地址。非默认形式用于替代调用约定，如尾调用优化或协程切换。
</details>

## 参考与延伸

- 原书 §5.3
- [3.4 STP/LDP 栈帧](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
- [Ch11 ERET 异常返回](../../chapter-11-exception-handling/notes/section-0-本章完整概述.md)
