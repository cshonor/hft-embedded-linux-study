# Ch 8 内存管理 · Memory Management

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **🔴 HFT 精读**  
> 物理页框分配 — 伙伴系统、Slab、`vmalloc`

---

## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **SLAB 分配器** | **SLUB** 取代 SLAB（2.6.23 起默认） | [SLUB: The unqueued slab allocator](https://lwn.net/Articles/229096/) |
| `kmem_cache` 结构 | SLUB 简化了结构，接口变化 | [Slab allocation improvements](https://lwn.net/Articles/887591/) |
| **`struct page`** | 大量字段移出，改用 **`struct folio`** | [Folios and the page cache](https://lwn.net/Articles/895104/) |
| 页框管理 `__GFP_*` | flag 更新，GFP 接口调整 | [Why folios?](https://lwn.net/Articles/880965/) |

> **原则**：SLAB→SLUB、page→folio 是两大重构。ULK3 的 Slab 分配器章节仅作概念理解，现代实现查 bootlin 内存管理训练材料。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 页框管理 | [notes/section-2-页框管理.md](./notes/section-2-页框管理.md) |
| 3. Slab 分配器 | [notes/section-3-Slab分配器.md](./notes/section-3-Slab分配器.md) |
| 4. 非连续内存与 vmalloc | [notes/section-4-非连续内存与vmalloc.md](./notes/section-4-非连续内存与vmalloc.md) |

---

## 相关

- 上一章：[chapter-07-process-scheduling/](../chapter-07-process-scheduling/)
- 下一章：[chapter-09-process-address-space/](../chapter-09-process-address-space/)
- 深潜：[07 Gorman](../../09-linux-mm/) · [chapter-17-page-reclaim/](../chapter-17-page-reclaim/)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
