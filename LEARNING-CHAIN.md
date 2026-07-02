# HFT 学习链路 · 从知其所以然到动手实现

> **文件夹 `00`–`16` = 物理编号 = 推荐阅读顺序**（2025-06：`13`/`14` 性能书放在 `12` DPDK 之后）。

```
知其所以然 → 系统纵深 → 底层动手 → 网络 → 性能观测 → 工程
  01–02        03–06       07         08–12    13–14      15–16
```

---

## 一眼版 · 执行顺序

```
00  Harris
01  CSAPP
02  Hennessy

03  LKD → 04 ULK → 05 Gorman → 06 TLPI

07  自制 OS
    └─ 01-mikan-os（HFT 主线）· 02-30days-os（可选启蒙）

17  C++ · [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)
08  陈硕 PNP / muduo
09  UNP
01  CSAPP Ch10–11（网络篇，可与 08–09 交叉）
10  TCP/IP → 11 Rosen → 12 DPDK

13  SysPerf → 14 BPF

15  HFT Practice
16  Rust Guide

── 可选 · 嵌入式 Linux 支线（18 起，建议 03–06 后）──
18  ARM64 → 19  U-Boot/内核 → 20  驱动 → 21  DT → 22  实战 → 23  PID/飞控
```

**HFT 最短路径：** `01` → `02` → `03`–`06` → `07/01` MikanOS → `12` DPDK → `13`–`14` → `15` HFT

**嵌入式支线：** `18 → … → 23`（与 HFT **并行或后置**）

---

## 为何这样排？

| 调整 | 理由 |
|------|------|
| **`02` Hennessy 紧接 `01` CSAPP** | 机器级程序 → 体系结构 → 再读内核 |
| **`03`–`06` 内核 + TLPI 提前** | 原 05–08 整体上移；先建立 syscall/VM 图景 |
| **`13`/`14` 在 `12` DPDK 之后** | 有内核、网络、用户态旁路可观测后再读 Gregg 双书 |
| **`07/01` MikanOS 在 `07/02` 30 天前** | HFT 走 UEFI/64 位；30 天 BIOS 启蒙可选 |
| **`04` ULK 紧接 `03` LKD** | 内核地图 → 立刻下潜源码 |

---

## 文件夹 ↔ 阶段

| 文件夹 | 模块 |
|--------|------|
| **03** | [LKD](./03-Linux-Kernel-Development/) |
| **04** | [ULK](./04-Understanding-Linux-Kernel/) |
| **05** | [Gorman](./05-Linux-Virtual-Memory-Manager/) |
| **06** | [TLPI](./06-The-Linux-Programming-Interface/) |
| **07/01** | [MikanOS](./07-system-low-level-hands-on/01-mikan-os/) — HFT OS 主线 |
| **07/02** | [30 天 OS](./07-system-low-level-hands-on/02-30days-os/) — 可选 |
| **08–12** | PNP / UNP / TCP/IP / Rosen / **DPDK** |
| **13** | [SysPerf](./13-Systems-Performance-2nd/) |
| **14** | [BPF Tools](./14-BPF-Performance-Tools/) |
| **15–16** | HFT / Rust |

---

## 内核段衔接

```
03 LKD（内核里有什么）
    ↓
04 ULK（代码里长什么样）
    ↓
05 Gorman（VM 深度）
    ↓
06 TLPI（用户态 epoll/mmap）
    ↓
07/01 MikanOS（UEFI/64 位 · 从零搭机制）
    ↓
08–12 网络栈（含 12 DPDK）
    ↓
13 SysPerf → 14 BPF
    ↓
15 HFT
```

→ [07 HFT 学习主次](./07-system-low-level-hands-on/HFT-AND-EMBEDDED-PRIORITY.md) · [06 TLPI OUTLINE](./06-The-Linux-Programming-Interface/OUTLINE.md)

---

## 相关文档

- [READING-LIST.md](./READING-LIST.md) · [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) · [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

**HFT 主线执行序号：** `00 → 01 → 02 → 03 → 04 → 05 → 06 → 07/01 → 17 → 08 → 09 → 01网络 → 10 → 11 → 12 → 13 → 14 → 15 → 16`

> **C++ 外部仓：** [17-cpp-learning-notes/](./17-cpp-learning-notes/) — **07 之后、08 PNP 之前** 至少读完 *Effective Modern C++*。

> **重编号脚本：** [scripts/renumber-modules-03-14-perf-defer.py](./scripts/renumber-modules-03-14-perf-defer.py)
