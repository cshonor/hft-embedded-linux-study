# Ch 5 §1 启动内存映射的表示 (`bootmem_data`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 1. 启动内存映射的表示 (`bootmem_data`)

**每个内存节点 (Node)** 在启动阶段用 **`bootmem_data`**（或 **`bootmem_data_t`**）记录分配状态：

| 字段 | 作用 |
|------|------|
| **`node_bootmem_map`** | 指向 **位图 (bitmap)** — **每一位对应一个物理页框**：空闲 / 已分配 |
| **`last_pos`** | 上次分配用到的 **物理页框号 (PFN)** |
| **`last_offset`** | 该页内 **上次分配结束的偏移** |

**合并小额分配：** 新的小块分配若 **仍能塞进 `last_pos` 那一页** 的剩余空间，则 **与上次同页合并** — 减少 **整页占用**（boot 阶段内存也珍贵）。

```
bootmem_data (per node)
    node_bootmem_map ──► [ bit per page frame: 0=free 1=used ]
    last_pos / last_offset ──►  bump-pointer 式同页续分配
```

→ 与 Ch 2 **Node / PFN / struct page** 同一套物理页框编号。

---
