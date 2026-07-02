# Ch 12 §3 缺页与 Swap（共享 shmem 页）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 3. 缺页与 Swap（共享 shmem 页）

### 私有匿名页 vs 共享 shmem 页（换出时）

| | **私有匿名页** | **共享 shmem / tmpfs 页** |
|---|----------------|---------------------------|
| **swap 位置存哪** | **各进程 PTE** 里的 **`swp_entry`** | PTE **清零**；位置存 **inode 私有数据**（**类 ext2 块索引**） |
| **原因** | 单进程映射 | **多 PTE 映射同一页** — **统一由 inode 管 swap 索引** |

**inode 内 swap 管理（原书）：** 借用 **直接块 + 间接块** 思想 — 在 **`shmem_inode_info`** 里存 **页索引 → swp_entry** 向量。

### 缺页路径

**`shmem_nopage` / `shmem_getpage`**（现代 **`shmem_fault`** 等）：

```
fault on shmem VMA
    → 查 inode 页树 / swap 索引
    → 若在 swap：读盘（Ch 11）
    → 若未分配：alloc 新页（零页或 demand）
    → 安装 PTE present
```

→ [Ch 3 rmap](../../chapter-03-page-table-management/) — **多进程共享** 时 **换出/换入** 仍依赖 **rmap + swap cache**（Ch 10–11）。

---
