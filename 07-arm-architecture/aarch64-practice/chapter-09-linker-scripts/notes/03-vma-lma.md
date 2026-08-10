# 9.3 VMA vs LMA

> 来源：§9.3 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

VMA（虚拟内存地址）和 LMA（加载内存地址）的区别。

## 核心要点

| 概念 | 含义 |
|------|------|
| VMA | 运行时地址（CPU 执行时看到的地址） |
| LMA | 加载地址（程序被加载到的物理地址） |

```ld
.text 0x80000 : AT(0x40000) { *(.text) }
```
VMA=0x80000, LMA=0x40000

## HFT 关联

- 内核被加载到物理低位（LMA），运行在高位虚拟地址（VMA）
- BenOS 启动时 VMA=LMA（恒等映射），开 MMU 后切换

## 自测题

1. VMA 和 LMA 什么时候不同？
<details><summary>答案</summary>
内核启动（LMA=物理低位，VMA=高位虚拟地址），嵌入式从 Flash 加载到 RAM（LMA=Flash，VMA=RAM）。
</details>

2. 内核启动为什么需要从 LMA 拷贝 .data 到 VMA？
<details><summary>答案</summary>
开 MMU 后 CPU 用 VMA 访问数据，但数据还在 LMA。需拷贝否则 page fault。
</details>

3. `AT()` 的作用？
<details><summary>答案</summary>
`AT(addr)` 指定段的 LMA（加载地址）。不指定时 LMA=VMA。
</details>

## 参考与延伸

- 原书 §9.3
- [Ch14 MMU 地址映射](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
