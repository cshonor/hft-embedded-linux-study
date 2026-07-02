# The Linux Programming Interface — Michael Kerrisk（TLPI）

**定位：** Linux **用户态系统编程** 全书 — syscall、进程、内存、线程、信号、**epoll**；衔接 `01` CSAPP / `05` LKD 与 `09` PNP / `10` UNP。

**文件夹 `07`** · [返回总清单](../READING-LIST.md#7-the-linux-programming-interface--michael-kerrisk) · [LEARNING-CHAIN](../LEARNING-CHAIN.md)

> **推荐顺序：** `05` LKD + `06` Gorman 概念打底 → **本目录** → `08` 自制 OS/CPU → `09` PNP → `10` UNP  
> **与 UNP 分工：** TLPI = **Linux 特有** syscall / epoll / `timerfd` / `eventfd`；UNP = Stevens **网络 API** 系统化。

## 子目录

| 路径 | 内容 |
|------|------|
| [OUTLINE.md](./OUTLINE.md) | 全书章节 · HFT 🔴/🟡/⚪ 裁剪 |
| `chapter-*/` | 每章 `notes.md` + 按需 `code/` |

## HFT 为什么读 TLPI

| 主题 | TLPI 章节 | HFT 价值 |
|------|-----------|----------|
| **epoll / 多路复用** | Ch 63 | 行情多路接入 — 比 UNP 更贴 Linux 实现 |
| **mmap / 大页** | Ch 49–50 | 订单簿共享内存、减少拷贝 |
| **线程 / 锁** | Ch 29–33 | 绑核、优先级、与无锁设计衔接 |
| **进程 / 信号** | Ch 20–28 | 热路径信号屏蔽、`SIGIO` 背景 |
| **Socket 基础** | Ch 56–61 | 进 UNP 前的 Linux socket 语义 |
| **调度 / 优先级** | Ch 35–37 | `SCHED_FIFO`、nice 与延迟 |

## 与仓库其他模块

| 模块 | 关系 |
|------|------|
| [01-CSAPP Ch8/10](../01-CSAPP-3rd/chapter-08-exceptional-control-flow/) | 进程、信号、I/O 程序员视角 |
| [03-Linux-Kernel-Development](../03-Linux-Kernel-Development/) | syscall **在内核里** 如何实现 |
| [05-Linux-Virtual-Memory-Manager](../05-Linux-Virtual-Memory-Manager/) | mmap 背后 VM |
| [13-Systems-Performance](../13-Systems-Performance-2nd/) | 量 epoll 延迟、off-CPU |
| [09-PNP](../08-Practical-Network-Programming/) | 实验层网络服务 |
| [10-UNP](../09-UNP-Vol1/) | 网络 API 纵深 |
| [14-HFT](../15-HFT-Low-Latency-Practice/) | 工程落地 |

## 版本说明

本索引默认 **TLPI 第 2 版（2010）** 章节号；与第 1 版大体一致，网络/epoll 章在 2nd 更完整。
