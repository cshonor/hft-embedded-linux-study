# 9.6 Linux 内核链接脚本分析

> 来源：§9.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

分析 Linux 内核链接脚本 `arch/arm64/kernel/vmlinux.lds`。

## 核心要点

- 内核 VMA = 0xFFFF000000000000（高位虚拟地址）
- `_text`/`_etext` 标记代码段起止
- `__init_begin`/`__init_end` 标记 init 段（启动后释放）
- `.percpu` 段 → 每核独立数据

## HFT 关联

- percpu 段 → 每核独立数据，避免跨核共享（HFT 低延迟关键）

## 自测题

1. 为什么内核 VMA 设为 0xFFFF000000000000？
<details><summary>答案</summary>
ARMv8 VA bit[63:48] 全 1 走 TTBR1（内核空间）。用户和内核地址空间隔离。
</details>

2. `__init_begin` 和 `__init_end` 有什么用？
<details><summary>答案</summary>
标记 init 段起止。内核启动后 free_initmem() 释放这部分内存。
</details>

3. percpu 段如何实现每核独立数据？
<details><summary>答案</summary>
链接脚本把 percpu 变量放在一起，内核为每 CPU 创建副本。运行时通过 percpu 偏移（TPIDR_EL1）计算当前 CPU 的副本地址。
</details>

## 参考与延伸

- 原书 §9.6
- [Ch14 MMU 虚拟地址](../../chapter-14-memory-management/notes/section-0-本章完整概述.md)
