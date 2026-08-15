# 1. 本章要回答的问题

| 传统误区 | 本章视角 |
|----------|----------|
| 只看磁盘 I/O | 先看 **逻辑 I/O** 与 **缓存命中** |
| `iostat` 不忙但应用慢 | VFS 层 **`fileslower`**、**`ext4dist`** |
| 不知在读哪个文件 | **`filetop`**、**`opensnoop`** |
| 缓存是否有效 | **`cachestat`** 命中率 |

```
应用程序 read/write/mmap
        ↓
   VFS（opensnoop / vfsstat / fileslower）
        ↓
  页缓存 / dcache / inode cache（cachestat / readahead / writeback）
        ↓
  具体 FS：ext4 / xfs（ext4dist / xfsslower）
        ↓
  块设备 ——→ [Ch 9 磁盘 I/O](../../chapter-09-disk-io/)
```

---
