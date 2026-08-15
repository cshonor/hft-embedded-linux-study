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

→ [Ch 1](../../chapter-01-intro/) **一切皆文件** · [Ch 5](../../chapter-05-system-calls/) · [03-linux-userspace-api Ch4 文件 I/O](../../../03-linux-userspace-api/chapter-04-file-io-universal/)

**HFT：** 热路径 **`read`/`write`/`mmap`/`send`** 都经 VFS 或并行子系统；排障可分层：**syscall → VFS → 具体 FS/协议栈**。

→ [03 SysPerf Ch8 VFS 追踪](../../../14-systems-performance/chapter-08-file-systems/notes/section-8.4-文件系统架构与特性.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** VFS 的作用是什么？为什么 HFT 工程师需要理解它？

<details><summary>答案</summary>

VFS 是系统调用（open/read/write）和具体文件系统（ext4/nfs/proc）之间的抽象层。HFT 需要 VFS 因为：1) 热路径 read/write 经过 VFS；2) /proc 和 /sys 是 VFS 文件系统，用于调优（CPU 亲和性/调度策略）；3) mmap 文件时 VFS 决定 page cache 行为。理解 VFS 能在排障时分清是 syscall 层、VFS 层还是文件系统层的问题。

</details>

</details>
---
