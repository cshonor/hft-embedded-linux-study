# 9.5 分析链接结果

> 来源：§9.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

用 readelf/objdump/nm 分析链接后的 ELF 文件。

## 核心要点

| 工具 | 用途 |
|------|------|
| `readelf -l` | 程序头（段布局） |
| `nm` | 符号表 |
| `objdump -d` | 反汇编代码段 |
| `size` | 段大小 |

## 自测题

1. `nm` 输出中 `T`、`D`、`B`、`U` 分别代表什么？
<details><summary>答案</summary>
T=Text(代码)，D=Data(已初始化数据)，B=BSS(未初始化)，U=Undefined(未定义)。小写表示局部符号(static)。
</details>

2. `readelf -l` 和 `readelf -S` 的区别？
<details><summary>答案</summary>
`-l` 显示 PROGRAM headers（加载视图），`-S` 显示 SECTION headers（链接视图）。
</details>

3. 如何确认某函数被 `--gc-sections` 删除了？
<details><summary>答案</summary>
`nm` 找不到该符号，或 `-Map` 中标记为 "discarded"。
</details>

## 参考与延伸

- 原书 §9.5
- [9.4 链接选项](04-linker-options.md)
