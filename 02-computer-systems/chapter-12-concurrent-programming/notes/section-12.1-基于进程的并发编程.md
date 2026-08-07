## 12.1 基于进程的并发编程

> **Ch12 §12.1** · [章导读](../README.md) · 上节 — · 下节 [§12.2 →](./section-12.2-基于I-O多路复用的并发编程.md)

---

#### 12.1.1 基于进程的并发服务器

```c
while (1) {
    connfd = accept(listenfd, ...);
    if ((pid = fork()) == 0) {   // 子进程
        close(listenfd);
        echo(connfd);
        exit(0);
    }
    close(connfd);               // 父进程关掉子进程用的 fd
}
```

- **`fork`** — 子进程复制地址空间；`execve` 可换程序（Tiny 动态内容用过）
- **`SIGCHLD` + `waitpid`** — 回收僵尸进程（→ [Ch 8](../../chapter-08-exceptional-control-flow/)）
- 父子共享 **已打开 fd 表项** — 必须 `close` 不需要的 fd

#### 12.1.2 进程的优缺点

| 优点 | 缺点 |
|------|------|
| 隔离好，一个崩不影响其他 | `fork` + 复制页表 **开销大** |
| 可跑不同程序 | 进程间共享数据麻烦（IPC） |

**HFT：** 每连接 `fork` **不适合** 低延迟网关；更适合 **隔离** 场景（沙箱、子进程跑脚本）。

---

### 常见陷阱
1. **fork 复制页表不复制物理页（COW）** — 开销仍比线程大（复制页表结构 + 内核数据结构）
2. **父子必须关闭不需要的 fd** — fork 后父子共享 fd 表项，不关会导致 fd 泄漏和连接无法正常关闭
3. **进程隔离好但共享数据难** — 线程共享地址空间天然方便，进程需 IPC（管道/共享内存/消息队列）

### 自测题

<details>
<summary>Q1: 基于 fork 的并发服务器中，父子进程分别关闭哪些 fd？为什么？</summary>

子进程关闭 listenfd（不负责 accept），父进程关闭 connfd（不负责 echo）。fork 后父子共享 fd 表项，不关会导致 fd 泄漏和引用计数无法归零。

</details>

<details>
<summary>Q2: fork 后父子进程的地址空间关系？开销在哪里？</summary>

COW 机制：fork 复制页表但不复制物理页，首次写才分配新页。开销在复制页表结构（mm_struct、VMA、页表项）和内核进程控制块（task_struct）。

</details>

<details>
<summary>Q3: 为什么 HFT 不用 fork 做并发网关？</summary>

fork 开销大（微秒级），且进程间共享数据需 IPC（额外延迟）。HFT 用线程池或单线程 epoll reactor，避免进程创建/切换开销。fork 更适合隔离场景（沙箱、子进程跑脚本）。

</details>

<details>
<summary>Q4: SIGCHLD 和 waitpid 在并发服务器中的作用？</summary>

子进程退出后变成僵尸进程（Z 状态），占用 PID 和少量内核资源。父进程捕获 SIGCHLD 信号后调 waitpid 回收子进程，释放资源。不回收会导致 PID 耗尽。

</details>

---

← — · [本章导读](../README.md) · [§12.2 →](./section-12.2-基于I-O多路复用的并发编程.md)
