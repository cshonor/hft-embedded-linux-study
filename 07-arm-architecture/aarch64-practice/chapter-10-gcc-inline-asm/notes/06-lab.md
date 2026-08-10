# 10.6 实验要点

> 来源：§10.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 实验列表

| 实验 | 内容 |
|------|------|
| 10-1 | 基本内联汇编（加减法） |
| 10-2 | 系统寄存器读写 |
| 10-3 | memset 实现 |
| 10-4 | 原子操作 |
| 10-5 | asm goto |

## 自测题

1. 如何验证内联汇编生成的指令正确？
<details><summary>答案</summary>
`objdump -d` 反汇编查看实际指令。GDB 断点单步验证。对比 -O0 和 -O2 的输出。
</details>

2. 内联汇编 memset 和 glibc memset 性能差距来自哪里？
<details><summary>答案</summary>
glibc 用 NEON 向量指令（一次 128 字节）+ 循环展开。简单内联用 STR（4字节）循环。
</details>

## 参考与延伸

- 原书 §10.6
- [Ch22 NEON 优化](../../chapter-22-fp-neon/notes/section-0-本章完整概述.md)
