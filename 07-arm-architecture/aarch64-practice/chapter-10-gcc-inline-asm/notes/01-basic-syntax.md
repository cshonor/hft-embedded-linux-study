# 10.1 基本语法

> 来源：§10.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

GCC 内联汇编的基本语法：asm、输入/输出操作数、约束。

## 核心要点

```c
asm volatile (
    "汇编指令"
    : 输出操作数      // 结果写回 C 变量
    : 输入操作数      // C 变量传入汇编
    : clobber 列表    // 被修改的寄存器
);
```

- `volatile` 防止编译器优化掉汇编
- `"=r"` 输出约束，`"r"` 输入约束
- `%0`、`%1` 引用操作数

## HFT 关联

- 读取时间戳 `mrs x0, cntvct_el0` → 精确计时
- 手动优化关键指令序列 → 编译器不优化的场景

## 自测题

1. `asm` 和 `asm volatile` 的区别？
<details><summary>答案</summary>
`asm` 可能被优化掉（如果输出未使用）。`asm volatile` 有副作用不能删除。读系统寄存器、屏障必须加 volatile。
</details>

2. `"=r"` 中的 `=` 和 `r` 分别代表什么？
<details><summary>答案</summary>
`=` 表示输出（write-only），`r` 表示通用寄存器。
</details>

3. 为什么多条指令要用 `\n` 分隔？
<details><summary>答案</summary>
GCC 把字符串原样传给汇编器。`\n` 让每条指令单独一行，GNU as 按行解析。
</details>

## 参考与延伸

- 原书 §10.1
- [10.2 约束字符](02-constraints.md)
