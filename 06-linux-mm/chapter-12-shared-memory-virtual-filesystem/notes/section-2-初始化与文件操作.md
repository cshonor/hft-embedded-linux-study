# Ch 12 §2 初始化与文件操作

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 2. 初始化与文件操作

### `init_tmpfs()` 与 `shmem_inode_info`

| 组件 | 作用 |
|------|------|
| **`init_tmpfs()`** | 注册并挂载 **shm / tmpfs** |
| **`shmem_inode_info`** | 每个 **inode 私有数据** — 管 **页树、swap 向量、已分配页计数** 等 |

### 文件 ops（原书）

**`mmap` · `read` · `write` · 目录/链接 · truncate** — 与 **普通 FS** 接口一致。

| 特例 | 原因 |
|------|------|
| **`fsync` 空操作** | 数据 **仅在 RAM** — **无需刷盘**（除非 swap） |

**HFT：** 共享 ring **write 后无 fsync 成本** — 但需 **用户态内存屏障 / 原子序** 做 **进程间同步**（不属于 VM 章，但别误以为 fsync 能代替）。

---
