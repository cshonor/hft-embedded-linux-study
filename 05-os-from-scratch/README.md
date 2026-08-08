# 09 · 系统底层动手（自制 OS）

**文件夹 `09`** · [README](../README.md)

> **定位：** **知其所以然 → 动手造** — 用最小系统把进程、中断、内存映射、指令执行「摸一遍」。  
> **HFT 主线：** **直接 [01 MikanOS](./mikanos/)**；[02 30 天](./thirty-days-os/) 后置可选 — 见 **[HFT-AND-EMBEDDED-PRIORITY.md](./HFT-AND-EMBEDDED-PRIORITY.md)**  
> **前置：** `05` Gorman + **`06` TLPI**（syscall / 进程 / mmap 概念）  
> **执行顺序（HFT）：** C 扎实 → **`07/mikanos`** → `05` LKD / `14` DPDK → `15` HFT（`07/thirty-days-os` 可跳过）

## 学习链位置

```
05 Gorman → 06 TLPI
    ↓
07 自制 OS（本文件夹）
    ↓
09 PNP → 10 UNP → 11–13 网络栈
    ↓
14 SysPerf → 15 BPF（后置 · 有系统可观测后再开）
    ↓
16 HFT · 17 Rust
```

## 子模块

| 子文件夹 | 内容 |
|----------|------|
| **[mikanos](./mikanos/)** | 内田公太《从零自制操作系统》— **HFT 主线 OS 动手** · UEFI/64 位 |
| [thirty-days-os](./thirty-days-os/) | 川合秀实《30 天自制操作系统》— BIOS 软盘 · **通用启蒙 · HFT 可后置** |

> **两条路线：** **通用零基础** → 02 30 天启蒙 → 01 MikanOS；**HFT** → **C 后直接 01**，02 可不学。详 **[HFT-AND-EMBEDDED-PRIORITY.md](./HFT-AND-EMBEDDED-PRIORITY.md)**。

## 交叉阅读

- [HFT-AND-EMBEDDED-PRIORITY.md](./HFT-AND-EMBEDDED-PRIORITY.md) — C 优先 · 01 MikanOS 主线 · 03/04 后置
- [02-computer-systems](../02-computer-systems/) · [07-linux-kernel](../07-linux-kernel/) · [04-linux-userspace-api](../04-linux-userspace-api/)
- 下一步：[15-network-sockets/muduo-sockets](../15-network-sockets/muduo-sockets/)
