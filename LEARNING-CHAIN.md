# HFT 学习链路 · 从知其所以然到动手实现

> **文件夹 `00`–`16` = 推荐阅读顺序**（2025-06 物理重编号对齐）。

```
知其所以然  →  知其然  →  工具落地  →  系统纵深  →  网络实战  →  工程实现
  01–02         03          04          05–08         09–14           15–16
```

---

## 一眼版 · 执行顺序

```
00  Harris
01  CSAPP
02  Hennessy
03  SysPerf → 04  BPF

05  LKD → 06 ULK → 07 Gorman → 08 TLPI

09  自制 OS / CPU
10  陈硕 PNP / muduo
11  UNP
01  CSAPP Ch10–11（网络篇，可与 10–11 交叉）
12  TCP/IP → 13  Rosen → 14  DPDK

15  HFT Practice
16  Rust Guide
```

**最短四步：** `01` → `02` → `03` → `04` → `15`/`16`（业务向加 `00`）

---

## 为何这样排？

| 调整 | 理由 |
|------|------|
| **`02` Hennessy 紧接 `01` CSAPP** | 机器级程序 → 体系结构理论 → 再读 SysPerf 才有量化靶心 |
| **`06` ULK 紧接 `05` LKD** | 内核地图 → 立刻下潜源码 → `07` Gorman 专精 VM |

---

## 文件夹 ↔ 阶段

| 文件夹 | 模块 | 阶段 |
|--------|------|------|
| **08** | [TLPI](./08-The-Linux-Programming-Interface/) | Linux 用户态 syscall |
| **06** | [ULK](./06-Understanding-Linux-Kernel/) | Linux 内核实现（紧接 05） |
| **09** | [自制 OS/CPU](./09-system-low-level-hands-on/) | 底层动手 |
| **10–14** | PNP / UNP / TCP/IP / Rosen / DPDK | 网络纵深 |
| **15–16** | HFT / Rust | 工程实现 |

---

## 内核段衔接

```
05 LKD（内核里有什么）
    ↓
06 ULK（代码里长什么样）
    ↓
07 Gorman（VM 深度）
    ↓
08 TLPI（用户态 epoll/mmap）
    ↓
09 自制 OS（从零写启动/中断/分页）
```

→ [08 TLPI OUTLINE](./08-The-Linux-Programming-Interface/OUTLINE.md)

---

## 相关文档

- [READING-LIST.md](./READING-LIST.md) · [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) · [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

**执行序号：** `00 → 01 → 02 → 03 → 04 → 05 → 06 → 07 → 08 → 09 → 10 → 11 → 01网络章 → 12 → 13 → 14 → 15 → 16`

> **重编号脚本：** [scripts/renumber-modules-02-08-align.py](./scripts/renumber-modules-02-08-align.py)
