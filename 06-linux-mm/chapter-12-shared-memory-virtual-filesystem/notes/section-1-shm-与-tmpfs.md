# Ch 12 §1 shm 与 tmpfs

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 1. shm 与 tmpfs

| 变体 | 谁可见 | 用途 |
|------|--------|------|
| **`shm`** | **内核内部挂载**，用户 **不可见** | **匿名 MAP_SHARED**、**SysV shm** 的 **内部后备** |
| **`tmpfs`** | 管理员挂载 — **`/tmp/`、`/dev/shm/`** 等 | **RAM 临时文件系统** — 用户 **mmap tmpfs 文件** 做 IPC |

**共同点：** 数据 **只在内存**（+ swap 可能），**极快** — HFT 常用 **`/dev/shm/my_ring`** 或 **memfd_create**（现代，后于原书）映射共享区。

```
用户：mmap(/dev/shm/feed, MAP_SHARED)
内核：走 tmpfs → shmem 页 → 多进程共享同一 struct page（rmap）
```

---
