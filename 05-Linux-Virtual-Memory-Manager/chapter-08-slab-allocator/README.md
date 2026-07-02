# Ch 8 Slab分配器 · Slab Allocator

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读**（HFT：**对象池 / per-CPU 本地缓存 / cache line 对齐** 的用户态设计，直接对照本章）

[Ch 6 Buddy](../../chapter-06-physical-page-allocation/) 按 **整页（2^n）** 分配 — 快，但对 **小于一页** 的请求造成 **内部碎片**。**Slab 分配器** 在 **Buddy 之上** 做 **对象级缓存**：把 **物理页切成固定大小对象**，并 **复用已释放实例**。

> **时代说明：** 原书描述 **经典 SLAB**（`kmem_cache_t`、`slab_t`、`kmem_bufctl`）。**现代主线** 默认多为 **SLUB**（`mm/slub.c`）— API 仍是 **`kmem_cache_*` / `kmalloc`**，内部实现更简；**思想一致**：cache → slab(full/partial/free) → per-CPU 本地池。读源码以当前树为准（[`mm/slub.c`](https://elixir.bootlin.com/linux/latest/source/mm/slub.c) 或 `CONFIG_SLAB` 时 [`mm/slab.c`](https://elixir.bootlin.com/linux/latest/source/mm/slab.c)）。

---

## 本章在 VM 子系统中的位置

```
Ch 6 Buddy ──整页──►  Slab 切对象 / kmalloc 尺寸档
        ↑                    │
        └──── grow slab ─────┘（kmem_cache_grow → alloc_pages）
Ch 4 进程 mm_struct、inode 等 ──► 多由 slab 分配
HFT 用户态 ──► DPDK mempool / 自研 order pool 是同构设计
```

**HFT 为什么要精读：** 不是要写内核 slab，而是 **订单簿节点、Order 对象、fix message** — **固定大小 + 池化 + 每核缓存 + cache line 对齐**，与 slab 三大目标 **一一对应**。

→ 交叉：[Hennessy Ch2 cache line](../02-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/) · [12-DPDK mempool](../12-DPDK-Low-Latency-Network/01-Intro-Book/notes/chapter-01-DPDK架构与EAL/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. Slab 分配器的三大核心目标 | [notes/section-1-Slab-分配器的三大核心目标.md](./notes/section-1-Slab-分配器的三大核心目标.md) |
| 2. 核心数据结构：Cache 与 Slab | [notes/section-2-核心数据结构：Cache-与-Slab.md](./notes/section-2-核心数据结构：Cache-与-Slab.md) |
| 3. 对象分配与释放 | [notes/section-3-对象分配与释放.md](./notes/section-3-对象分配与释放.md) |
| 4. 尺寸缓存  与 `kmalloc` / `kfree` | [notes/section-4-尺寸缓存-与-kmalloc-kfree.md](./notes/section-4-尺寸缓存-与-kmalloc-kfree.md) |
| 5. 每 CPU 对象缓存 | [notes/section-5-每-CPU-对象缓存.md](./notes/section-5-每-CPU-对象缓存.md) |
| 6. 2.6 内核的新变化 | [notes/section-6-2.6-内核的新变化.md](./notes/section-6-2.6-内核的新变化.md) |

---

## 相关章节

- 上一章：[../chapter-07-noncontiguous-memory-allocation/](../chapter-07-noncontiguous-memory-allocation/)
- 下一章：[../chapter-09-high-memory-management/](../chapter-09-high-memory-management/)
- 附录 H：[../../appendix-H-Slab分配器.md](../../appendix-H-Slab分配器.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
