# 8.5 C ↔ 汇编互调

> 来源：§8.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

C 函数调用汇编函数，以及汇编调用 C 函数——AAPCS64 调用约定的实践。

## 核心要点

AAPCS64 调用约定：
| 寄存器 | 用途 |
|--------|------|
| X0-X7 | 参数 1-8 / 返回值 |
| X9-X15 | 临时寄存器（caller-saved） |
| X19-X28 | 被调用者保存（callee-saved） |
| X29 | FP 帧指针 |
| X30 | LR 返回地址 |

```asm
.global my_func
my_func:
    add x0, x0, x1    // x0=a, x1=b，返回值放 x0
    ret
```

## HFT 关联

- 热路径函数用汇编手写 → 通过 AAPCS64 与 C 代码无缝衔接
- 参数最多 8 个在寄存器中传递 → 避免栈传参开销
- callee-saved 寄存器（X19-X28）需要保存恢复

## 自测题

1. C 函数 `int add(int a, int b)` 调用时，a 和 b 分别在哪个寄存器？
<details><summary>答案</summary>
a 在 X0，b 在 X1。返回值放 X0。
</details>

2. 汇编函数中可以使用 X19 吗？有什么要求？
<details><summary>答案</summary>
可以用，但 X19 是 callee-saved。使用前必须保存原值（压栈），返回前恢复。否则调用者的 X19 被破坏。
</details>

3. 如果 C 函数有 10 个参数，后 2 个怎么传？
<details><summary>答案</summary>
第 9-10 个参数通过栈传递。调用者在栈上分配空间，把参数写入 [sp+0] 和 [sp+8]。比寄存器传参慢。
</details>

## 参考与延伸

- 原书 §8.5
- [Ch21 调用约定与栈帧](../../chapter-21-os-topics/notes/section-0-本章完整概述.md)
- [Ch10 GCC 内联汇编](../../chapter-10-gcc-inline-asm/notes/section-0-本章完整概述.md)
