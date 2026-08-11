# TLPI 第 37 章 — Daemons

**优先级**：🔴（后台服务、嵌入式常驻进程）  
**前置**：[Ch34 会话/`setsid`](../chapter-34-process-groups-sessions/notes.md) · [Ch36 rlimit](../chapter-36-process-resources/notes.md)  
**后置**：[Ch38 特权程序安全](../chapter-38-secure-privileged/notes.md)

---

## 小节目录

- [37.1 特征](./notes/37.1-characteristics.md)
- [37.2 标准 7 步（及原因）](./notes/37.2-section-37-2.md)
- [37.3 编写规范](./notes/37.3-standard.md)
- [37.4 `SIGHUP` 热重载](./notes/37.4-sighup.md)
- [37.5 syslog](./notes/37.5-syslog.md)

---

## 章节目标


守护特征；标准守护化步骤与 `becomeDaemon()`；syslog；`SIGHUP`/`SIGTERM`；PID 文件单实例；对比 `daemon()`。

---


---

## 易错清单


1. 只 fork 一次  
2. 不 `chdir`  
3. 只 close 012 不重定向  
4. SIGHUP handler 里做重 IO  
5. 靠 `daemon()`  
6. 无 PID 锁多实例  
7. 依赖 stdout  

---


---

## 实验清单


1. 双重 fork 后查 SID/无 tty  
2. `becomeDaemon`  
3. syslog  
4. SIGHUP 标志位重载  
5. （选）PID 文件锁  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 无终端；双重 fork + setsid |
| 2 | 二次 fork：非会话首，防再抢 tty |
| 3 | chdir + 关 fd + 012→null |
| 4 | SIGHUP=重载；SIGTERM=退出 |
| 5 | 日志用 syslog |
| 6 | 自写 becomeDaemon，慎用 daemon() |

---


---

## 参考


- Kerrisk · TLPI Ch37  
- `man 3 daemon` · `man 3 syslog` · `man 2 setsid`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
