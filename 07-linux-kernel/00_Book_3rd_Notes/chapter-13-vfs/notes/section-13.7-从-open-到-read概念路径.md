## 从 open 到 read（概念路径）

```
open("/path/file", O_RDONLY)
    ▼
路径 walk ──► dcache 命中？ ──► inode lookup
    ▼
分配 struct file ──► 填入 files_struct->fd[]
    ▼
read(fd, ...)
    ▼
VFS sys_read ──► file->f_op->read ──► ext4_file_read / …
    ▼
（可能）页缓存 Ch 16 ──► 块层 Ch 14
```

---
