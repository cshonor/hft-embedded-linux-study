# 8.2 常用伪指令/伪操作

> 来源：§8.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

GNU as 常用伪操作：.global/.text/.data/.align/.word/.quad/.string 等。

## 核心要点

| 伪操作 | 作用 |
|--------|------|
| `.global sym` | 声明全局符号 |
| `.text` | 代码段 |
| `.data` | 数据段 |
| `.bss` | 未初始化数据段 |
| `.align n` | 对齐 2^n 字节（AArch64） |
| `.word val` | 4 字节常量 |
| `.quad val` | 8 字节常量 |
| `.string "str"` | 字符串（含 NULL） |

- `.align n` 在 AArch64 上是 2^n 对齐（与 x86 的 n 字节对齐不同）
- `.string` 自动加 NULL 结尾，`.ascii` 不加

## HFT 关联

- `.align` 控制 cache line 对齐 → HFT 数据结构需 64 字节对齐避免 false sharing
- `.bss` 的零初始化数据不占二进制空间 → 减少加载时间
- 理解段概念是链接脚本（Ch9）的基础

## 自测题

1. `.align 4` 在 AArch64 上对齐多少字节？
<details><summary>答案</summary>
2^4 = 16 字节。注意与 x86 不同——x86 的 `.align 4` 是 4 字节对齐。AArch64 的参数是 2 的幂指数。
</details>

2. `.string` 和 `.ascii` 的区别？
<details><summary>答案</summary>
`.string "Hello"` 存储 6 字节（5字符 + NULL）。`.ascii "Hello"` 存储 5 字节（无 NULL）。
</details>

3. 以下代码中 `counter` 的地址对齐到多少？
```asm
.data
msg:
    .string "Hi"    // 3 字节
counter:
    .quad 0
```
<details><summary>答案</summary>
`counter` 紧跟 `msg` 后，地址为 msg+3，未对齐到 8 字节。应加 `.align 3` 再定义 `counter`。
</details>

## 参考与延伸

- 原书 §8.2
- [8.3 段](03-sections.md)
- [Ch9 链接脚本](../../chapter-09-linker-scripts/notes/section-0-本章完整概述.md)
