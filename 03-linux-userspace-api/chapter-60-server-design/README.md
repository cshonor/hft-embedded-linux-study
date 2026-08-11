# TLPI 第 60 章 — Sockets: Server Design

**优先级**：🔴（TCP 服务架构选型）  
**前置**：[Ch59 Internet Domains](../chapter-59-internet-domains/notes.md)  
**后置**：[Ch61 Socket Advanced](../chapter-61-sockets-advanced/notes.md)

---

## 小节目录

- [60.1 迭代服务器](./notes/60.1-server-iterate.md)
- [60.2 –60.3 fork 并发](./notes/60.2-fork.md)
- [60.4 多线程](./notes/60.4-thread.md)
- [60.5 事件驱动（select/poll 入门）](./notes/60.5-select-poll.md)
- [60.6 工程问题](./notes/60.6-section-60-6.md)
- [60.7 对比](./notes/60.7-comparison.md)

---

## 章节目标


迭代 / fork / pthread / select·poll；僵尸与 fd 纪律；`SO_REUSEADDR`；惊群；四模型对比。

---


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


---

## 参考


- Kerrisk · TLPI Ch60（非「第 53 章」误标）  
- `man 2 accept` · `waitpid` · `setsockopt`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
