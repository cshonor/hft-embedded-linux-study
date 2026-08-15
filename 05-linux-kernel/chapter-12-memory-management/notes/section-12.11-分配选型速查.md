## ⑪ 分配选型速查

内核分配 **没有万能 malloc** — 按 **大小、连续性、上下文、性能** 选型。本表作 **Ch 12 总复习 + 驱动/HFT 决策树**。

#### 主决策表

| 需求 | 首选 API | 避免 |
|------|----------|------|
| **若干连续物理页** | `alloc_pages` / `__get_free_pages` | `vmalloc` 再逐页查 PA |
| **小对象、物理连续、快** | **`kmalloc`** | 中断里 `GFP_KERNEL` |
| **中断 / softirq / 持 spinlock** | **`kmalloc(..., GFP_ATOMIC)`** 或 **预分配池** | 任何可能 **睡眠** 的路径 |
| **固定类型、高频 alloc/free** | **`kmem_cache_*`** | 裸 `kmalloc` 相同 size |
| **大块、仅内核访问、非热点** | **`vmalloc` / `vzalloc`** | DMA 缓冲 |
| **设备 DMA 一致映射** | **`dma_alloc_coherent`**（驱动 API） | `vmalloc` + 手工 PA |
| **每核私有、高频写** | **per-CPU** | 全局 `atomic_t` |
| **HIGHMEM 页内核访问** | **`kmap_atomic`**（短）/ `kmap`（可睡） | 假设 `page_address` 非 NULL |
| **栈上临时** | **仅几百字节内** | 大数组 |

#### 按上下文

| 上下文 | 可用 gfp | 不可用 |
|--------|----------|--------|
| **进程上下文，无锁** | `GFP_KERNEL` | — |
| **进程 + spinlock** | **`GFP_ATOMIC`** | `GFP_KERNEL` |
| **hardirq / timer callback** | **`GFP_ATOMIC`** + 预池 | `GFP_KERNEL`、`kmap` |
| **softirq** | 同 atomic；仍须 **短** | 睡眠 |

#### 按大小（x86 量级，配置可变）

| 大小 | 倾向路径 |
|------|----------|
| **≤ 512B～几 KB** | Slab → **`kmalloc`** |
| **~128KB～几 MB** | **`kmalloc` 上限** 或 **`__get_free_pages`** |
| **> KMALLOC_MAX** | **`vmalloc`** 或多页 `alloc_pages` |
| **2MB / 1GB huge** | **`alloc_pages` high order** / **hugetlb**（用户 Ch 15） |

#### ASCII 决策流

```
需要分配？
    │
    ├─ 固定类型、极高频？ ──► kmem_cache
    │
    ├─ 每核计数/队列？ ──► per-CPU
    │
    ├─ 要物理连续？
    │     ├─ 整页/ DMA？ ──► alloc_pages / dma_*
    │     └─ 字节？ ──► kmalloc (看 gfp)
    │
    └─ 大块、不要求 PA 连续？ ──► vmalloc
```

#### HFT / 低延迟清单

| # | 原则 |
|---|------|
| 1 | **启动期** 完成所有 **`kmalloc` / `alloc_pages` / Slab create** |
| 2 | **数据面** 仅 **cache hit / ring pop** — **零 GFP_ATOMIC** |
| 3 | **NUMA** — 网卡所在 node **`set_mempolicy` / 驱动 local alloc** |
| 4 | **测失败路径** — `GFP_ATOMIC` 耗尽时 **丢包 vs 延迟尖刺** |
| 5 | 用户态 **mlock + hugepage** 与内核 **CMA/reserve** 对称规划 |

→ [Ch 12 各节](./section-12.1-为何内核内存更复杂.md) · [06 Gorman 全书索引](../../../06-linux-mm/) · [Ch 15 用户 mmap](../../chapter-15-process-address-space/) · [17 HFT Practice](../../../16-hft-engineering/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 总结：内核中需要分配 200 字节、4MB、100MB 分别用什么？

<details><summary>答案</summary>

200 字节 → kmalloc(200, GFP_KERNEL)，从 kmalloc-256 slab 分配，O(1)。4MB → alloc_pages(GFP_KERNEL, 10)（2^10=1024 页=4MB），从 buddy 系统分配，物理连续。100MB → vmalloc(100MB)，虚拟连续物理可不连续，需改页表。选型口诀：小用 kmalloc、大且连续用 alloc_pages、大且不连续用 vmalloc。

</details>

</details>
---
