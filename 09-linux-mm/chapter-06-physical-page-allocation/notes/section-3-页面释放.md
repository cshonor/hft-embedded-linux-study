# Ch 6 §3 页面释放 (Free Pages)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 3. 页面释放 (Free Pages)

### `free_pages()` 与合并 (Coalescing)

释放 **order-k** 块时：

```
free 块 B
    伙伴 B' 也空闲？ ──否──► B 挂入 free_area[k]
         │
         是
         ▼
    合并为 order-(k+1) 块 ──► 递归检查能否继续合并
```

**与拆分对称** — 保证 **相同 order 的空闲块可拼接**，缓解 **外部碎片**。

---
