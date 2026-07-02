# 09 · 系统底层动手（自制 OS）

**文件夹 `09`** · [LEARNING-CHAIN](../LEARNING-CHAIN.md)

> **定位：** **知其所以然 → 动手造** — 用最小系统把进程、中断、内存映射、指令执行「摸一遍」。  
> **HFT 主线：** **直接 [01 MikanOS](./01-mikan-os/)**；[02 30 天](./02-30days-os/) 后置可选 — 见 **[HFT-AND-EMBEDDED-PRIORITY.md](./HFT-AND-EMBEDDED-PRIORITY.md)**  
> **前置：** `05` Gorman + **`06` TLPI**（syscall / 进程 / mmap 概念）  
> **执行顺序（HFT）：** C 扎实 → **`07/01-mikan-os`** → `05` LKD / `14` DPDK → `15` HFT（`07/02-30days-os` 可跳过）

## 学习链位置

```
05 Gorman → 06 TLPI
    ↓
07 自制 OS（本文件夹）
    ↓
08 PNP → 09 UNP → 10–12 网络栈
    ↓
13 SysPerf → 14 BPF（后置 · 有系统可观测后再开）
    ↓
15 HFT · 16 Rust
```

## 子模块

| 子文件夹 | 内容 |
|----------|------|
| **[01-mikan-os](./01-mikan-os/)** | 内田公太《从零自制操作系统》— **HFT 主线 OS 动手** · UEFI/64 位 |
| [02-30days-os](./02-30days-os/) | 川合秀实《30 天自制操作系统》— BIOS 软盘 · **通用启蒙 · HFT 可后置** |

> **两条路线：** **通用零基础** → 02 30 天启蒙 → 01 MikanOS；**HFT** → **C 后直接 01**，02 可不学。详 **[HFT-AND-EMBEDDED-PRIORITY.md](./HFT-AND-EMBEDDED-PRIORITY.md)**。

## 交叉阅读

- [HFT-AND-EMBEDDED-PRIORITY.md](./HFT-AND-EMBEDDED-PRIORITY.md) — C 优先 · 01 MikanOS 主线 · 03/04 后置
- [01-CSAPP-3rd](../01-CSAPP-3rd/) · [03-Linux-Kernel-Development](../03-Linux-Kernel-Development/) · [06-The-Linux-Programming-Interface](../06-The-Linux-Programming-Interface/)
- 下一步：[08-Practical-Network-Programming](../08-Practical-Network-Programming/)
