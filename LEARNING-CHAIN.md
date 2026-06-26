# HFT 学习链路 · 从知其所以然到动手实现

> **文件夹 `00`–`16` 封顶不变**；**读序** 见下（与目录编号不一致处已标明）。

```
知其所以然  →  知其然  →  工具落地  →  系统纵深  →  网络实战  →  工程实现
  01+04         02          03          05–08         09–14           15–16
```

---

## 一眼版 · 推荐执行顺序

```
00  Harris
01  CSAPP
04  Hennessy              ← 原 02 位：紧接 01，再进性能篇
02  SysPerf → 03  BPF      ← 原 02–03 顺延

05  LKD → 08 ULK           ← 内核概念 → 立刻下潜源码
06  Gorman → 07  TLPI     ← 内存专精 → 用户态 syscall

09  自制 OS / CPU
10  陈硕 PNP / muduo
11  UNP
01  CSAPP Ch10–11（网络篇，可与 10–11 交叉）
12  TCP/IP → 13  Rosen → 14  DPDK

15  HFT Practice
16  Rust Guide
```

**最短四步：** `01` → `04` → `02` → `03` → `15`/`16`（业务向加 `00`）

---

## 为何这样排？

| 调整 | 理由 |
|------|------|
| **`04` 提到 `02` 原位** | CSAPP 机器级程序刚建立直觉 → Hennessy 补 cache/流水线/一致性 → 再读 SysPerf **才有量化靶心** |
| **`08` 紧接 `05`** | LKD 画内核地图 → ULK 立刻对照 **数据结构/路径** → 再 Gorman 专精 VM，比把 ULK 拖到 TLPI 后更顺 |

---

## 文件夹 ↔ 阶段

| 文件夹 | 模块 | 阶段 |
|--------|------|------|
| **07** | [TLPI](./07-The-Linux-Programming-Interface/) | Linux 用户态 syscall |
| **08** | [ULK](./08-Understanding-Linux-Kernel/) | Linux 内核实现（**读序紧接 05**） |
| **09** | [自制 OS/CPU](./09-system-low-level-hands-on/) | 底层动手（30天 / MikanOS / CPU） |
| **10–14** | PNP / UNP / TCP/IP / Rosen / DPDK | 网络纵深 |
| **15–16** | HFT / Rust | 工程实现 |

---

## 内核段衔接

```
05 LKD（内核里有什么）
    ↓
08 ULK（代码里长什么样）
    ↓
06 Gorman（VM 深度）
    ↓
07 TLPI（用户态怎么调 epoll/mmap）
    ↓
09 自制 OS（从零写一遍启动/中断/分页）
```

→ [07 OUTLINE](./07-The-Linux-Programming-Interface/OUTLINE.md)

---

## 相关文档

- [READING-LIST.md](./READING-LIST.md) · [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) · [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

**执行序号：** `00 → 01 → 04 → 02 → 03 → 05 → 08 → 06 → 07 → 09 → 10 → 11 → 01网络章 → 12 → 13 → 14 → 15 → 16`

> **远期（未建目录）：** 裸金属 HFT 生产部署 · IPC 进程间通信架构 — 结业阶段补 [15-HFT](./15-HFT-Low-Latency-Practice/)。
