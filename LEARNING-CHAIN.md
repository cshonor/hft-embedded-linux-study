# HFT 学习链路 · 从知其所以然到动手实现

> **仓库：** [hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study) · **文件夹 `00`–`18` 主线 + 嵌入式 `19`–`24` = 物理编号 = 推荐阅读顺序**

```
知其所以然 → 系统纵深 → 底层+C++ → 网络 → 性能观测 → 工程
  01–03        04–07       08–09      10–14    15–16      17–18
  （02=C）
```

---

## 一眼版 · 执行顺序

```
00  Harris
01  CSAPP
02  C 语言 · [c-programming](./02-c-programming/)（笔记 → [外部 11-C](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C)）
03  Hennessy

04  LKD → 05 ULK → 06 Gorman → 07 TLPI

08  自制 OS
    └─ 01-mikan-os（HFT 主线）· 02-30days-os（可选）

09  C++ · [cpp-learning-notes](./09-cpp-learning-notes/)
10  陈硕 PNP / muduo
11  UNP
01  CSAPP Ch10–11（网络篇，可与 10–11 交叉）
12  TCP/IP → 13 Rosen → 14 DPDK

15  SysPerf → 16 BPF

17  HFT Practice
18  Rust Guide

── 嵌入式 Linux 支线（19 起，建议 04–07 后 · 全外文 19–21）──
19  ARM Assembly Language（汇编前置）
20  Mastering Embedded Linux Programming (3rd ed) → Embedded Linux Primer
21  LDD3 → Linux Device Driver Development
22  Device Tree → 23 实战 → 24  PID/飞控
    （22 = 内核官方 DT Usage 文档，无单册书）
```

**HFT 最短路径：** `01` → **`02` C** → `03` → `04`–`07` → `08/01` MikanOS → `09` C++ → `14` DPDK → `15`–`16` → `17` HFT

---

## 为何 02 C 在 CSAPP 与 Hennessy 之间？

| 步骤 | 作用 |
|------|------|
| **01 CSAPP** | 硬件、机器级程序、内存层次 **整体图景** |
| **02 C** | **系统级 C** — 底层开发的「通用母语」；指针、内存、链接；能读会写内核/驱动风格代码 |
| **03 Hennessy** | CPU/缓存/ILP **量化** — 读 C/汇编时知道「慢在哪」；**可与 02 同步做小实验**（如 ARM 裸机验证 EL 切换） |
| **04–07** | 内核与 syscall — 主体是 **C** |
| **09 C++** | 在 C 过关后再加 RAII/Modern C++（muduo/HFT） |
| **19–24 嵌入式** | ARM · 驱动 · 飞控 — **同样以 C 与硬件/内核交互**，02 过关后支线不重学语法 |

**理论 + 实践：** CSAPP 给图景 → **02 用 C 写程序** → 03 学体系结构时已有语言抓手 → 04+ 内核、08 MikanOS、19+ 嵌入式/HFT 底层都能接上。

---

## 文件夹 ↔ 阶段

| 文件夹 | 模块 |
|--------|------|
| **01** | CSAPP |
| **02** | [C 语言](./02-c-programming/) |
| **03** | Hennessy |
| **04–07** | LKD · ULK · Gorman · TLPI |
| **08/01** | MikanOS |
| **09** | C++ 索引 |
| **10–14** | 网络 + DPDK |
| **15–16** | SysPerf · BPF |
| **17–18** | HFT · Rust |

---

## 内核段衔接

```
01 CSAPP → 02 C → 03 Hennessy
    ↓
04 LKD → 05 ULK → 06 Gorman → 07 TLPI
    ↓
08/01 MikanOS → 09 C++ → 10–14 网络/DPDK
    ↓
15 SysPerf → 16 BPF → 17 HFT
```

→ [08 HFT 主次](./08-system-low-level-hands-on/HFT-AND-EMBEDDED-PRIORITY.md) · [02 C OUTLINE](./02-c-programming/OUTLINE.md) · [外部 C 笔记](https://github.com/cshonor/cpp-learning-notes/tree/main/11-Linux-Kernel-DPDK-Network-C) · [GitHub 仓库](https://github.com/cshonor/hft-embedded-linux-study)

---

**HFT 主线执行序号：** `00 → 01 → 02 → 03 → 04 → 05 → 06 → 07 → 08/01 → 09 → 10 → 01网络 → 11 → 12 → 13 → 14 → 15 → 16 → 17 → 18`

> **C++：** [09-cpp-learning-notes/](./09-cpp-learning-notes/) — **08/07 之后、10 PNP 之前** · *Effective Modern C++*
