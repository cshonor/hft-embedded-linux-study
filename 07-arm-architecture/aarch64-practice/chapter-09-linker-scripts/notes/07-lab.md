# 9.7 实验要点

> 来源：§9.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 实验列表

| 实验 | 内容 |
|------|------|
| 9-1 | 编写简单链接脚本 |
| 9-2 | VMA ≠ LMA 实验 |
| 9-3 | 分析 ELF 文件 |
| 9-4 | --gc-sections 效果 |
| 9-5 | BenOS 链接脚本 |

## 自测题

1. 如何验证 VMA ≠ LMA？
<details><summary>答案</summary>
`readelf -l` 查看 LOAD 段的 VirtAddr(VMA) 和 PhysAddr(LMA)。两者不同则 VMA ≠ LMA。
</details>

2. `--gc-sections` 前后 size 输出变化说明了什么？
<details><summary>答案</summary>
text 段变小 → 未引用的函数被删除（死代码消除）。
</details>

## 参考与延伸

- 原书 §9.7
- [9.5 分析链接结果](05-analyze-output.md)
