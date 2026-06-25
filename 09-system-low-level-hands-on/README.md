# 09 · 系统底层动手（自制 OS / CPU）

**文件夹 `09`** · [LEARNING-CHAIN](../LEARNING-CHAIN.md)

> **定位：** **知其所以然 → 动手造** — 用最小系统把进程、中断、内存映射、指令执行「摸一遍」。  
> **前置：** `06` Gorman + **`07` TLPI**（syscall / 进程 / mmap 概念）  
> **执行顺序：** `07` TLPI → **`09` 本模块** → `10` PNP → `11` UNP → `12`–`14` 网络 → `15` HFT

## 学习链位置

```
06 Gorman → 07 TLPI
    ↓
09 自制 OS / CPU（本文件夹）
    ↓
10 PNP → 11 UNP → 12–14 网络栈
    ↓
15 HFT · 16 Rust
```

## 子模块

| 子文件夹 | 内容 |
|----------|------|
| [01-30days-os](./01-30days-os/) | 川合秀实《30 天自制操作系统》— 32 位 BIOS 软盘 · 实模式 → 保护模式 |
| [02-mikan-os](./02-mikan-os/) | 内田公太《ゼロからの OS 自作入門》— **64 位 UEFI · MikanOS** · 分页/syscall |
| [03-30days-cpu](./03-30days-cpu/) | 矢泽久雄《30 天自制 CPU》— 门电路到能跑汇编的最小 CPU |

> 子目录用 `01` / `02` / `03` 编号。**推荐：** 01 启蒙 → 02 现代 OS；03 可与 01 并行或后补。

## 交叉阅读

- [07-TLPI](../07-The-Linux-Programming-Interface/) · [05-LKD](../05-Linux-Kernel-Development/)
- 下一步：[10-Practical-Network-Programming](../10-Practical-Network-Programming/)
