# Ch 6 §1 空闲块的管理 (Managing Free Blocks)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 1. 空闲块的管理 (Managing Free Blocks)

### 阶 (Order)

物理内存按 **连续页框块** 组织，块大小为 **2 的幂次页**：

| Order | 块含页数 |
|-------|----------|
| 0 | 1 页 |
| 1 | 2 页 |
| 2 | 4 页 |
| … | … |
| 9 | 512 页（原书 **`MAX_ORDER=10`** 时，最大 order 索引为 9） |

**阶 (order)** = 该块 **log₂(页数)** 的指数（表述上：order `k` 块含 **2^k** 页）。

### 数据结构

每个 **Zone** 维护 **`free_area_t` 数组**（现代 **`struct free_area`**）— **每个 order 一条空闲块链表**：

```
zone->free_area[0]  ──►  1-page  free blocks
zone->free_area[1]  ──►  2-page  free blocks
        …
zone->free_area[MAX_ORDER-1]  ──►  最大块
```

### 伙伴位图 (Buddy Bitmap)

为 **省空间**，用 **1 bit** 表示 **一对伙伴块** 的状态：

| 位值 | 含义（原书） |
|------|--------------|
| **0** | 该 **伙伴对** 要么 **全空闲**，要么 **全占用** |
| **1** | **恰有一边** 被占用 |

合并 / 拆分时查位图，决定 **能否 coalesce** 成更高 order。

→ 与 Ch 2 **`struct zone`**、**`pageset`** 同在一个 zone 数据结构族中。

---
