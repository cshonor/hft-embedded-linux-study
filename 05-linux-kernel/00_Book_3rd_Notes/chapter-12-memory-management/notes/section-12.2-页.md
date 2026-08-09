## ② 页 · Pages

物理内存管理的基本粒度是 **页（page frame）** — 通常 **4KB**（`PAGE_SIZE`），Huge page 另论。

#### `struct page` — 描述页框，不是页内数据

| 要点 | 说明 |
|------|------|
| **`struct page`** | **每个物理页框** 一个描述符 — 在 **mem_map[]** 或 **sparse vmemmap** 中 |
| **不是** 页内容 | 数据通过 **`page_address()` / kmap** 得到 **内核 VA** |
| **`page->flags`** | 脏、锁定、LRU、compound head 等 |
| **`page->_refcount`** | **引用计数** — 归零且不在 LRU 时可回收 |

```
物理 RAM 页框 #N
    │
    ├── struct page[N]   ← 元数据
    └── 4KB 数据          ← 通过线性映射或 kmap 访问
```

#### 页的状态（概念）

| 状态 | 含义 |
|------|------|
| **空闲** | 在 **伙伴系统** free list |
| **已分配** | refcount > 0 — 某子系统持有 |
| **缓存** | **页缓存**（Ch 16）— 可回收 |
| **Slab 页** | 切成固定对象槽 |

#### 与伙伴系统（Buddy）

| 阶 order | 含义 |
|----------|------|
| **order 0** | 1 页（4KB） |
| **order 1** | 2 页连续 |
| **order n** | 2^n 页连续 |

`alloc_pages(gfp, order)` 从 **合适 order** 链表取块；释放时 **尝试合并 buddy** 减碎片。

#### 嵌入式 / ARM 注意

| 点 | 说明 |
|----|------|
| **`PAGE_SIZE`** | 多数 ARM32/64 为 **4KB**；部分 **64KB** 配置 |
| **无 MMU** | **uClinux** 等 flat 模型 — 无 `struct page` 完整语义（本书以 MMU Linux 为主） |
| **CMA** | **Contiguous Memory Allocator** — 给 DMA 预留 **连续** 大块 |

**HFT：** 用户态 **`mmap` + hugepage** 也按 **页框** 粒度映射 — **TLB 条目数** ∝ 覆盖 VA / page size。内核 **`__get_free_pages(order)`** 要 **物理连续** — 长时间运行 **碎片化** 后 **大块 order 失败** 类似用户态 **hugetlb 池耗尽**。

→ [06 Gorman Ch2 页框](../../../../06-linux-mm/chapter-02-describing-physical-memory/notes/section-3-物理页框.md) · [Ch 15 用户页表映射](../../chapter-15-process-address-space/)


> ↔ [ULK Ch8 §2 页框管理](../../../../20-linux-kernel-deep/chapter-08-memory-management/notes/section-2-页框管理.md)
---
