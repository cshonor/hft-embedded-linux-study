# Ch 6 物理页分配 · Physical Page Allocation

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读**（HFT：**NUMA 本地页、GFP、per-CPU pageset、zone 水位** 与延迟/抖动强相关 — 建议至少精读 §2、§4、§6）

本章讲 Linux **运行时** 如何 **分配 / 释放物理页框** — 核心算法是 **二进制伙伴分配器 (Binary Buddy Allocator)**：**2 的幂次连续页块** + **拆分 / 合并**，追求 **极高分配速度**。

> **源码入口：** [`mm/page_alloc.c`](https://elixir.bootlin.com/linux/latest/source/mm/page_alloc.c)（Ch 1 阅读路线第 3 步）· 接 [Ch 5](../../chapter-05-boot-memory-allocator/) **`mem_init()` 移交** 的空闲页。

---

## 本章在 VM 子系统中的位置

```
Ch 5 bootmem/memblock 退役 ──►  Buddy 接管 zone freelist
        ↓
Ch 6 alloc_pages / free_pages   ← 本章
        ↓
Ch 4 缺页 fault ──► 最终也调用 page allocator 拿 struct page
Ch 8 slab ──► 从 Buddy 拿整页再切小对象
Ch 7 vmalloc ──► 需要连续物理页时仍依赖 Buddy（或 CMA 等）
```

**HFT：** 用户态 **`mmap` fault**、内核 **`kmalloc`**（经 slab）底层都可能走到 **`alloc_pages`**；**`GFP_ATOMIC`**、**direct reclaim**、**跨 node 分配** 都会表现为 **延迟尖刺**。

→ 交叉：[Ch 2 Zone 水位](../../chapter-02-describing-physical-memory/notes/section-2-内存区域.md#区域水位线-zone-watermarks) · [Ch 4 缺页](../../chapter-04-process-address-space/notes/section-4-异常处理与缺页异常.md#4-异常处理与缺页异常-page-faulting)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 空闲块的管理 | [notes/section-1-空闲块的管理.md](./notes/section-1-空闲块的管理.md) |
| 2. 页面分配 | [notes/section-2-页面分配.md](./notes/section-2-页面分配.md) |
| 3. 页面释放 | [notes/section-3-页面释放.md](./notes/section-3-页面释放.md) |
| 4. GFP 标志与进程标志 | [notes/section-4-GFP-标志与进程标志.md](./notes/section-4-GFP-标志与进程标志.md) |
| 5. 避免碎片化 | [notes/section-5-避免碎片化.md](./notes/section-5-避免碎片化.md) |
| 6. 2.6 内核的新变化 | [notes/section-6-2.6-内核的新变化.md](./notes/section-6-2.6-内核的新变化.md) |

---

## 相关章节

- 上一章：[../chapter-05-boot-memory-allocator/](../chapter-05-boot-memory-allocator/)
- 下一章：[../chapter-07-noncontiguous-memory-allocation/](../chapter-07-noncontiguous-memory-allocation/)
- 附录 F：[../../appendix-F-物理页分配.md](../../appendix-F-物理页分配.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
