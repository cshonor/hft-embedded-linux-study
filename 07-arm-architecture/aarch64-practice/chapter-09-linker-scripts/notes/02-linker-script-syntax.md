# 9.2 链接脚本语法

> 来源：§9.2 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

链接脚本的基本语法：ENTRY、SECTIONS、位置计数器、段定义。

## 核心要点

```ld
ENTRY(_start)
SECTIONS {
    . = 0x400000;
    .text : { *(.text) }
    . = ALIGN(8);
    .data : { *(.data) }
    .bss : { *(.bss) }
}
```

- `.` 是位置计数器（当前地址）
- `*(.text)` = 匹配所有输入文件的 .text 段

## HFT 关联

- 代码段放在特定地址 → 配合 MMU 映射优化
- 数据段对齐 → cache line 对齐减少 false sharing

## 自测题

1. 链接脚本中 `.` 代表什么？
<details><summary>答案</summary>
位置计数器，表示当前的虚拟地址（VMA）。链接器从起始地址开始，每放入一段数据 `.` 就增加相应大小。
</details>

2. `*(.text)` 和 `obj.o(.text)` 的区别？
<details><summary>答案</summary>
`*(.text)` 匹配所有输入文件的 .text 段。`obj.o(.text)` 只匹配 obj.o 文件的 .text 段。
</details>

3. ENTRY(_start) 的作用？
<details><summary>答案</summary>
指定程序入口地址为 `_start` 符号。程序加载后 CPU 从此地址开始执行。
</details>

## 参考与延伸

- 原书 §9.2
- [9.3 VMA vs LMA](03-vma-lma.md)
