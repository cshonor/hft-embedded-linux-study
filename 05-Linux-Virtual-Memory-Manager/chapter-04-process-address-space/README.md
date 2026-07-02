# Ch 4 进程地址空间 · Process Address Space

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读**（HFT：**mmap / mlock / 缺页路径** 与热路径强相关，建议精读 §3–§5）

虚拟内存的核心优势之一：**每个进程拥有独立的虚拟地址空间**。本章讲 Linux 如何 **描述、管理** 这些空间 — 从 **`mm_struct` / VMA** 到 **`mmap`/`munmap`/`mlock`**，再到 **缺页异常** 如何把「预留的 VA」变成「真正的物理页」。

> **时代说明：** 原书以 **32 位 x86（3G+1G）**、**2.6** 为主。 **x86_64** 为用户态提供巨大 canonical 地址空间，**用户/内核划分** 仍靠 **`PAGE_OFFSET`** 思路，但不再是「3GiB 用户 + 1GiB 内核」这一固定数字。

---

## 内核 vs 用户：两种「分配」哲学

| | **内核空间** | **用户空间** |
|---|-------------|-------------|
| **分配语义** | 请求后 **尽快** 得到物理页（或 kmalloc/vmalloc 路径） | 多数时候只是在 **线性地址里预留** VA 范围 |
| **何时有物理页** | 分配路径上 **立即**（或明确失败） | 往往 **首次访问**（读/写）才通过 **缺页异常** 真正分配 |
| **可见性** | 内核映射 **全局一致**（各进程共享内核页表上半） | **每进程独立** PGD / VMA，上下文切换时 **mm 可能变** |

**HFT 结论：** 热路径上 **第一次 touch 某页** = 可能 **page fault + 分配 + 清零/读盘** — 延迟尖刺。所以常用 **`mmap` + 预 touch**、**`MAP_POPULATE`**、**`mlock`**、**大页** 把 fault 挡在 **启动/预热阶段**。

→ 用户态 API：[06-The-Linux-Programming-Interface](../06-The-Linux-Programming-Interface/) · 概念：[01-CSAPP Ch9](../01-CSAPP-3rd/chapter-09-virtual-memory/) · 内核对照：[03-Linux-Kernel-Development Ch15](../03-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-15-process-address-space/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 线性地址空间 | [notes/section-1-线性地址空间.md](./notes/section-1-线性地址空间.md) |
| 2. 进程地址空间描述符 | [notes/section-2-进程地址空间描述符.md](./notes/section-2-进程地址空间描述符.md) |
| 3. 内存区域 | [notes/section-3-内存区域.md](./notes/section-3-内存区域.md) |
| 4. 异常处理与缺页异常 | [notes/section-4-异常处理与缺页异常.md](./notes/section-4-异常处理与缺页异常.md) |
| 5. 内核与用户空间的数据拷贝 | [notes/section-5-内核与用户空间的数据拷贝.md](./notes/section-5-内核与用户空间的数据拷贝.md) |
| 6. Linux 2.6 内核的新变化 | [notes/section-6-Linux-2.6-内核的新变化.md](./notes/section-6-Linux-2.6-内核的新变化.md) |

---

## 相关章节

- 上一章：[../chapter-03-page-table-management/](../chapter-03-page-table-management/)
- 下一章：[../chapter-05-boot-memory-allocator/](../chapter-05-boot-memory-allocator/)
- 附录 D：[../../appendix-D-进程地址空间.md](../../appendix-D-进程地址空间.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
