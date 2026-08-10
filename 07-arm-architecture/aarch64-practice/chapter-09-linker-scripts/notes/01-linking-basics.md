# 9.1 链接基本概念

> 来源：§9.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

链接器的作用：把多个 .o 目标文件合并为可执行文件，解析符号引用。

## 核心要点

链接器三个核心任务：符号解析、段合并、地址分配。

- 默认链接脚本：`ld --verbose` 查看
- LTO（链接时优化）可以跨文件内联

## HFT 关联

- 段布局影响 cache 行为 → 热路径代码集中放置减少 icache miss
- 链接脚本控制内存布局 → 把代码放在特定内存区域

## 自测题

1. 链接器为什么要合并同名段？
<details><summary>答案</summary>
1. 统一管理内存属性 2. 减少内存碎片 3. 同段内符号引用可用相对地址
</details>

2. 两个 .o 文件都定义了 `int x = 1`，链接会怎样？
<details><summary>答案</summary>
链接报错 `multiple definition of 'x'`。一个改为 `extern int x` 或用 `static` 限制作用域。
</details>

3. `ld --verbose` 输出的是什么？
<details><summary>答案</summary>
默认链接脚本。显示默认的段布局、入口地址、内存区域等。
</details>

## 参考与延伸

- 原书 §9.1
- [9.2 链接脚本语法](02-linker-script-syntax.md)
