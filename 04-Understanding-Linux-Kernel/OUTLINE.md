# ULK 3rd · 全书目录

> **Understanding the Linux Kernel** · Bovet & Cesati · 基于 Linux **2.6**

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 — 调度/中断/内存/syscall |
| 🟡 | 选读 |
| ⚪ | 跳过 — VFS/块/Ext2 等非热路径 |

| 章 | 英文 | 笔记 | HFT | LKD 对照 |
|----|------|------|-----|----------|
| 1 | Introduction | [chapter-01-introduction.md](./chapter-01-introduction.md) · [notes](./chapter-01-introduction/) | 🟡 | LKD Ch 1 |
| 2 | Memory Addressing | [chapter-02-memory-addressing.md](./chapter-02-memory-addressing.md) · [notes](./chapter-02-memory-addressing/) | 🔴 | LKD Ch 11 概述 |
| 3 | Processes | [chapter-03-processes.md](./chapter-03-processes.md) · [notes](./chapter-03-processes/) | 🔴 | LKD Ch 3 |
| 4 | Interrupts and Exceptions | [chapter-04-interrupts-and-exceptions.md](./chapter-04-interrupts-and-exceptions.md) · [notes](./chapter-04-interrupts-and-exceptions/) | 🔴 | LKD Ch 7 |
| 5 | Kernel Synchronization | [chapter-05-kernel-synchronization.md](./chapter-05-kernel-synchronization.md) · [notes](./chapter-05-kernel-synchronization/) | 🔴 | LKD Ch 9–10 |
| 6 | Timing Measurements | [chapter-06-timing.md](./chapter-06-timing.md) · [notes](./chapter-06-timing/) | 🟡 | LKD Ch 11 |
| 7 | Process Scheduling | [chapter-07-process-scheduling.md](./chapter-07-process-scheduling.md) · [notes](./chapter-07-process-scheduling/) | 🔴 | LKD Ch 4 |
| 8 | Memory Management | [chapter-08-memory-management.md](./chapter-08-memory-management.md) · [notes](./chapter-08-memory-management/) | 🔴 | → Gorman |
| 9 | Process Address Space | [chapter-09-process-address-space.md](./chapter-09-process-address-space.md) · [notes](./chapter-09-process-address-space/) | 🔴 | Gorman Ch 4 |
| 10 | System Calls | [chapter-10-system-calls.md](./chapter-10-system-calls.md) · [notes](./chapter-10-system-calls/) | 🔴 | LKD Ch 5 · TLPI Ch 2 |
| 11 | Signals | [chapter-11-signals.md](./chapter-11-signals.md) · [notes](./chapter-11-signals/) | 🟡 | TLPI Ch 20–21 |
| 12 | The Virtual Filesystem | [chapter-12-VFS.md](./chapter-12-VFS.md) · [notes](./chapter-12-VFS/) | ⚪ | LKD Ch 13 |
| 13 | I/O Architecture and Device Drivers | [chapter-13-io-architecture.md](./chapter-13-io-architecture.md) · [notes](./chapter-13-io-architecture/) | ⚪ | |
| 16 | Block Device Drivers | [chapter-14-block-devices.md](./chapter-14-block-devices.md) · [notes](./chapter-14-block-devices/) | ⚪ | |
| 16 | The Page Cache | [chapter-15-page-cache.md](./chapter-15-page-cache.md) · [notes](./chapter-15-page-cache/) | ⚪ | |
| 16 | Accessing Files | [chapter-16-file-access.md](./chapter-16-file-access.md) · [notes](./chapter-16-file-access/) | ⚪ | |
| 17 | Page Frame Reclaiming | [chapter-17-page-reclaim.md](./chapter-17-page-reclaim.md) · [notes](./chapter-17-page-reclaim/) | 🟡 | Gorman Ch 10 |
| 18 | Ext2 and Ext3 | [chapter-18-ext2-ext3.md](./chapter-18-ext2-ext3.md) · [notes](./chapter-18-ext2-ext3/) | ⚪ | |
| 19 | Process Communication | [chapter-19-ipc.md](./chapter-19-ipc.md) · [notes](./chapter-19-ipc/) | 🟡 | TLPI IPC · 远期 IPC 模块 |
| 20 | Program Execution | [chapter-20-program-execution.md](./chapter-20-program-execution.md) · [notes](./chapter-20-program-execution/) | 🟡 | CSAPP Ch 8 |

## 附录

| 附录 | 笔记 | 标签 |
|------|------|------|
| A System Startup | [appendix-A-system-startup.md](./appendix-A-system-startup.md) | 🟡 |
| B Modules | [appendix-B-modules.md](./appendix-B-modules.md) | 🟡 |

> 各 `chapter-*.md` 随精读进度从占位扩写；结构对齐 [03-Linux-Kernel-Development](../03-Linux-Kernel-Development/00_Book_3rd_Notes/) 笔记风格。
