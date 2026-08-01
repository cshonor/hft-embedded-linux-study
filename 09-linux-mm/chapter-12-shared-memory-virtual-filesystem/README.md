# Ch 12 共享内存虚拟文件系统 · Shared Memory Virtual Filesystem

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读**（HFT：**跨进程共享行情 / ring buffer** 常走 **`mmap(MAP_SHARED)`** — 文件映射或 **`/dev/shm` tmpfs**；本章解释 **匿名共享** 如何 **伪装成文件页** 走统一 VM 路径）

## 问题：匿名共享没有「真文件」

| 场景 | 后备存储 |
|------|----------|
| **`mmap` 真实文件 + `MAP_SHARED`** | 磁盘文件 — 复用 **page cache + address_space**（Ch 10） |
| **匿名 `MAP_SHARED`**（无 fd） | **无物理文件** — 不能直接用现有 **文件页** 管理接口 |
| **System V `shmget()` / `shmat()`** | 同样 **无磁盘文件** |

内核解法：在 **RAM 里造一个虚拟文件系统** — 给匿名共享页 **伪装的 file backing**，让 **缺页、swap、LRU** 等 **按文件映射同一套逻辑** 处理。

→ [Ch 4 mmap](../../chapter-04-process-address-space/) · [Ch 11 共享页 swap](../../chapter-11-swap-management/)

> **源码：** 现代主线 [`mm/shmem.c`](https://elixir.bootlin.com/linux/latest/source/mm/shmem.c)（tmpfs/shmem 合一）· 挂载点 **`/dev/shm`**、**`/tmp`**（tmpfs）。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. shm 与 tmpfs | [notes/section-1-shm-与-tmpfs.md](./notes/section-1-shm-与-tmpfs.md) |
| 2. 初始化与文件操作 | [notes/section-2-初始化与文件操作.md](./notes/section-2-初始化与文件操作.md) |
| 3. 缺页与 Swap（共享 shmem 页） | [notes/section-3-缺页与-Swap（共享-shmem-页）.md](./notes/section-3-缺页与-Swap（共享-shmem-页）.md) |
| 4. 建立共享区与 System V IPC | [notes/section-4-建立共享区与-System-V-IPC.md](./notes/section-4-建立共享区与-System-V-IPC.md) |
| 5. 2.6 内核的新变化 | [notes/section-5-2.6-内核的新变化.md](./notes/section-5-2.6-内核的新变化.md) |

---

## 相关章节

- 上一章：[../chapter-11-swap-management/](../chapter-11-swap-management/)
- 下一章：[../chapter-13-out-of-memory-management/](../chapter-13-out-of-memory-management/)
- 附录 L：[../../appendix-L-共享内存虚拟文件系统.md](../../appendix-L-共享内存虚拟文件系统.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
