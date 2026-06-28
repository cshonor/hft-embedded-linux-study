# ULK 3rd · 全书目录

> **Understanding the Linux Kernel** · Bovet & Cesati · 基于 Linux **2.6**

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 — 调度/中断/内存/syscall |
| 🟡 | 选读 |
| ⚪ | 跳过 — VFS/块/Ext2 等非热路径 |

| 章 | 英文 | 笔记 | HFT | LKD 对照 |
|----|------|------|-----|----------|
| 1 | Introduction | [chapter-01-引言.md](./chapter-01-引言.md) · [notes](./chapter-01-引言/) | 🟡 | LKD Ch 1 |
| 2 | Memory Addressing | [chapter-02-内存寻址.md](./chapter-02-内存寻址.md) · [notes](./chapter-02-内存寻址/) | 🔴 | LKD Ch 11 概述 |
| 3 | Processes | [chapter-03-进程.md](./chapter-03-进程.md) · [notes](./chapter-03-进程/) | 🔴 | LKD Ch 3 |
| 4 | Interrupts and Exceptions | [chapter-04-中断与异常.md](./chapter-04-中断与异常.md) · [notes](./chapter-04-中断与异常/) | 🔴 | LKD Ch 7 |
| 5 | Kernel Synchronization | [chapter-05-内核同步.md](./chapter-05-内核同步.md) | 🔴 | LKD Ch 9–10 |
| 6 | Timing Measurements | [chapter-06-计时.md](./chapter-06-计时.md) | 🟡 | LKD Ch 11 |
| 7 | Process Scheduling | [chapter-07-进程调度.md](./chapter-07-进程调度.md) | 🔴 | LKD Ch 4 |
| 8 | Memory Management | [chapter-08-内存管理.md](./chapter-08-内存管理.md) | 🔴 | → Gorman |
| 9 | Process Address Space | [chapter-09-进程地址空间.md](./chapter-09-进程地址空间.md) | 🔴 | Gorman Ch 4 |
| 10 | System Calls | [chapter-10-系统调用.md](./chapter-10-系统调用.md) | 🔴 | LKD Ch 5 · TLPI Ch 2 |
| 11 | Signals | [chapter-11-信号.md](./chapter-11-信号.md) | 🟡 | TLPI Ch 20–21 |
| 12 | The Virtual Filesystem | [chapter-12-VFS.md](./chapter-12-VFS.md) | ⚪ | LKD Ch 13 |
| 13 | I/O Architecture and Device Drivers | [chapter-13-IO架构.md](./chapter-13-IO架构.md) | ⚪ | |
| 16 | Block Device Drivers | [chapter-14-块设备.md](./chapter-14-块设备.md) | ⚪ | |
| 16 | The Page Cache | [chapter-15-页缓存.md](./chapter-15-页缓存.md) | ⚪ | |
| 16 | Accessing Files | [chapter-16-文件访问.md](./chapter-16-文件访问.md) | ⚪ | |
| 17 | Page Frame Reclaiming | [chapter-17-页回收.md](./chapter-17-页回收.md) | 🟡 | Gorman Ch 10 |
| 18 | Ext2 and Ext3 | [chapter-18-ext2-ext3.md](./chapter-18-ext2-ext3.md) | ⚪ | |
| 19 | Process Communication | [chapter-19-进程通信.md](./chapter-19-进程通信.md) | 🟡 | TLPI IPC · 远期 IPC 模块 |
| 20 | Program Execution | [chapter-20-程序执行.md](./chapter-20-程序执行.md) | 🟡 | CSAPP Ch 8 |

## 附录

| 附录 | 笔记 | 标签 |
|------|------|------|
| A System Startup | [appendix-A-系统启动.md](./appendix-A-系统启动.md) | 🟡 |
| B Modules | [appendix-B-模块.md](./appendix-B-模块.md) | 🟡 |

> 各 `chapter-*.md` 随精读进度从占位扩写；结构对齐 [05-LKD](../05-Linux-Kernel-Development/00_Book_3rd_Notes/) 笔记风格。
