# Ch 5 启动内存分配器 · Boot Memory Allocator

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过**（HFT 热路径 **不经过** bootmem；读作 **boot → 伙伴系统**  handoff 背景即可）

## 本章解决什么问题

内核启动时的 **「鸡生蛋」**：

```
要初始化 物理页分配器（伙伴系统, Ch 6）
    → 需要分配内存来放它的数据结构
        → 但伙伴系统 还没就绪 …
```

在 **正式物理页分配器**（**Buddy Allocator**）完全可用之前，内核使用 **临时、结构简单** 的 **启动内存分配器 (Boot Memory Allocator)** 为 **boot 阶段** 的数据结构分配内存 — **包括伙伴系统自身初始化所需的内存**。

> **时代说明：** 原书描述 **`bootmem` / `bootmem_data`**（2.4–2.6 常见）。**现代主线** 已 largely 由 **`memblock`** 取代（bootmem 代码已移除）；**角色相同** — early alloc → **`mem_init()` 退役** → 页框交给 **`page_alloc.c` 伙伴系统**。读当前源码见 [`mm/memblock.c`](https://elixir.bootlin.com/linux/latest/source/mm/memblock.c) · [`mm/init-mm.c`](https://elixir.bootlin.com/linux/latest/source/mm/init-mm.c) 等。

---

## 本章在 VM 子系统中的位置

```
Ch 3 pagetable_init / paging_init  （需要 early 内存）
        ↓
Ch 5 bootmem / memblock            ← 本章：临时位图分配器
        ↓
Ch 6 page_alloc.c · 伙伴系统       ← boot 结束 handoff 目标
        ↓
Ch 4 运行时 fault / mmap          ← 进程用的已是 Buddy + slab 世界
```

**HFT：** 生产路径 **从不** 调用 `alloc_bootmem`；理解本章有助于 **读 dmesg / 启动参数 / 内存布局**，以及 **「第一个真正 page 从哪来」**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 启动内存映射的表示 | [notes/section-1-启动内存映射的表示.md](./notes/section-1-启动内存映射的表示.md) |
| 2. 发现与初始化 | [notes/section-2-发现与初始化.md](./notes/section-2-发现与初始化.md) |
| 3. 内存分配与释放 | [notes/section-3-内存分配与释放.md](./notes/section-3-内存分配与释放.md) |
| 4. 启动内存分配器的退役 | [notes/section-4-启动内存分配器的退役.md](./notes/section-4-启动内存分配器的退役.md) |
| 5. 2.6 内核的新变化 | [notes/section-5-2.6-内核的新变化.md](./notes/section-5-2.6-内核的新变化.md) |

---

## 相关章节

- 上一章：[../chapter-04-process-address-space/](../chapter-04-process-address-space/)
- 下一章：[../chapter-06-physical-page-allocation/](../chapter-06-physical-page-allocation/)
- 附录 E：[../../appendix-E-启动内存分配器.md](../../appendix-E-启动内存分配器.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
