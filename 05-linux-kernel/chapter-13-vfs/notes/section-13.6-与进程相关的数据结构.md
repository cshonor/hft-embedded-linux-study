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

</details>
---
