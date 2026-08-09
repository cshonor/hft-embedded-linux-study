# TLPI 第 37 章 — Daemons

> 对应目录：`chapter-37-daemons/`  
> 书名原文：**Daemons**  
> ⚠️ **双重 fork + `setsid`：** 第一次保证可 `setsid`；第二次使进程**不再是会话首进程**，防再绑控制终端。日志用 **syslog**；`SIGHUP` = 热重载约定。慎用 glibc `daemon()`（只有一次 fork）。

**优先级**：🔴（后台服务、嵌入式常驻进程）  
**前置**：[Ch34 会话/`setsid`](../chapter-34-process-groups-sessions/notes.md) · [Ch36 rlimit](../chapter-36-process-resources/notes.md)  
**后置**：[Ch38 特权程序安全](../chapter-38-secure-privileged/notes.md)

---

## 章节目标

守护特征；标准守护化步骤与 `becomeDaemon()`；syslog；`SIGHUP`/`SIGTERM`；PID 文件单实例；对比 `daemon()`。

---

## 37.1 特征

长期后台；**无控制终端**（`ps` TTY=`?`）；常被 init/systemd 收养；独立会话/组；日志走 syslog/文件，不依赖终端 stdio。

---

## 37.2 标准 7 步（及原因）

| 步 | 动作 | 为何 |
|----|------|------|
| 1 | `fork`，父 `_exit` | shell 归还提示符；子非组 Leader → 可 `setsid` |
| 2 | `setsid` | 新会话+新组，脱离控制终端 |
| 3 | **再 fork**，中间父退出 | 最终进程**非会话首**，难再抢终端 |
| 4 | `umask(0)` | 自控创建 mode |
| 5 | `chdir("/")`（或 `/var`） | 不挡 umount |
| 6 | 关继承 fd | 防泄漏 |
| 7 | 0/1/2 → `/dev/null` | 用 `open`+`dup2`，勿只 `close` 被抢 fd |

glibc `daemon()`：常**缺第二次 fork** → 仍可能是会话首；生产宜自写 `becomeDaemon`。

Demo：[`code/become_daemon.c`](./code/become_daemon.c) · [`code/mini_daemon.c`](./code/mini_daemon.c)

---

## 37.3 编写规范

1. **单实例**：`/run/xxx.pid` + `fcntl` 排他锁  
2. 防泄漏；启动可 `setrlimit(NOFILE…)`  
3. **SIGTERM**：快清理退出（systemd 随后可能 `SIGKILL`）  
4. **SIGHUP**：重载配置/日志（daemon 无终端，可复用此信号）  
5. 少用缓冲 stdio；优先 syslog  
6. 能降权则降权（Ch38）  

---

## 37.4 `SIGHUP` 热重载

`kill -HUP <pid>`。handler **只设** `volatile sig_atomic_t`；主循环里重载。

Demo：[`code/daemon_sighup.c`](./code/daemon_sighup.c)（演示标志位范式；可配合 becomeDaemon）

---

## 37.5 syslog

```c
openlog(ident, LOG_PID|LOG_NDELAY, LOG_DAEMON);
syslog(LOG_INFO, "msg %d", n);
closelog();
```

facility：`LOG_DAEMON`/`USER`…  
priority：`EMERG`…`DEBUG`。  
走 `/dev/log` → rsyslog 等；分级、轮转由系统管。

Demo：[`code/t_syslog.c`](./code/t_syslog.c)

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

## 实验清单

1. 双重 fork 后查 SID/无 tty  
2. `becomeDaemon`  
3. syslog  
4. SIGHUP 标志位重载  
5. （选）PID 文件锁  

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

## 参考

- Kerrisk · TLPI Ch37  
- `man 3 daemon` · `man 3 syslog` · `man 2 setsid`
