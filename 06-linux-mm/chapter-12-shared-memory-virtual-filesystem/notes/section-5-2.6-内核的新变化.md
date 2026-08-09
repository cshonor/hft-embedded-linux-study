# Ch 12 §5 2.6 内核的新变化

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 5. 2.6 内核的新变化

| 改进 | 说明 |
|------|------|
| **`shmem_inode_info.alloced`** | 记录 **已分配页数** — 免 2.4 **动态遍历计数** |
| **`VM_ACCOUNT`** | **精确内存配额** — 防 **overcommit** 下 shmem **无限涨** |
| **`llseek` / `sendfile`** | 扩展文件语义 |
| **非线性映射 (nonlinear mappings)** | 大文件 **稀疏 / 特定页映射** 优化 |
| **专用 inode slab cache** | inode **快速分配/回收** |

**HFT 延伸：** **`memfd_create` + sealing**（现代 Linux）— **仍走 shmem** — 创建 **无路径的 RAM 文件** 再 **mmap**，比 **SysV key** 更易用。

---

## 两条 IPC 路径一图

```
                    进程 A          进程 B
                       │               │
    真实文件 MAP_SHARED ├───────────────┤  page cache 同一 struct page
                       │               │
    tmpfs 文件 mmap     ├───────────────┤  shmem inode 页树
    (/dev/shm/x)       │               │
                       │               │
    MAP_SHARED 匿名     ├───────────────┤  内部 dev/zero shmem 文件
                       │               │
    SysV shmget/shmat   ├───────────────┤  内部 SYSVNN shmem 文件
```

---

## HFT 精读 checklist

| 需求 | 做法 |
|------|------|
| **跨进程零拷贝读行情** | **`mmap MAP_SHARED`** on **tmpfs 文件** 或 **memfd** |
| **避免 swap 拖慢共享区** | **`mlock` 映射区** · 足够 RAM |
| **理解 swap 行为** | 共享 shmem 页 swap 索引在 **inode** — **换入一次** 多 PTE 受益 |
| **别用 MAP_PRIVATE 当共享** | PRIVATE → **COW** — 各进程 **各一份** |
| **与 Ch 10 LRU** | shmem 页在 **page cache / LRU** — 内存紧时 **可被回收**（除非 mlock） |

---
