# 9.4 常用链接器选项

> 来源：§9.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

GCC/LD 常用链接选项。

## 核心要点

| 选项 | 作用 |
|------|------|
| `-T script.ld` | 指定链接脚本 |
| `-Map=map.txt` | 输出链接映射文件 |
| `--gc-sections` | 删除未引用的段 |
| `-nostdlib` | 不链接标准库 |

## HFT 关联

- `--gc-sections` 配合 `-ffunction-sections` 删除死代码 → 减小 icache 压力
- `-static` 静态链接 → 避免运行时动态链接开销

## 自测题

1. `--gc-sections` 需要配合什么编译选项？
<details><summary>答案</summary>
`-ffunction-sections`（每函数独立段）和 `-fdata-sections`。否则函数在同一段中，一个被引用整段保留。
</details>

2. `-Map=output.map` 有什么用？
<details><summary>答案</summary>
记录每个段和符号的最终地址、大小。用于确认段布局、查找符号地址、分析代码体积。
</details>

3. 裸机开发为什么要用 `-nostdlib`？
<details><summary>答案</summary>
裸机无 OS，标准库依赖系统调用不可用。需自己实现基本函数或用 newlib。
</details>

## 参考与延伸

- 原书 §9.4
- [9.5 分析链接结果](05-analyze-output.md)
