# Ch 6 §5 避免碎片化 (Avoiding Fragmentation)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 5. 避免碎片化 (Avoiding Fragmentation)

| 类型 | 含义 | Buddy 表现 |
|------|------|------------|
| **外部碎片** | 总空闲够，但 **没有足够大的连续物理块** | Buddy **拆分/合并** 专门缓解 |
| **内部碎片** | 只需 **1 小块**，却分到 **整页或多页**，页内浪费 | Buddy **按 2^n 页分配** → **易产生** |

**内部碎片 → Slab：** 内核大量 **小于一页** 的对象（`task_struct`、`inode`…）不直接用 Buddy 裸分，而由 **[Ch 8 Slab](../../chapter-08-slab-allocator/)** 在 **整页内切小对象**。

**HFT 用户态镜像：** **DPDK mempool**、**订单簿 arena** — 向 OS 要 **大页/大块**，内部 **自己切对象**，同一逻辑。

**大页 (huge order)：** 请求 **高 order** 或 **hugetlbfs** 需要 **长时间保持连续物理内存** — 外部碎片严重时会 **alloc 失败**（`ENOMEM`），即使 **总 free 内存不少**。

---
