# Ch 12 内存管理 · Memory Management

> **Linux Kernel Development 3rd** · Robert Love · **选读**

> 本章定位：内核侧 **页、区、kmalloc/vmalloc、Slab、高端内存、per-CPU** — 与用户态 malloc/mmap **不同规则**。为 **Ch 15 进程地址空间**、**07 Gorman**、HFT **大页/mlock/零缺页** 补内核视角。

---

## ⚠️ 过时标记（LKD 3rd 基于 2.6.34，现为 6.x）

| LKD 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **SLAB 分配器 §12.7** | **SLUB** 取代 SLAB（2.6.23 起默认） | [SLUB: The unqueued slab allocator](https://lwn.net/Articles/229096/) |
| `struct page` | 大量字段移出，改用 **`struct folio`** | [Folios and the page cache](https://lwn.net/Articles/895104/) |
| `kmalloc`/`vmalloc` | 接口仍在，但底层 SLUB 实现不同 | [Slab allocation improvements](https://lwn.net/Articles/887591/) |
| per-CPU 分配器 | 仍存在，但接口和实现有更新 | [Kernel doc: percpu](https://docs.kernel.org/core-api/percpu.html) |
| 高端内存 (highmem) | 64 位内核无 highmem 概念 | [Why folios?](https://lwn.net/Articles/880965/) |

> **原则**：§12.7 SLAB 仅作概念理解（看 SLUB 替代）。`struct page`→`folio` 是重大重构。其余分配器概念仍有效。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 页** | `struct page` | 物理页 · `_count` |
| **② 区** | Zones | DMA · NORMAL · HIGHMEM |
| **③ 获得页** | `alloc_pages` | 连续物理页 |
| **④ kmalloc** | gfp_mask | **GFP_KERNEL vs ATOMIC** |
| **⑤ vmalloc** | 虚连续 | 大分配 · TLB 代价 |
| **⑥ Slab** | 对象缓存 | `task_struct` 等 |
| **⑦ 内核栈** | 静态分配 | **勿大数组** |
| **⑧ 高端内存** | kmap | 永久 vs 原子映射 |
| **⑨ per-CPU** | 每核副本 | 少锁 · 少 cache 颠簸 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 为何内核内存更复杂 | [notes/section-12.1-为何内核内存更复杂.md](./notes/section-12.1-为何内核内存更复杂.md) |
| 页 | [notes/section-12.2-页.md](./notes/section-12.2-页.md) |
| 区 | [notes/section-12.3-区.md](./notes/section-12.3-区.md) |
| 获得页 | [notes/section-12.4-获得页.md](./notes/section-12.4-获得页.md) |
| kmalloc() 与 kfree() | [notes/section-12.5-kmalloc-与-kfree.md](./notes/section-12.5-kmalloc-与-kfree.md) |
| vmalloc() | [notes/section-12.6-vmalloc.md](./notes/section-12.6-vmalloc.md) |
| Slab 层 | [notes/section-12.7-Slab-层.md](./notes/section-12.7-Slab-层.md) |
| 在栈上的静态分配 | [notes/section-12.8-在栈上的静态分配.md](./notes/section-12.8-在栈上的静态分配.md) |
| 高端内存的映射 | [notes/section-12.9-高端内存的映射.md](./notes/section-12.9-高端内存的映射.md) |
| 每个 CPU 的分配 | [notes/section-12.10-每个-CPU-的分配.md](./notes/section-12.10-每个-CPU-的分配.md) |
| 分配选型速查 | [notes/section-12.11-分配选型速查.md](./notes/section-12.11-分配选型速查.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 基本单元？ | **物理页** · `struct page` |
| 为何要 Zones？ | **DMA 限制** · **内核映射限制** |
| kmalloc vs vmalloc？ | **物理连续快** vs **虚连续大块慢** |
| GFP_KERNEL vs ATOMIC？ | **可睡** vs **原子上下文** |
| Slab 解决什么？ | **对象缓存** · 快 · 抗碎片 |
| 内核栈？ | **极小** — 大对象 **堆/Slab** |
| per-CPU？ | **少锁** · ** locality** |

---

## 本章学习目标 · 自检

- [ ] 解释 **`struct page` 描述物理页而非数据**
- [ ] 说出 **ZONE_NORMAL vs ZONE_HIGHMEM**（32 位语境）
- [ ] 区分 **`GFP_KERNEL` 与 `GFP_ATOMIC`** 使用场景
- [ ] 知 **vmalloc 的 TLB 代价**
- [ ] 联系 **Ch 3 Slab 分配 task_struct**
- [ ] HFT：对照用户态 **大页、mlock、对象池、NUMA 本地分配**

---

## 相关章节

- 上一章：[../chapter-11-timers/](../chapter-11-timers/)
- 下一章：[../chapter-13-vfs/](../chapter-13-vfs/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
