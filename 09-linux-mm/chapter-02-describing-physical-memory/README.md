# Ch 2 描述物理内存 · Describing Physical Memory

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读**

Linux 支持多种硬件体系结构，内核需要 **与体系结构无关** 的方式描述物理内存。本章按 **宏观 → 微观** 展开：**节点 (Nodes) → 区域 (Zones) → 物理页框 (Pages)**。

> **时代说明：** 原书以 **2.4 / 2.6** 内核为主（`zone_t`、`struct zone` 等命名）。现代主线仍保留 **Node / Zone / struct page** 三层直觉，具体字段与 LRU、per-CPU 页缓存机制在后续版本继续演进 — 读源码以 [Elixir Bootlin](https://elixir.bootlin.com/linux/latest/source) 为准。

---

## 本章在 VM 子系统中的位置

```
物理内存怎么「分块、分档、分状态」描述清楚
        ↓
Ch 3 页表：虚拟地址如何映射到这些页框
        ↓
Ch 4 VMA：进程地址空间如何引用这些结构
        ↓
Ch 6 page_alloc / Ch 10 回收：在这些 Zone 上分配与回收
```

**HFT 为什么要先读本章：** **NUMA 本地分配**、**Zone 水位与回收延迟**、**struct page 状态**、**缓存行对齐（padding）** — 都直接影响 **绑核绑内存、mlock、大页、内存池布局** 时「为什么会抖、为什么会慢」。

→ 源码入口（Ch 1 路线第 3 步）：[`mm/page_alloc.c`](https://elixir.bootlin.com/linux/latest/source/mm/page_alloc.c) · 相关 [`mm/mmzone.c`](https://elixir.bootlin.com/linux/latest/source/mm/mmzone.c)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 内存节点 | [notes/section-1-内存节点.md](./notes/section-1-内存节点.md) |
| 2. 内存区域 | [notes/section-2-内存区域.md](./notes/section-2-内存区域.md) |
| 3. 物理页框 | [notes/section-3-物理页框.md](./notes/section-3-物理页框.md) |
| 4. 高端内存 | [notes/section-4-高端内存.md](./notes/section-4-高端内存.md) |
| 5. 2.6 内核的新变化 | [notes/section-5-2.6-内核的新变化.md](./notes/section-5-2.6-内核的新变化.md) |

---

## 相关章节

- 上一章：[../chapter-01-introduction/](../chapter-01-introduction/)
- 下一章：[../chapter-03-page-table-management/](../chapter-03-page-table-management/)
- 附录 B：[../../appendix-B-描述物理内存.md](../../appendix-B-描述物理内存.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
