## 12.1–12.2 进程与 I/O 多路复用

### 12.1 基于进程的并发编程

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

### 12.2 基于 I/O 多路复用的并发编程

#### 12.2.1 事件驱动服务器（`select`）

```c
FD_ZERO(&read_set);
FD_SET(listenfd, &read_set);
while (1) {
    FD_SET(listenfd, &read_set);
    for (i = 0; i < maxi; i++)
        if (connfd[i] >= 0) FD_SET(connfd[i], &read_set);
    select(maxfd + 1, &read_set, NULL, NULL, NULL);
    if (FD_ISSET(listenfd, &read_set)) { /* accept */ }
    for (i = 0; i < maxi; i++)
        if (connfd[i] >= 0 && FD_ISSET(connfd[i], &read_set))
            echo(connfd[i]);
}
```

- **单线程** 同时监视多个 fd — **就绪** 才 `read`/`write`
- 配合 **非阻塞 I/O** 避免一个慢客户端堵死全体

#### 12.2.2 I/O 多路复用的优缺点

| 优点 | 缺点 |
|------|------|
| 无进程/线程切换开销 | 代码复杂；长 CPU 计算会阻塞事件循环 |
| 无锁共享数据结构（单线程） | `select` O(n)；Linux 生产用 **`epoll`** |

**HFT：** **reactor 模式** 主流 — 单（或每核）事件循环 + 非阻塞 socket；行情 feed、会话管理常如此。→ [09-UNP](../../../09-UNP-Vol1/) · [Ch 11](../../chapter-11-network-programming/)

---

← [本章导读](../README.md)
