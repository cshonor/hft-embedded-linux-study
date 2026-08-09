# Ch 10 §1 页面替换策略 (Page Replacement Policy)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 1. 页面替换策略 (Page Replacement Policy)

Linux **不用纯 LRU**，而用 **双链表** 近似 **LRU 2Q + clock**：

| 链表 | 角色 |
|------|------|
| **`active_list`** | **工作集** — 近期频繁访问的页 |
| **`inactive_list`** | **回收候选** — 适合被踢出 |

**目标：** active 容纳 **各进程工作集**；inactive 供 **扫描回收** — 在 **近似 LRU** 与 **实现开销** 之间折中。

→ [Ch 2 §2.6 LRU 本地化](../../chapter-02-describing-physical-memory/notes/section-5-2.6-内核的新变化.md#lru-链表本地化) — **2.6 起 per-zone active/inactive**（非全局一条链）。

**`PG_active` / `PG_referenced` / accessed 位**（Ch 2 page flags · Ch 3 PTE young）— 驱动 **在 active ↔ inactive 间迁移**。

---
