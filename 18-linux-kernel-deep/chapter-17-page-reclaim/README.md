# Ch 17 页框回收 · Page Frame Reclaiming

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **🟡 选读**  
> 内存耗尽 — PFRA、LRU、反向映射、swap、OOM

---

## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **LRU 双链表** (active/inactive) | **Multi-generational LRU** (MGLRU, 6.1+) | [Multi-generational LRU](https://lwn.net/Articles/856931/) |
| `shrink_zone()` | 重写为 MGLRU 回收路径 | [MGLRU documentation](https://docs.kernel.org/admin-guide/mm/multigen_lru.html) |
| OOM killer | 仍存在但策略可配置 (cgroup OOM) | [Cgroup-aware OOM killer](https://lwn.net/Articles/704179/) |

> **原则**：LRU→MGLRU 是页回收算法的重构。ULK3 的回收路径仅作概念理解，现代实现查 MGLRU 文档。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. PFRA与页分类 | [notes/section-2-PFRA与页分类.md](./notes/section-2-PFRA与页分类.md) |
| 3. 反向映射 | [notes/section-3-反向映射.md](./notes/section-3-反向映射.md) |
| 4. LRU链表 | [notes/section-4-LRU链表.md](./notes/section-4-LRU链表.md) |
| 5. 执行时机与OOM | [notes/section-5-执行时机与OOM.md](./notes/section-5-执行时机与OOM.md) |
| 6. 交换机制 | [notes/section-6-交换机制.md](./notes/section-6-交换机制.md) |

---

## 相关

- 上一章：[chapter-16-file-access/](../chapter-16-file-access/)
- 下一章：[chapter-18-ext2-ext3/](../chapter-18-ext2-ext3/)
- 深潜：[07 Gorman Ch 10](../../06-linux-mm/) · [Ch 8 伙伴系统](../chapter-08-memory-management/) · [Ch 15 页缓存](../chapter-15-page-cache/)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
