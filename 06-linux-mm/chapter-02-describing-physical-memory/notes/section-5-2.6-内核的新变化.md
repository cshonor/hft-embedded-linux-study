# Ch 2 §5 2.6 内核的新变化 (What's New in 2.6)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`include/linux/mmzone.h` 的 `per_cpu_pages`、`CACHELINE_PADDING`）

---

## 本节讲什么

原书末尾总结 2.6 相对 2.4 的三大变化，本质都是**「多核下减少全局锁争用」**。本节：

1. 讲清这三点各自解决什么争用
2. 用 v6.6 源码把每一点做实
3. 顺带补一条「从 2.6 到 v6.6 又变了什么」的演进视角

---

## 1. LRU 链表本地化（2.4 → 2.6）

| 2.4 | 2.6+ |
|-----|------|
| `active_list` / `inactive_list` **全局各一条** | LRU 链**移入每个 `struct zone`** 内部维护 |
| 回收顺序全局竞争（一把大锁） | **按 zone 局部**决定回收，更贴合 NUMA / 多 zone |

**v6.6 现状：** LRU 已经进一步下沉——`struct pglist_data` 里有 `struct lruvec __lruvec`（`mmzone.h:1383`），LRU 以 **node + memcg** 为粒度组织（`mem_cgroup_lruvec()`）。粒度从「全局」→「zone」→「node×memcg」，每一层都在缩小锁的半径。回收细节见 [Ch 10](../../chapter-10-page-frame-reclamation/)。

---

## 2. 每 CPU 页缓存（per-CPU pageset）→ v6.6 `per_cpu_pages`

**问题：** 多 CPU 同时从 zone 的 freelist 取页，抢同一把 `zone->lock` 自旋锁。

**2.6 做法：** 在 `struct zone` 里给**每个 CPU** 维护热/冷页缓存（pageset），多数分配先命中 per-CPU 列表，减少锁竞争。

**v6.6 真身（`mmzone.h:679`）：**

```c
struct per_cpu_pages {
    spinlock_t lock;        /* 保护本 CPU 列表（本地锁，争用极小） */
    int count;              /* 列表里的页数 */
    int high;               /* 高水位：超过就回灌 buddy（bulk free） */
    int batch;              /* 从 buddy 批量取/还的块大小 */
    short free_factor;      /* 释放路径的批量缩放因子 */
    struct list_head lists[NR_PCP_LISTS];  /* 按 migrate type 分的页链表 */
} ____cacheline_aligned_in_smp;
```

**机制：** 分配时先取本 CPU 的 `per_cpu_pages`，空了才以 `batch` 为单位从 buddy 批量 refill（拿一次锁换一批页）；释放时先攒在本 CPU 列表，攒够 `high` 再批量还 buddy。**把「每页一次锁」变成「每批一次锁」**。

---

## 3. 结构体填充（Padding）：锁/热字段分 cache line

2.6 在 `struct zone` 里加 padding，让 `zone->lock` 和 `zone->lru_lock` 这类**高频成对访问的锁**落在不同 cache line，避免 false sharing。

**v6.6 真身：** `struct zone` 里三段 `CACHELINE_PADDING(_pad1_/_pad2_/_pad3_)`（`mmzone.h:945/985/1001` 附近），把「读密集字段（水位）」「写密集字段（free_area + lock）」「统计字段」物理隔开：

```c
struct zone {
    /* 只读字段：水位、计数 ... */
    CACHELINE_PADDING(_pad1_);              /* 隔开读写 */
    struct free_area free_area[MAX_ORDER + 1];  /* 写密集 */
    spinlock_t lock;                        /* 高频锁 */
    CACHELINE_PADDING(_pad2_);
    /* compaction/vmstat ... */
    CACHELINE_PADDING(_pad3_);
    atomic_long_t vm_stat[NR_VM_ZONE_STAT_ITEMS];  /* 每 CPU 会碰的统计 */
} ____cacheline_internodealigned_in_smp;
```

`____cacheline_internodealigned_in_smp` 还保证整个 `struct zone` 在 NUMA 下按**跨节点 cache line** 对齐。

---

## 4. 从 2.6 到 v6.6：又变了什么

| 主题 | 2.6（原书） | v6.6 |
|------|------------|------|
| 大页表示 | compound page（首页+尾页） | **folio**（`mm_types.h:293`）一等公民 |
| LRU 粒度 | per-zone | **node × memcg**（`lruvec`） |
| 页缓存单元 | `struct page` | 逐渐迁到 **folio**（大块读写按 folio 操作） |
| 新 zone 类型 | DMA/NORMAL/HIGHMEM | + `ZONE_DMA32` / `ZONE_MOVABLE` / `ZONE_DEVICE` |
| 水位 | MIN/LOW/HIGH | + `WMARK_PROMO`（NUMA 迁移） |
| bootmem | bootmem 分配器 | **memblock** 接管（Ch 5） |

**时代说明贯穿本章：** 三层结构（Node → Zone → Page）的**骨架没变**，但每一层的字段、粒度、配套机制都在演进。读原书抓骨架，读 v6.6 抓真身。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| per-CPU pageset | 内核版「每核缓存减少锁」——用户态 mempool 设计的内核镜像 |
| zone padding | 锁/热字段分 cache line——用户态结构体同样要做（订单簿、计数器） |
| `batch` 批量 refill | 「拿一次锁换一批」的批处理思想，与 DPDK mempool 批量出队同构 |
| false sharing | `____cacheline_aligned_in_smp` 与 `____cacheline_internodealigned_in_smp` 是内核侧的 cache line 对齐工具 |

---

## 6. 衔接

- 上节 [§4 高端内存](./section-4-高端内存.md)
- 下章：[Ch 3 页表管理](../../chapter-03-page-table-management/)（虚拟地址怎么落到这些页框）
- 回收落地：[Ch 10 页框回收](../../chapter-10-page-frame-reclamation/)（LRU、kswapd 详解）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：per-CPU pageset 的「热页/冷页」是什么意思？**
A：指**缓存亲和性**。热页是「刚刚被本 CPU 用过、大概率还在本 CPU 的 L1/L2 cache 里」的页，冷页则相反。2.6 时代把 pageset 分热/冷两区，让 CPU 优先复用自己 cache 里的页。v6.6 里这个概念弱化了（分配器更关注 migrate type），但「per-CPU 缓存减少锁」的核心不变。

**Q2：`high` 和 `batch` 两个参数分别控制什么？**
A：`batch` 是「从 buddy 批量取/还的块大小」——一次拿锁换多少页；`high` 是「per-CPU 列表的触发回灌水位」——攒到这么多就批量还 buddy。`batch` 定粒度，`high` 定上限，两者一起决定「锁被拿得多频繁」和「每 CPU 最多囤多少页」。

**Q3：为什么 padding 能消除 false sharing？**
A：CPU 以 **cache line（通常 64B）** 为单位加载/失效缓存。两个被不同 CPU 频繁写、却落在同一 cache line 的字段，会互相把对方的 cache line 打失效，造成跨核缓存一致性流量。用 padding 把它们推到不同 cache line，就切断了这种「假共享」。`CACHELINE_PADDING` 宏就是干这个的。

**Q4：`____cacheline_internodealigned_in_smp` 和 `____cacheline_aligned_in_smp` 差在哪？**
A：前者按**跨 NUMA 节点**的 cache line 大小对齐（避免跨 socket 的一致性流量，代价是更大的对齐填充），后者按**本节点内** cache line 对齐。`struct zone` 这类跨节点共享的结构用 internode 版本，`per_cpu_pages` 这种每 CPU 私有的用普通版本。

**Q5：folio 出现后，`struct page` 是不是变小了？**
A：是。核心字段（flags/lru/mapping/index/_refcount 等）提升到 `struct folio`，`struct page` 瘦身成薄包装。这也缓解了 §3 讲的「`struct page` 每大 1 字节全系统多耗几 MiB」的自我吞噬问题——字段按需住在 folio 里，而不是每个页都背上全套。

</details>

---
