# Ch 6 §6 2.6 内核的新变化

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 6. 2.6 内核的新变化

### 每 CPU 页面集合 (Per-CPU Page Lists · pageset)

| 问题 | 2.6 方案 |
|------|----------|
| 多核 **order-0（单页）** 分配极频 **`zone->lock` 争用** | 每 CPU **`pageset`**：**热页 / 冷页** 缓存 |
| 每次 alloc/free 都抢 zone 锁 | **0 阶** 多数路径 **无锁** 从 **本 CPU pageset** 取/还 |

与 [Ch 2 §2.6 pageset](../../chapter-02-describing-physical-memory/notes/section-5-2.6-内核的新变化.md#每-cpu-页面集合-per-cpu-page-lists--pageset) 同一机制 — **读源码时 zone 结构和 page_alloc 要一起看**。

### 统一 NUMA API

2.4 UMA/NUMA **不同底层函数** → 2.6 **`numa_node_id()`** 等 **统一隐式 node 选择** — 应用仍可用 **`set_mempolicy`** 覆盖。

---

## Buddy 分配 + pageset 简图

```
                    alloc_pages(gfp, order)
                              │
              ┌───────────────┴───────────────┐
              │ order == 0 ?                  │
              └───────────────┬───────────────┘
                    是        │        否
                    ▼         │         ▼
            CPU pageset       │    zone->free_area[order]
            (hot/cold)        │         │
                    │         │    无块则 split 高 order
                    └─────────┴─────────┘
                              │
                    水位 / GFP / reclaim ?
                              │
                         struct page *
```

---

## HFT 精读 checklist

| 现象 | 查什么 |
|------|--------|
| **远程 NUMA 延迟** | 是否 **node-local** 分配；`numastat` |
| **latency 尖刺** | **direct reclaim**、**compaction**、**GFP** 上下文 |
| **大页分配失败** | **外部碎片** — 长期运行后 **2MB 连续物理页** 难拿 |
| **与 slab/mempool** | Buddy 管 **页**；热路径对象应用 **池化**（Ch 8 / DPDK） |
| **监控** | `/proc/vmstat`（`pgalloc_*`、`allocstall`** 等） |

---
