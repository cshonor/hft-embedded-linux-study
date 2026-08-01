# Ch 9 §2 映射与解除映射高端页

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 2. 映射与解除映射高端页

### 常规：`kmap()` / `kunmap()`

```
struct page *page  (HIGHMEM)
    kmap(page)  → 内核可用的 void *vaddr（占用 PKMap 槽）
    … 内核读写该页 …
    kunmap(page) → 释放 PKMap 槽
```

| 特点 | 说明 |
|------|------|
| **可能睡眠** | PKMap 满时需 **等待** 槽位 — **不可在中断里用** |
| **必须配对 kunmap** | 否则 **映射泄漏** |

### 原子：`kmap_atomic()` / `kunmap_atomic()`

| 特点 | 说明 |
|------|------|
| **不睡眠** | 供 **中断 / 原子上下文** |
| **每 CPU 预留槽** | 每核 **固定用途** 的页表项 — **极快** |
| **必须同 CPU 配对 unmap** | 槽位 **CPU 私有** |

→ 与 [Ch 6 `GFP_ATOMIC`](../../chapter-06-physical-page-allocation/notes/section-4-GFP-标志与进程标志.md#4-gfp-标志与进程标志-gfp--process-flags) 同类：**硬上下文不能用会睡的路径**。

**现代 x86_64：** 大量原 **HIGHMEM kmap** 路径 **不再执行**；**`kmap_local_page`** 等是后续演进 — 思想仍是 **临时内核 VA 窗口**。

---
