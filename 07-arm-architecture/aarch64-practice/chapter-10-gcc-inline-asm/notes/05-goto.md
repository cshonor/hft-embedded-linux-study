# 10.5 goto 模板

> 来源：§10.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

`asm goto` —— 内联汇编可以直接跳转到 C 标签。

## 核心要点

```c
asm goto(
    "tbz %w0, #0, %l[zero]\n"
    : : "r"(x) : : zero
);
return 1;
zero:
return 0;
```

- `%l[label]` 引用 C 标签
- 内核中用于 `static_branch`（静态键）

## HFT 关联

- `static_branch` 让调试代码默认零开销（NOP）→ 需要时才跳转
- 比 if 判断更高效（无分支预测，直接 NOP 或 JMP）

## 自测题

1. `asm goto` 和普通 `asm` 的区别？
<details><summary>答案</summary>
`asm goto` 可从汇编跳转到 C 标签。没有输出操作数——结果通过跳转表达。
</details>

2. `static_branch` 如何实现零开销？
<details><summary>答案</summary>
默认放 NOP 指令（几乎无开销）。需要开启时把 NOP 改为 B label。修改代码后 flush I-cache。
</details>

3. `asm goto` 为什么没有输出操作数？
<details><summary>答案</summary>
输出通过跳转表达。同时有输出和跳转的复杂度太高，GCC 限制 asm goto 不能有输出。
</details>

## 参考与延伸

- 原书 §10.5
- [10.3 实战示例](03-examples.md)
