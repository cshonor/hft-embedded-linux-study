# Ch 9 §5 2.6 内核的新变化

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 5. 2.6 内核的新变化

| 变化 | 说明 |
|------|------|
| **通用 `mempool`** | 2.4 **HIGHMEM 专用** emergency pool → 2.6 **`mempool_t`** 全内核可用 |
| **移除 `page->virtual`** | 2.4 在 **`struct page`** 存 PKMap 的 **vaddr** — 海量 page 结构 **浪费内存**；2.6 **删除**，改用 **其他映射跟踪** |

**HFT：** **`struct page` 体积** 影响 **mem_map / vmemmap 内存开销** — 现代仍强调 **page 结构紧凑**（**compound page、THP** 等叠在上）。

---

## HIGHMEM 访问路径一图（32 位）

```
HIGHMEM struct page
        │
        ├─ 内核要读写内容
        │      kmap / kmap_atomic → PKMap VA → 访问
        │      kunmap / kunmap_atomic
        │
        └─ 设备 DMA
               bounce buffer (LOWMEM) ↔ memcpy ↔ HIGHMEM page
               emergency pool 保底 bounce 分配
```

---

## HFT / 阅读建议

| 读者 | 建议 |
|------|------|
| **x86_64 HFT** | **跳过正文**；记住 **Ch 2 HIGHMEM 为何存在**、**DMA 物理地址可见性** |
| **嵌入式 / 32 位** | 精读 **kmap / bounce** |
| **与 Ch 8 衔接** | **emergency pool → mempool** — 关键路径 **reserve** 思想 |
| **继续** | [Ch 10 页框回收](../../chapter-10-page-frame-reclamation/) |

---
