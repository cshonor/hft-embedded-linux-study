# HFT 学习链路 · 从知其所以然到动手实现

> **文件夹 `00`–`16`。** **执行序号**见文末。

```
知其所以然  →  知其然  →  工具落地  →  系统纵深  →  网络实战  →  工程实现
  01–04         02          03          05–08         09–14           15–16
```

---

## 一眼版 · 推荐执行顺序

```
00  Harris
01  CSAPP + 04 Hennessy
02  SysPerf → 03  BPF
05  LKD → 08 ULK（实现细节）→ 06  Gorman

07  TLPI                 ← Linux 用户态：epoll / mmap / 线程
09  自制 OS / CPU         ← 01 30天 → 02 MikanOS（UEFI/分页）
10  陈硕 PNP / muduo
11  UNP
01  CSAPP Ch10–11
12  TCP/IP → 13  Rosen → 14  DPDK

15  HFT Practice
16  Rust Guide
```

**最短四步：** `01` → `02` → `03` → `15`/`16`

---

## 文件夹 ↔ 阶段

| 文件夹 | 模块 | 阶段 |
|--------|------|------|
| **07** | [TLPI](./07-The-Linux-Programming-Interface/) | Linux 用户态 syscall |
| **08** | [ULK](./08-Understanding-Linux-Kernel/) | Linux 内核实现细节（LKD↔Gorman 桥梁） |
| **09** | [自制 OS/CPU](./09-system-low-level-hands-on/) | 底层动手（30天 / MikanOS / CPU） |
| **10–14** | PNP / UNP / TCP/IP / Rosen / DPDK | 网络纵深 |
| **15–16** | HFT / Rust | 工程实现 |

---

## 07 · TLPI 在链上的位置

```
05 LKD（内核里是什么）  →  08 ULK（实现/数据结构）  →  07 TLPI（用户态怎么调）
06 Gorman（VM 深度）    →  07 TLPI（mmap/mlock）
07 TLPI（epoll）        →  10 PNP（写网络服务）
07 TLPI（socket 基础）  →  11 UNP（API 系统化）
```

→ [OUTLINE.md](./07-The-Linux-Programming-Interface/OUTLINE.md)

---

## 相关文档

- [READING-LIST.md](./READING-LIST.md) · [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) · [CROSS-MODULE-GUIDE.md](./CROSS-MODULE-GUIDE.md)

**执行序号：** `00 → 01(+04) → 02 → 03 → 05 → 08 → 06 → 07 → 09 → 10 → 11 → 01网络章 → 12 → 13 → 14 → 15 → 16`

> **远期（未建目录）：** 裸金属 HFT 生产部署 · IPC 进程间通信架构 — 结业阶段补 [15-HFT](./15-HFT-Low-Latency-Practice/)。
