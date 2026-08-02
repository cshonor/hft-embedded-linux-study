# TLPI 第 60 章 — Sockets: Server Design

> 对应目录：`chapter-60-server-design/`  
> 书名原文：**Sockets: Server Design**  
> ⚠️ **fork 后：子关 listen、父关 conn**（否则 fd 泄漏 / 连不断）。收尸：`SIGCHLD` + `waitpid(..., WNOHANG)`。线程勿传 `&connfd` 局部变量。重启 bind：`SO_REUSEADDR`。后置按地图是 [Ch61 Advanced Topics](../chapter-61-sockets-advanced/notes.md)（含选项等；勿标成单独「Socket Options」章）。

**优先级**：🔴（TCP 服务架构选型）  
**前置**：[Ch59 Internet Domains](../chapter-59-internet-domains/notes.md)  
**后置**：[Ch61 Socket Advanced](../chapter-61-sockets-advanced/notes.md)

---

## 章节目标

迭代 / fork / pthread / select·poll；僵尸与 fd 纪律；`SO_REUSEADDR`；惊群；四模型对比。

---

## 60.1 迭代服务器

`accept → 同步处理完 → close(conn)`。无并发；慢客户端堵死全体。  
适合极低流量短请求。示例见 [Ch59 `tcp_iter_sv`](../chapter-59-internet-domains/code/)。

---

## 60.2–60.3 fork 并发

```
accept → fork
  子: close(listen); handle; close(conn); _exit
  父: close(conn); 继续 accept
```

| 要点 | |
|------|--|
| fd 引用 | fork 后 +1；双方关无用端 |
| 僵尸 | `SIGCHLD` 里循环 `waitpid(-1,NULL,WNOHANG)` |
| 隔离 | 强；崩溃不易拖垮主服务 |
| 代价 | fork/页表贵；海量连接受限 |

Demo：[`code/fork_sv.c`](./code/fork_sv.c)

---

## 60.4 多线程

`accept → pthread_create(handler)`。  
**坑**：传 `&connfd` → 主循环覆盖 → 多线程同一 fd。  
修：堆上分配 fd 副本，或加锁传递。  
优：轻、易共享状态；劣：一线程崩整进程、须加锁、线程过多调度差。进阶：线程池。

---

## 60.5 事件驱动（select/poll 入门）

单线程监视 listen + 多 conn 的可读/可写；无 per-conn 线程。  
高并发基础（C10K）；状态机复杂；select 有 fd 上限；CPU 密集堵事件环。  
**epoll** 见后文 [Ch63 Alternative I/O](../chapter-63-alternative-io/notes.md)。

---

## 60.6 工程问题

| 问题 | 处理 |
|------|------|
| `Address already in use` / TIME_WAIT | `SO_REUSEADDR`（与 `SO_REUSEPORT` 不同） |
| 惊群 | 多进程堵同一 accept 时全醒；现代内核 listen 侧已改善 |
| fd 耗尽 | `ulimit -n` / `RLIMIT_NOFILE` |
| 慢客户端 | 迭代最惨；事件环须超时踢闲置 |

---

## 60.7 对比

| 模型 | 并发 | 隔离 | 开销 | 难度 |
|------|------|------|------|------|
| 迭代 | 无 | — | 最低 | 最简 |
| fork | 中 | 强 | 高 | 中 |
| pthread | 中高 | 弱 | 中 | 中高 |
| select/poll | 高 | 单进程 | 低 | 高 |

选型：稳 → fork；共享状态中等并发 → 线程/池；高连接 → 多路复用（生产常 epoll）。

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 迭代：一慢堵全站 |
| 2 | fork：子关 listen、父关 conn |
| 3 | SIGCHLD + WNOHANG 防僵尸 |
| 4 | 线程勿传 &connfd |
| 5 | SO_REUSEADDR 利重启 |
| 6 | 高并发 → 事件驱动 / epoll |

---

## 参考

- Kerrisk · TLPI Ch60（非「第 53 章」误标）  
- `man 2 accept` · `waitpid` · `setsockopt`
