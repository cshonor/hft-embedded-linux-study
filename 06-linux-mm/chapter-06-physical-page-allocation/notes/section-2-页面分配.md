# Ch 6 §2 页面分配 (Allocating Pages)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 2. 页面分配 (Allocating Pages)

### 核心：`alloc_pages()` / `__alloc_pages()`

**分配 order = n** 的连续 **2^n** 页：

```
请求 order-k 块
    free_area[k] 有空？ ──是──► 取下返回
         │
         否
         ▼
    从更高 order 取块 ──► **拆分 (split)**
         │                    ├─ 一半用于本次分配
         │                    └─ 另一半（伙伴）挂入较低 order 链表
         ▼
    重复直到满足或失败
```

**按需拆分 (Splitting)：** 没有正好大小的块时，**把大块一分为二**，一半分配、一半 **降级** 入链 — Buddy 算法核心。

### 节点本地与 Zone Fallback

| 策略 | 行为 |
|------|------|
| **Node-local** | 优先从 **当前 CPU 所属 NUMA node** 分配 |
| **Zone fallback** | 首选 zone（如 **HIGHMEM**）不够 → 按预定顺序 **降级**（如 **NORMAL → DMA**） |

**HFT：** **`numactl --membind`**、**`mbind(MPOL_BIND)`** 是在 **用户态约束** 这一策略；违背后 **remote node** 分配 = **更高延迟**。

→ [Ch 2 Nodes](../../chapter-02-describing-physical-memory/notes/section-1-内存节点.md#1-内存节点-nodes)

---
