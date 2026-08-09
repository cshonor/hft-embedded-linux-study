# Ch 5 §5 2.6 内核的新变化

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 5. 2.6 内核的新变化

2.4 → 2.6 **bootmem 无架构级重写**，主要是 **小优化**：

| 变化 | 作用 |
|------|------|
| **`last_success` 字段**（`bootmem_data_t`） | 记录 **上次成功分配位置** — 缩短后续 **扫描位图找空闲位** 的距离 |

与 **`last_pos` / `last_offset`** 同类：**减少 boot 路径上 O(n) 位图扫描**。

---

## bootmem → Buddy 一图

```
        上电 / arch setup
              │
              ▼
    探测 PFN 范围 (min_low_pfn, max_low_pfn, …)
              │
              ▼
    init_bootmem_core · 位图 per node
              │
              ▼
    alloc_bootmem*  ──► 页表、mem_map、伙伴系统元数据 …
              │
              ▼
    mem_init() · 退役 bootmem
              │
              ▼
    空闲页框 ──► Buddy (Ch 6) ──► 此后所有 runtime 分配
```

---

## HFT / 阅读建议

| 读者 | 建议 |
|------|------|
| **HFT 工程** | **可跳过正文**；知道 **运行时内存来自 Buddy + slab**，bootmem 仅 **启动 几百毫秒** |
| **读内核启动 / 调试** | 理解 **memblock Reserve/Allocate** 与 **`/proc/iomem`** 中 **Kernel code / reserved** 从哪来 |
| **继续精读** | [Ch 6 物理页分配](../../chapter-06-physical-page-allocation/) · Ch 1 路线中的 [`page_alloc.c`](https://elixir.bootlin.com/linux/latest/source/mm/page_alloc.c) |

---
