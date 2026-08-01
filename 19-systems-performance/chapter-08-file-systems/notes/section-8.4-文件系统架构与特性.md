## 8.4 文件系统架构与特性

### VFS（虚拟文件系统）

```
App: read() / write() / open()
         ↓
    VFS: vfs_read() / vfs_write()  ← BPF 追踪统一点
         ↓
    ext4 / xfs / zfs / ...
         ↓
    block layer → disk
```

**性能分析：** 可在 **应用 syscall**、**VFS**、**具体 FS（ext4_xfs_*）** 各层量延迟 — 层越低越接近真实磁盘。

→ Ch 3 [VFS 概念](../../chapter-03-operating-systems/)

### Linux 缓存层

| 缓存 | 存什么 | 工具 |
|------|--------|------|
| **Page Cache** | 文件 **内容**页 | `free`、`cachestat` |
| **Dentry Cache** | 路径名 → inode 查找 | `sar -v` |
| **Inode Cache** | inode 结构 / 属性 | `sar -v`、`slabtop` |

**与 Ch 7 关系：** page cache 占用 **主存** — `free` 低不代表没内存，可能是 **cache 可回收**（直到 direct reclaim）。

### 高级特性

| 特性 | 作用 | 性能 |
|------|------|------|
| **Extents** | 连续块分配，减碎片 | 顺序大文件友好 |
| **Journaling** | 崩溃一致性 | 元数据 journal 增写放大 |
| **COW** | 快照、克隆 | btrfs/ZFS；写路径可能变复杂 |

### 常见文件系统（Linux）

| FS | 特点 | HFT 场景 |
|----|------|----------|
| **ext4** | 默认、成熟 | 系统盘、日志盘 |
| **XFS** | 大文件、并行分配组、延迟分配 | 大容量日志 / 数据归档 |
| **ZFS** | ARC 缓存、存储池、recordsize | 非 tick 路径；调 recordsize 匹配 I/O |
| **btrfs** | COW、快照 | 备份、开发环境 |

**HFT 实践：** 系统盘 ext4/xfs + **`noatime`**；NVMe 日志盘与数据盘 **分离**，避免 journaling 与 bulk 写争抢。

---


---

← [本章导读](../README.md)
