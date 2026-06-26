## ① 通用文件系统接口与抽象层

**VFS** = 用户 **系统调用**（`open`/`read`/`write`/`close`…）与 **底层存储/实现** 之间的 **粘合剂**。

```
用户态 read(fd, buf, n)
    ▼
sys_read() ──► VFS（统一语义）
    ▼
具体 FS：ext4 / xfs / fat / proc / sysfs / socket…
    ▼
块设备 / 内存 / 网络 / 驱动…
```

| 设计 | 效果 |
|------|------|
| **抽象层**规定统一概念与数据结构 | 各 FS **隐藏细节** |
| 对内核其余部分 | **所有文件系统看起来一样** |

| 可共存示例 | NTFS · FAT · ext4 · 各类 Unix FS… |

→ [Ch 1](../../chapter-01-intro/) **一切皆文件** · [Ch 5](../../chapter-05-system-calls/) · [08-TLPI Ch3 文件 I/O](../../../../08-The-Linux-Programming-Interface/chapter-03-file-io/)

**HFT：** 热路径 **`read`/`write`/`mmap`/`send`** 都经 VFS 或并行子系统；排障可分层：**syscall → VFS → 具体 FS/协议栈**。

→ [03 SysPerf Ch8 VFS 追踪](../../../../03-Systems-Performance-2nd/chapter-08-file-systems/notes/section-8.4-文件系统架构与特性.md)

---
