## ③ 操作对象 · Operations Objects

每个主对象内嵌 **函数指针表** — 具体 FS **实现方法**：

| 操作结构 | 父对象 | 典型方法 |
|----------|--------|----------|
| **`super_operations`** | superblock | 读写 superblock、同步 FS |
| **`inode_operations`** | inode | `lookup`、`create`、`mkdir`… |
| **`dentry_operations`** | dentry | `d_hash`、`d_compare`… |
| **`file_operations`** | file | **`read`、`write`、`mmap`、`ioctl`**… |

```c
/* 概念：驱动/FS 填充 file_operations */
struct file_operations {
    ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
    /* ... */
};
```

| 多态 | VFS 调用 `file->f_op->read(...)` — 实际跑到 ext4 或 pipe 或设备驱动 |

→ [Ch 5](../../chapter-05-system-calls/) 替代 syscall：**字符设备** 也靠 **`file_operations`**

→ 教学对照：01 Day 18–19 FAT（具体 FS 在 VFS 之下）



<details>
<summary>自测题（点击展开）</summary>

**Q1.** file_operations、inode_operations、super_operations 分别在什么时机被调用？

<details><summary>答案</summary>

file_operations：对已打开文件操作（read/write/mmap/ioctl/poll），每次 syscall 直接调用。inode_operations：文件系统元数据操作（create/lookup/link/unlink），路径解析和文件创建时调用。super_operations：文件系统级别操作（alloc_inode/evict_inode/write_inode），inode 生命周期管理时调用。HFT 驱动如果实现字符设备，只需提供 file_operations。

</details>

</details>
---
