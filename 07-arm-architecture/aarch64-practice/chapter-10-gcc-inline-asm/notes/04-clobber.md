# 10.4 clobber 列表

> 来源：§10.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

clobber 列表声明汇编修改的寄存器和内存。

## 核心要点

| clobber | 含义 |
|---------|------|
| `"memory"` | 汇编修改内存（编译器屏障） |
| `"cc"` | 修改 NZCV 条件标志 |
| 寄存器名 | 修改该寄存器 |

## HFT 关联

- `"memory"` 是编译器屏障，阻止内存访问重排
- 过多 `"memory"` 限制编译器优化

## 自测题

1. 什么时候需要 `"cc"` clobber？
<details><summary>答案</summary>
汇编修改了 NZCV 条件标志时（如 ADDS/SUBS/CMP）。告诉编译器标志被修改，后续条件判断需重新评估。
</details>

2. `"memory"` clobber 等价于什么屏障？
<details><summary>答案</summary>
编译器屏障（compiler barrier），等价于 `barrier()` 宏。阻止编译器重排内存访问，但不生成 CPU 内存屏障指令。
</details>

3. 如果汇编修改了 X0 但没在 clobber 中声明？
<details><summary>答案</summary>
编译器可能把 C 变量分配在 X0 中。汇编修改 X0 后 C 变量值被破坏 → 难以调试的 bug。
</details>

## 参考与延伸

- 原书 §10.4
- [Ch18 编译器屏障 vs CPU 屏障](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md)
