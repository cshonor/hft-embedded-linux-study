# 8.3 段（Section）

> 来源：§8.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

.text/.data/.bss 段的概念和自定义段的使用。

## 核心要点

| 段 | 内容 | 可写 | 占二进制 |
|----|------|------|----------|
| .text | 代码 | 否 | 是 |
| .data | 已初始化数据 | 是 | 是 |
| .bss | 未初始化数据 | 是 | 否（零填充） |
| .rodata | 只读数据 | 否 | 是 |

自定义段：`.section .mydata, "aw"`（可读可写）

## HFT 关联

- .text 标记只读+可执行 → 防止意外写入
- .data 标记可写不可执行 → 防止代码注入
- 内核 `__init` 段在启动后释放 → 节省内存

## 自测题

1. .bss 段为什么不占二进制文件空间？
<details><summary>答案</summary>
.bss 存储未初始化的变量，值全为 0。程序加载时由 OS 零填充，不需要在二进制中存储零值。
</details>

2. 自定义段 `.section .mydata, "aw"` 的属性是什么？
<details><summary>答案</summary>
`a`=allocatable，`w`=writable。没有 `x`（不可执行）。可读可写但不可执行，适合存放数据。
</details>

3. 内核的 `__init` 段有什么特殊用途？
<details><summary>答案</summary>
存放只在初始化时调用的函数。内核启动完成后释放这部分内存（free_initmem），节省运行时内存。
</details>

## 参考与延伸

- 原书 §8.3
- [8.2 伪指令](02-directives.md)
- [Ch9 链接脚本段布局](../../chapter-09-linker-scripts/notes/section-0-本章完整概述.md)
