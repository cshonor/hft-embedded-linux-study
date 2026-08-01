# Ch 7 非连续内存分配 · Noncontiguous Memory Allocation

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过**（HFT 热路径 **优先物理连续大页**；`vmalloc` 适合 **大块内核模块/驱动** — 理解 **与 `kmalloc`/Buddy 的分工** 即可）

## 为什么需要 vmalloc

[Ch 6 Buddy](../../chapter-06-physical-page-allocation/) 分配 **物理连续** 的 **2^n 页** — 快、缓存友好。但 **外部碎片** 积累后，可能 **凑不出大块连续物理内存**，尽管 **总空闲页足够**。

**`vmalloc()` 的取舍：**

| | **Buddy / `__get_free_pages`** | **`vmalloc()`** |
|---|-------------------------------|-----------------|
| **物理内存** | **连续** | **不连续**（每页独立 alloc） |
| **虚拟地址** | 内核线性映射或固定映射 | **虚拟连续** 一段 VA |
| **TLB / 缓存** | 较好 | **每页独立映射** — TLB 压力更大 |
| **典型用途** | 热路径、DMA、大页 | **大但不必物理相邻** 的内核缓冲区、模块加载 |

→ Ch 1 阅读路线 **第 2 步**：[`mm/vmalloc.c`](https://elixir.bootlin.com/linux/latest/source/mm/vmalloc.c)

---

## 本章在 VM 子系统中的位置

```
Ch 6 Buddy：要连续物理页，失败时可能 EMEM（碎片）
        ↓
Ch 7 vmalloc：虚拟连续、物理可散 — 用页表「拼」出来
        ↓
Ch 8 slab：小块对象 — 通常 Buddy 拿整页再切
Ch 4 fault：vmalloc 区访问时同步页表项
```

**HFT：** 用户态 **订单簿 / ring buffer** 应 **`mmap` + hugepage / 预 fault 连续页** — **不要** 模仿 vmalloc（内核 API）。内核模块若大块非 DMA 缓冲，才可能走 vmalloc。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 描述虚拟内存区域 | [notes/section-1-描述虚拟内存区域.md](./notes/section-1-描述虚拟内存区域.md) |
| 2. 分配非连续区域 | [notes/section-2-分配非连续区域.md](./notes/section-2-分配非连续区域.md) |
| 3. 释放非连续区域 | [notes/section-3-释放非连续区域.md](./notes/section-3-释放非连续区域.md) |
| 4. 2.6 内核的新变化 | [notes/section-4-2.6-内核的新变化.md](./notes/section-4-2.6-内核的新变化.md) |

---

## 相关章节

- 上一章：[../chapter-06-physical-page-allocation/](../chapter-06-physical-page-allocation/)
- 下一章：[../chapter-08-slab-allocator/](../chapter-08-slab-allocator/)
- 附录 G：[../../appendix-G-非连续内存分配.md](../../appendix-G-非连续内存分配.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
