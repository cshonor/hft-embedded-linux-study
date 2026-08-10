# 8.4 宏

> 来源：§8.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

GNU as 的宏定义和使用，减少重复汇编代码。

## 核心要点

```asm
.macro SAVE_REGS
    stp x29, x30, [sp, #-16]!
    stp x0,  x1,  [sp, #-16]!
.endm

irq_handler:
    SAVE_REGS           // 展开为上面的指令
```

- `\reg` 是宏参数引用
- 宏在汇编阶段展开
- `.if`/`.endif` 支持条件汇编

## HFT 关联

- 异常处理中保存/恢复寄存器的代码重复 → 宏消除重复
- 但宏调试困难（GDB 看到的是展开后的代码）
- 现代倾向用 C 内联函数替代汇编宏

## 自测题

1. 汇编宏和 C 宏有什么区别？
<details><summary>答案</summary>
展开时机：汇编宏在汇编阶段，C 宏在预处理阶段。参数类型：汇编宏参数无类型，纯文本替换。
</details>

2. 宏参数引用用什么符号？
<details><summary>答案</summary>
反斜杠 `\`。如 `.macro FUNC reg` 中引用参数用 `\reg`。
</details>

3. 以下宏有什么效率问题？
```asm
.macro PUSH reg
    str \reg, [sp, #-8]!
.endm
```
<details><summary>答案</summary>
str 单个寄存器入栈效率不如 stp。多次 PUSH 后 SP 可能不 16 对齐。应改用 stp 一次存两个寄存器。
</details>

## 参考与延伸

- 原书 §8.4
- [8.5 C↔汇编互调](05-c-asm-interop.md)
- GNU as Manual §7
