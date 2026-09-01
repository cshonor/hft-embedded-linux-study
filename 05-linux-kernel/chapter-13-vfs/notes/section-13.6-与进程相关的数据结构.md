## ⑥ 与进程相关的数据结构

每进程与 VFS 通过三类结构关联：

| 结构 | 内容 |
|------|------|
| **`files_struct`** | **打开文件表** · **fd 数组** → `struct file *` |
| **`fs_struct`** | **当前工作目录 pwd** · **根目录 root** |
| **`namespace`（挂载命名空间）** | 进程看到的 **挂载树视图** |

#### 命名空间

| 默认 | 所有进程 **共享** 同一挂载命名空间 |
|------|----------------------------------|
| 容器 | 每进程可有 **独立** 挂载层次（Docker 等） |

```
进程 A: files_struct ── fd[0,1,2, socket_fd, log_fd ...]
        fs_struct    ── pwd=/opt/strategy  root=/
        namespace    ── 看到的主机挂载树
```

#### v6.6 源码里的 fd 表（两级结构 + RCU，核对 `include/linux/fdtable.h`）

```c
struct files_struct {
    /* read mostly part（读多写少，独占缓存行） */
    struct fdtable __rcu *fdt;      /* 指向下面的嵌入表或扩容表 */
    struct fdtable      fdtab;      /* 嵌入的第一级表 */
    /* written part（写热字段，另一缓存行） */
    spinlock_t    file_lock;
    unsigned int  next_fd;          /* fd 分配的提示起点 */
    unsigned long open_fds_init[1]; /* 位图：哪些 fd 打开着 */
    struct file __rcu *fd_array[NR_OPEN_DEFAULT]; /* 64 个内嵌槽 */
};
```

| 设计 | 机制 | 为什么 |
|------|------|--------|
| **两级表** | 前面 64 个 fd（`NR_OPEN_DEFAULT = BITS_PER_LONG`）内嵌在 files_struct 里；超过才 `kmalloc` 扩容一张新 fdtable，`fdt` 指针原子切换 | 绝大多数进程 <64 个 fd——**零分配**覆盖常态；扩容是罕见路径 |
| **RCU 换表** | 扩容时旧 fd 数组用 `call_rcu` 延迟释放；读者无锁走 `fdget()` | dup/close 与 read 并发时，read 里拿到的 file* 依然有效（配合 file 自身引用计数） |
| **读写缓存行分离** | "read mostly part" 与 file_lock/next_fd 分列注释两侧 | fdget 是每次 read/write 都走的热路径——别让它和 close 抢同一缓存行 |
| **fd = 位图分配** | open_fds 位图找零位 + next_fd 提示 | fd 分配 O(1)，且保证**最小可用 fd** 复现（dup2/重定向依赖这个确定性） |

> `fdget()` 的细节值得追：它不是简单读 `fdt->fd[n]`——先尝试**借用 files_struct 的引用计数**（fast path：若 files_struct 被当前任务独占/或 fd 未被并发 close，免原子操作），失败才对 file 做 `get_file()` 原子加引用。**热路径 syscall 的每一次省原子操作，都是这么抠出来的**（同一思想：Ch6 的 per-CPU、Ch9 的 seqlock）。

#### 三个结构的分镜头

| 结构 | clone 时的命运 | 典型受害者/受益者 |
|------|---------------|------------------|
| `files_struct` | CLONE_FILES 共享（线程）或不共享（fork 复制 fd 表、每个 fd 加引用） | 线程共享 fd 是**零成本**；fork 后父子 close 互不影响（各自引用计数） |
| `fs_struct` | CLONE_FS 共享 pwd/root——**线程 chdir 全线程生效** | 多线程程序里 chdir 是隐形全局变量 |
| `mnt_ns` | unshare(CLONE_NEWNS) 才分裂 | 容器文件视图隔离的根 |

→ **Ch 3** 进程资源「打开的文件」· **Ch 12** inode Slab 缓存



<details>
<summary>自测题（点击展开）</summary>

**Q1.** files_struct 和 fs_struct 分别管理什么？fd 表如何工作？

<details><summary>答案</summary>

files_struct 管理打开的文件描述符表（fd → file* 映射）；fs_struct 管理进程的根目录和当前工作目录（root/pwd）。fd 是整数索引，指向 file 结构。默认 fd 表大小 1024（RLIMIT_NOFILE），可通过 ulimit -n 调整。HFT 进程如果打开大量 socket 需要调高此限制。fork 后子进程继承 fd 表副本。

</details>

**Q2.** 为什么 0/1/2 分别是 stdin/stdout/stderr？

<details><summary>答案</summary>

这是 Unix 惯例：进程启动时 fd 0/1/2 已被 shell 打开并指向终端。C 库的 stdin/stdout/stderr 是 FILE* 包装的 fd 0/1/2。重定向 `> file` 就是 close(1) + open(file) → fd 1 被复用为文件。这种设计让所有程序天然支持重定向而无需特殊代码。

</details>

**Q3.** 线程 A 在 read(fd) 的同时线程 B close(同一个 fd)——为什么 read 不会拿悬空的 file 指针？

<details><summary>答案</summary>

两道防线：① **fd 表层**：fdget 用 RCU + files_struct 引用借用来保证「读 fdt->fd[n] 得到的指针」在当前 syscall 内不被 close 后释放（close 只把槽位置空 + 调 fput 减引用）。② **file 层**：read 全程持有该 file 的引用（`get_file`/`fput` 配对），close 触发的销毁要等**最后一个引用放下**才发生。所以 B 的 close 立刻"成功"（fd 槽空了，后续 open 会复用），但 A 的 read 仍在旧 file 上跑完。含义：close 的语义是「从 fd 表摘除」，**不是「立刻杀死进行中的操作」**——这是 SO_LINGER/EINTR 之外另一个「close 返回了但 IO 还在飞」的来源。

</details>

**Q4.** fd 表为什么内嵌 64 个槽而不是 4 个或 1024 个？扩容之后旧表怎么办？

<details><summary>答案</summary>

64 = BITS_PER_LONG（v6.6 fdtable.h：`NR_OPEN_DEFAULT = BITS_PER_LONG`）：位图 open_fds_init 恰好一个 unsigned long，**内嵌成本与位图对齐**——再大就浪费在 files_struct 本体（每个进程/线程一份）。选 4 会让多数进程被迫立刻扩容；1024 则每进程白背 8KB。扩容路径：分配新 fdtable + 新数组 + 新位图 → 复制 → `fdt` 指针 RCU 原子切换 → **旧数组 call_rcu 延迟释放**（等所有在飞读者退出临界区）。这是「小对象内嵌、大对象外挂、指针无缝升级」的通用内核模式（对照：files_struct 之于 fdtable ≈ vmacache 之于 maple tree 的分层思路）。

</details>

</details>
---
