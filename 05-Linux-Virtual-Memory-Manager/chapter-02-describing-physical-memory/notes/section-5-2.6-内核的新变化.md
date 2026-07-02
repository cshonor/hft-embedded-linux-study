# Ch 2 §5 2.6 内核的新变化 (What's New in 2.6)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 5. 2.6 内核的新变化 (What's New in 2.6)

原书末尾强调 2.6 相对 2.4 的三点 — **对多核性能理解仍有用**：

### LRU 链表本地化

| 2.4 | 2.6+ |
|-----|------|
| **`active_list` / `inactive_list` 全局** | LRU 链 **移入各 `struct zone` 内部** 维护 |
| 回收顺序全局竞争 | **按 zone 局部** 决定回收 — 更贴合 NUMA / 多 zone 现实 |

→ 接 [Ch 10 页框回收](../../chapter-10-page-frame-reclamation/)。

### 每 CPU 页面集合 (Per-CPU Page Lists · `pageset`)

| 问题 | 2.6 做法 |
|------|----------|
| 多 CPU **同时** 从 zone  freelist 取页 → **`zone->lock` 争用** | 在 **`struct zone`** 里为 **每个 CPU** 维护 **热/冷页缓存**（`pageset`） |
| 频繁自旋锁 | 多数分配 **先命中 per-CPU 列表**，减少锁竞争 |

**HFT 关联：** 与 **per-CPU 变量、无锁热路径** 同一思路 — 把 **共享锁上的操作** 变成 **本地缓存 + 批量 refill**（→ DPDK mempool、订单簿 per-core 分配区）。

### 结构体填充 (Padding · 缓存行隔离)

2.6 在 **`struct zone`** 中增加 **padding**，使 **`zone->lock`** 与 **`zone->lru_lock`** 等 **高频成对访问的锁** 落在 **不同 cache line** 上。

→ **false sharing / 缓存一致性流量** — 与 [Hennessy Ch2](../02-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/) · HFT **订单簿/cache line 对齐** 同构。

---

## 三层结构一图

```
                    ┌─────────────────────────────────┐
                    │  Node (pg_data_t)               │
                    │  NUMA node 0, 1, …              │
                    └───────────────┬─────────────────┘
                                    │
              ┌─────────────────────┼─────────────────────┐
              ▼                     ▼                     ▼
        ┌───────────┐       ┌───────────┐       ┌───────────┐
        │ ZONE_DMA  │       │ZONE_NORMAL│       │ZONE_HIGHMEM│
        │ watermarks│       │ LRU lists │       │ (32-bit)  │
        │ wait table│       │ pageset   │       └───────────┘
        └─────┬─────┘       └─────┬─────┘
              │                   │
              └─────────┬─────────┘
                        ▼
              每个物理页框 → struct page
              (mapping, count, flags: active/dirty/locked/…)
```

---

## HFT 精读 checklist

| 概念 | 落地 |
|------|------|
| **Node** | 进程/线程 **绑 NUMA node** 分配；订单簿 **与 NIC 同 node** |
| **Zone watermarks** | 监控 **direct reclaim**；避免与延迟敏感线程争用内存 |
| **`struct page` / LRU** | 理解 **mlock** 钉住的是哪些页；THP 是 **compound page** 叠加在此之上 |
| **per-CPU pageset** | 内核侧「**每核缓存减少锁**」— 用户态 mempool 设计的内核版镜像 |
| **zone padding** | **锁/热字段分 cache line** — 用户态结构体同样要做 |

---
