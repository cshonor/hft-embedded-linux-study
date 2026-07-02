# HFT 学习链路 · 从知其所以然到动手实现

> **文件夹 `00`–`17` = 物理编号 = 推荐阅读顺序**（2025-06：`08` C++ 纳入主线；`14`/`15` 性能书在 `13` DPDK 之后）。

```
知其所以然 → 系统纵深 → 底层+C++ → 网络 → 性能观测 → 工程
  01–02        03–06      07–08      09–13    14–15      16–17
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

08  C++ · [cpp-learning-notes](./08-cpp-learning-notes/)（索引 [GitHub 仓](https://github.com/cshonor/cpp-learning-notes)）
09  陈硕 PNP / muduo
10  UNP
01  CSAPP Ch10–11（网络篇，可与 09–10 交叉）
11  TCP/IP → 12 Rosen → 13 DPDK

14  SysPerf → 15 BPF

16  HFT Practice
17  Rust Guide

── 可选 · 嵌入式 Linux 支线（18 起，建议 03–06 后）──
18  ARM64 → 19  U-Boot/内核 → 20  驱动 → 21  DT → 22  实战 → 23  PID/飞控
```

**HFT 最短路径：** `01` → `02` → `03`–`06` → `07/01` MikanOS → `08` C++ → `13` DPDK → `14`–`15` → `16` HFT

**嵌入式支线：** `18 → … → 23`（与 HFT **并行或后置**）

---

## 为何这样排？

| 调整 | 理由 |
|------|------|
| **`08` C++ 在 PNP 前** | Modern C++ / 并发 — muduo、HFT 引擎前置 |
| **`14`/`15` 在 `13` DPDK 之后** | 有内核、网络、旁路可观测后再读 Gregg 双书 |
| **`03`–`06` 内核 + TLPI** | syscall / VM 图景先于 OS 动手与网络 |
| **`07/01` MikanOS 优先于 `07/02` 30 天** | HFT 走 UEFI/64 位 |

---

## 文件夹 ↔ 阶段

| 文件夹 | 模块 |
|--------|------|
| **03–06** | LKD · ULK · Gorman · TLPI |
| **07/01** | [MikanOS](./07-system-low-level-hands-on/01-mikan-os/) |
| **07/02** | [30 天 OS](./07-system-low-level-hands-on/02-30days-os/) — 可选 |
| **08** | [C++ 学习索引](./08-cpp-learning-notes/) |
| **09–13** | PNP · UNP · TCP/IP · Rosen · **DPDK** |
| **14–15** | SysPerf · BPF |
| **16–17** | HFT · Rust |

---

## 内核段衔接

```
03 LKD → 04 ULK → 05 Gorman → 06 TLPI
    ↓
07/01 MikanOS
    ↓
08 C++（Effective Modern C++）
    ↓
09–13 网络栈（含 13 DPDK）
    ↓
14 SysPerf → 15 BPF
    ↓
16 HFT
```

→ [07 HFT 学习主次](./07-system-low-level-hands-on/HFT-AND-EMBEDDED-PRIORITY.md) · [06 TLPI OUTLINE](./06-The-Linux-Programming-Interface/OUTLINE.md)

---

## 相关文档

- [READING-LIST.md](./READING-LIST.md) · [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) · [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

**HFT 主线执行序号：** `00 → 01 → 02 → 03 → 04 → 05 → 06 → 07/01 → 08 → 09 → 01网络 → 10 → 11 → 12 → 13 → 14 → 15 → 16 → 17`

> **C++：** [08-cpp-learning-notes/](./08-cpp-learning-notes/) — **07 之后、09 PNP 之前** 至少读完 *Effective Modern C++*。
