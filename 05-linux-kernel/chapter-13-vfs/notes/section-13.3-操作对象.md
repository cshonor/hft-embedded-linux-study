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

> ⚠️ **版本断崖**：上面是 LKD3rd 时代的形状。v6.6 里 `read`/`write` 两个函数指针**还在但已是遗留兼容槽**，现代 FS/驱动一律实现 `read_iter`/`write_iter`。核对 v6.6 `include/linux/fs.h`：`vfs_read()` 优先走 legacy `f_op->read`（若有），否则经 `new_sync_read()` → `call_read_iter()` → `f_op->read_iter(&kiocb, &iov_iter)`；且内核会在 read_iter 缺失时打警告（`!file->f_op->read_iter || file->f_op->read` 分支）。

#### 为什么 read 改成 read_iter（一次接口革命的动机）

| 维度 | 旧 `read`（缓冲区指针） | 新 `read_iter`（iov_iter） |
|------|------------------------|---------------------------|
| 参数形状 | `char __user *buf, size_t` | `struct kiocb *`（控制块）+ `struct iov_iter *`（迭代器） |
| 分散/聚合 IO | 不支持——readv 要单独的 readv 槽位 | **一个回调通吃** read/readv/preadv（iov_iter 抽象了"一串缓冲区"） |
| AIO / io_uring | 无法表达"稍后完成" | kiocb 携带完成回调——**io_uring 异步读直接复用这条路径** |
| 内核内调用者 | 需要 user 指针 hack | splice/sendfile/copy_file_range 全部改走 iter |

> 这是教科书级的**接口随负载演化**案例：syscall 表面看着没变（read/readv 各是各的），VFS 内部却统一成一个带异步能力的迭代器接口。**读 2.6 时代的资料时凡是画 `f_op->read` 调用图的，都过时了**——现代内核里那个槽只有极老的驱动还在用。

#### v6.6 file_operations 全景（真实槽位清单）

| 分组 | 槽位 |
|------|------|
| 读写核心 | `llseek` `read` `write` `read_iter` `write_iter` `iopoll` `fadvise` |
| 遍历/元数据 | `iterate_shared`（getdents 用） |
| 事件等待 | `poll` |
| 控制 | `unlocked_ioctl` `compat_ioctl` `check_flags` `flock` `lock` |
| 内存映射 | `mmap` `get_unmapped_area` `mmap_capabilities` |
| 生命周期 | `open` `flush` `release`（close 时调用——**注意名字不叫 close**） |
| 持久化 | `fsync` `fasync` `fallocate` `setlease` |
| 数据搬运捷径 | `splice_read` `splice_write` `splice_eof` `copy_file_range` `remap_file_range` |
| 调试 | `show_fdinfo`（`cat /proc/<pid>/fdinfo/<fd>` 的来源） |

> `show_fdinfo` 值得单独记：epoll fd、eventfd 都实现了它，所以你能 `/proc/self/fdinfo/<epfd>` 直接看到注册了多少事件——**VFS 接口连调试口都是标准化的**。

| 多态 | VFS 调用 `file->f_op->read_iter(...)` — 实际跑到 ext4 或 pipe 或设备驱动 |

→ [Ch 5](../../chapter-05-system-calls/) 替代 syscall：**字符设备** 也靠 **`file_operations`**

→ 教学对照：01 Day 18–19 FAT（具体 FS 在 VFS 之下）



<details>
<summary>自测题（点击展开）</summary>

**Q1.** file_operations、inode_operations、super_operations 分别在什么时机被调用？

<details><summary>答案</summary>

file_operations：对已打开文件操作（read/write/mmap/ioctl/poll），每次 syscall 直接调用。inode_operations：文件系统元数据操作（create/lookup/link/unlink），路径解析和文件创建时调用。super_operations：文件系统级别操作（alloc_inode/evict_inode/write_inode），inode 生命周期管理时调用。HFT 驱动如果实现字符设备，只需提供 file_operations。

</details>

**Q2.** 写一个现代字符设备驱动，read 回调该实现 `read` 还是 `read_iter`？为什么？

<details><summary>答案</summary>

`read_iter`。v6.6 中 legacy `read` 槽还在（vfs_read 优先兼容它），但：① 新代码用 legacy 槽会触发内核警告路径（read_iter 缺失检测）；② readv/preadv 会退化为多次复制调用你的 read——性能和语义都受损；③ io_uring 异步读**只认 iter 接口**，legacy 路径要靠同步包装，等于放弃了 io_uring 直达能力。`read_iter` 的实现成本并不高：`copy_to_iter(buf, len, to)` 一行替代 `copy_to_user`（iov_iter API 自带 user/kernel 判断）。这也是判断一份驱动教程新旧的金标准——还在教 `ssize_t my_read(struct file *f, char __user *buf, ...)` 的教程至少落后十年。

</details>

**Q3.** `copy_file_range` 和 `remap_file_range` 这两个槽位是干什么的？为什么它们的存在能让「跨进程传文件数据」不走用户态？

<details><summary>答案</summary>

`copy_file_range`：**服务器端/同 FS 文件间复制**，数据可完全不走用户态甚至不走 page cache 到用户再回内核（同 FS 时 ext4 可以走 extent 级 reflink/数据块引用）。`remap_file_range`：即 FICLONERANGE/ioctl 路径——**写时复制克隆**，两个文件共享数据块，直到一方写入才分裂。它们挂在 file_operations 上意味着：syscall 层（sendfile/copy_file_range/ioctl）只是通用入口，**每个 FS 自己决定能优化到什么程度**（不支持就返回 -EXOPNOTSUPP，VFS 层退化为读+写的通用实现）。对 HFT：行情回放/历史数据加载用 copy_file_range/reflink 可以把 GB 级数据复制从"内存带宽问题"降为"元数据操作"。

</details>

</details>
---
