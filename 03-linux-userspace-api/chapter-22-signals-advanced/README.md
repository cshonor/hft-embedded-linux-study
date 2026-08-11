# TLPI 第 22 章 — Signals: Advanced Features

**优先级**：🔴（可靠等待、实时信号、崩溃栈、服务进程信号模型）  
**前置**：[Ch21 Signal Handlers](../chapter-21-signal-handlers/notes.md)  
**后置**：[Ch23 Timers and Sleeping](../chapter-23-timers-sleeping/notes.md) · [Ch29+ 线程](../chapter-29-threads-intro/notes.md) · [Ch63 多路 I/O](../chapter-63-alternative-io/notes.md)

---

## 小节目录

- [22.1 `pause()`](./notes/22.1-pause.md)
- [22.2 `sigsuspend()`（核心）](./notes/22.2-sigsuspend.md)
- [22.3 同步等待：`sigwait` 族](./notes/22.3-sigwait.md)
- [22.4 实时信号与 `sigqueue`](./notes/22.4-sigqueue.md)
- [22.5 备用信号栈](./notes/22.5-signal-stack.md)
- [22.6 `prctl`（Linux）](./notes/22.6-prctl.md)
- [22.7 `EINTR` 再强调](./notes/22.7-eintr.md)
- [22.8 三大等待对比](./notes/22.8-wait-comparison.md)

---

## 章节目标


掌握 `pause`/`sigsuspend`、同步 `sigwait*`、`sigqueue` 实时信号、备用信号栈、`PR_SET_PDEATHSIG`；对比三种等待模型。

---


---

## 22.9 易错清单


1. `sigsuspend` 只改**当前线程**掩码  
2. `sigwait` 前必须阻塞目标信号  
3. `sival_ptr` 跨进程无效  
4. 信号栈用够 `SIGSTKSZ`；`SA_ONSTACK` 按信号注册  
5. `PR_SET_PDEATHSIG` 只盯直接父  
6. `sigqueue` 队列满要处理失败  

---


---

## 练习


1. 复现 `pause` 竞态，用 `sigsuspend` 修  
2. `sigwaitinfo` 循环取信号  
3. `sigqueue` 传 int  
4. （选）`sigaltstack` + `SA_ONSTACK`  
5. （选）`PR_SET_PDEATHSIG`  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | `pause` 竞态；用 `sigsuspend` |
| 2 | `sigsuspend` = 换掩码 + 睡（原子） |
| 3 | 服务端：阻塞 + `sigwaitinfo` 线程 |
| 4 | `sigqueue` + `SA_SIGINFO` 传数据；跨进程用 int |
| 5 | 栈溢出收尸用 `sigaltstack` + `SA_ONSTACK` |
| 6 | 勿迷信 `SA_RESTART`；处理 `EINTR` |

---


---

## 参考


- Kerrisk · TLPI Ch22  
- `man 2 sigsuspend` · `man 3 sigwaitinfo` · `man 3 sigqueue` · `man 2 sigaltstack` · `man 2 prctl`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
