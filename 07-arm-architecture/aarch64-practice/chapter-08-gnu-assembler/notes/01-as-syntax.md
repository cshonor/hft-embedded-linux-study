# 8.1 GNU as 基本语法

> 来源：§8.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

GNU as 汇编器的基本语法：指令大小写、注释、标号、指令格式。

## 核心要点

| 特性 | GNU as 风格 | ARM 官方手册风格 |
|------|------------|-----------------|
| 指令 | **小写**（推荐） | 大写 |
| 注释 | `//` 或 `/* */` 或 `;` | `;` |
| 标号 | `label:` 在行首 | 同 |
| 立即数 | `#imm` | `#imm` |

- GNU as 默认小写（本仓库统一用小写）
- 标号必须顶格（行首无空格），指令必须缩进
- `.directive` 是汇编器伪操作（.global/.text/.align 等）

## HFT 关联

汇编语法规范影响代码可读性和维护：
- 内核中 ARM64 汇编统一用 GNU as 小写风格
- 内联汇编也遵循 GNU as 语法
- HFT 中手动优化的热路径函数可能用汇编编写 → 统一风格便于团队维护
- 理解 `.directive` 伪操作对阅读内核汇编（如 head.S）至关重要

## 自测题

1. 以下代码有什么语法错误？
```asm
_start:
.global _start
    mov x0, #1
```
<details><summary>答案</summary>
`.global _start` 必须在标号 `_start:` 之前。伪操作 `.global` 声明全局符号，需在使用标号之前声明。
</details>

2. GNU as 中指令用大写还是小写？
<details><summary>答案</summary>
GNU as 大小写都支持，但推荐小写。ARM 官方手册用大写，但 GNU as 社区惯例是小写。本仓库统一用小写。
</details>

3. 标号和指令的缩进有什么规则？
<details><summary>答案</summary>
标号必须顶格（行首无空格），否则被当作指令。指令建议缩进以提高可读性。这是 GNU as 的语法要求。
</details>

## 参考与延伸

- 原书 §8.1
- [8.2 伪指令](02-directives.md)
- GNU as Manual §3
