# Ch 12 §4 建立共享区与 System V IPC

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 4. 建立共享区与 System V IPC

### 匿名 `MAP_SHARED` → `shmem_zero_setup()`

```
mmap(MAP_SHARED | MAP_ANONYMOUS, …)
    → shmem_zero_setup()
    → 在内部 shm _fs 静默创建 **dev/zero** 类虚拟文件
    → do_mmap 映射该 inode
```

**效果：** 用户以为是 **匿名共享内存**；内核看来是 **shmem 文件映射** — **统一 VM 路径**。

### System V：`shmget()` / `shmat()` 揭秘

| 用户 API | 内核实际 |
|----------|----------|
| **`shmget(key, size, …)`** | 在 **shm fs** 创建内部文件 **`SYSVNN`**（NN = key 编码） |
| **`shmat()`** | **`do_mmap()`** 把该 **虚拟文件** 映射进进程地址空间 |

**现代对照：** **POSIX shared memory**（`shm_open`）常映射 **`/dev/shm/xxx`** — 同样 **tmpfs/shmem** 后端。

**HFT：** 多进程 **读同一块 mmap 行情** = **MAP_SHARED shmem 页** + **用户态 seqlock/atomic**；**写时 COW 不共享** — 须 **SHARED** 而非 **PRIVATE**（Ch 4 COW）。

---
