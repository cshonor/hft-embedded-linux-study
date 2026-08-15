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



<details>
<summary>自测题（点击展开）</summary>

**Q1.** open("/path/file", O_RDONLY) 到 read(fd, buf, n) 的完整路径是什么？

<details><summary>答案</summary>

open: 1) 路径解析走 VFS dcache → 2) 找到 inode → 3) 权限检查 → 4) 分配 file 结构 → 5) fd 表中分配 fd → 返回 fd。read: 1) fd → file → file_operations->read → 2) 检查 page cache → 3) 命中则拷贝到用户态；未命中则发起磁盘 IO → 4) 数据进入 page cache → 5) 拷贝到用户 buf。HFT 用 mmap 跳过 read 的拷贝步骤。

</details>

</details>
---
