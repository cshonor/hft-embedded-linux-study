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

#### 「一切皆文件」的边界（VFS 不只是磁盘）

| 经 VFS 的东西 | 背后实现 | file_operations 提供者 |
|--------------|----------|----------------------|
| 普通/日志文件 | ext4/xfs/btrfs | 磁盘 FS |
| `/proc` `/sys` | procfs/sysfs | 内核按需生成（读时现算，**无页缓存**） |
| 管道、FIFO | pipefs | 内核内存环形缓冲 |
| socket | sockfs | 协议栈（`socket_file_ops`） |
| `/dev/null` `/dev/zero` | devtmpfs | 字符设备驱动 |
| eventfd / epoll fd / signalfd | 匿名 inode | 内核子系统各自注册 |

> 这个表是 HFT 视角的干货：**热路径上最常见的 fd——socket、eventfd、epoll、共享内存 fd——全部是 VFS 对象**。`epoll_ctl` 注册的就是 file*，`SO_REUSEPORT` 的负载均衡按 file/socket 找接收者。VFS 不是"文件系统章节"，它是**用户态握住的一切内核资源的把手**。

#### VFS 的两份「合同」

| 合同 | 内容 | 违约后果 |
|------|------|----------|
| **对上（用户态/syscall 层）** | 语义统一：open/read/write/lseek/mmap 的参数、错误码（ENOENT/EISDIR/…）、fd 生命周期 | 程序无需为不同 FS 写不同代码 |
| **对下（具体 FS）** | 接口统一：实现四张操作表（super/inode/dentry/file ops）即可接入内核 | 新 FS 只要填表，**不用改内核其余部分** |

> 第二份合同就是 Ch13.3 的主题——它决定了「给内核写一个新文件系统」的工程量是**填一张表**，而不是改上千处调用点。

→ [Ch 1](../../chapter-01-intro/) **一切皆文件** · [Ch 5](../../chapter-05-system-calls/) · [03-linux-userspace-api Ch4 文件 I/O](../../../03-linux-userspace-api/chapter-04-file-io-universal/)

**HFT：** 热路径 **`read`/`write`/`mmap`/`send`** 都经 VFS 或并行子系统；排障可分层：**syscall → VFS → 具体 FS/协议栈**。

#### 分层排障实操（这条链怎么用）

| 症状 | 先看哪层 | 工具 |
|------|----------|------|
| read 慢，不确定卡在哪 | syscall 入口耗时 vs VFS 以下耗时 | `perf trace -p <pid>` / `strace -T -e read`（分层计时） |
| 怀疑 page cache 未命中 | VFS 之下的块层 | `perf stat -e 'dtlb*'`、`/proc/meminfo` 的 Cached/Buffers |
| socket send 慢 | 不是磁盘 FS——走 sockfs 的 `socket_file_ops` | `ss -tin`、BPF 追 `tcp_sendmsg` |
| open 风暴（配置反复重读） | dcache 路径解析层 | `perf trace -e openat`、观察 dentry 增长 `/proc/sys/fs/dentry-state` |

> 分层心智模型的价值：**把"IO 慢"这个模糊陈述拆成 syscall 层 / VFS 层 / 具体实现层三选一**，再选对工具。与 06.6 SysPerf Ch8 的 USE 方法（每层都问利用率/饱和度/错误）是同一件事。

→ [06.6 SysPerf Ch8 VFS 追踪](../../../06.6-systems-performance/chapter-08-file-systems/notes/section-8.4-文件系统架构与特性.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** VFS 的作用是什么？为什么 HFT 工程师需要理解它？

<details><summary>答案</summary>

VFS 是系统调用（open/read/write）和具体文件系统（ext4/nfs/proc）之间的抽象层。HFT 需要 VFS 因为：1) 热路径 read/write 经过 VFS；2) /proc 和 /sys 是 VFS 文件系统，用于调优（CPU 亲和性/调度策略）；3) mmap 文件时 VFS 决定 page cache 行为。理解 VFS 能在排障时分清是 syscall 层、VFS 层还是文件系统层的问题。

</details>

**Q2.** socket 也说「一切皆文件」——socket fd 经 VFS 吗？证据是什么？

<details><summary>答案</summary>

经。socket 的 inode 属于内核内部的 **sockfs**，accept 返回的 fd 指向一个 `struct file`，其 `f_op` 是 `socket_file_ops`（sock_read_iter/sock_write_iter/sock_mmap…）。所以 `read(fd)` 对 socket 也能用（等价 recv 默认语义），epoll 注册的也是这个 file。反过来说明「一切皆文件」的实现机制正是 VFS 操作表：**任何内核对象只要实现 file_operations 并挂进 fd 表，就自动获得整套文件 API**（open/close/mmap/poll/epoll 全部免费）。

</details>

**Q3.** `/proc/cpuinfo` 的 read 和 `/data/log.txt` 的 read，在 VFS 层之下的开销结构有何本质区别？

<details><summary>答案</summary>

**磁盘文件**：read 命中 page cache 则内存拷贝；未命中则块层 IO（毫秒级），且首次 read 要走完整的 dcache/inode 路径。**procfs**：数据**不落盘、也不进 page cache**——每次 read 都调用该 /proc 条目注册的 seq_file 回调**现场生成**（比如 cpuinfo 要遍历所有 CPU 结构体格式化输出）。含义：① procfs 的 read 成本 = 内核执行回调的 CPU 时间，几乎无 IO；② 但高频读 /proc 仍是纯 CPU 开销（曾有用 /proc/stat 做监控导致软中断风暴的事故）；③ HFT 热路径绝不应有 /proc 轮询——配置读一次缓存住。

</details>

</details>
---
