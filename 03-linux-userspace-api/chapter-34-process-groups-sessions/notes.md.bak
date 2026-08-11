# TLPI 第 34 章 — Process Groups, Sessions, and Job Control

> 对应目录：`chapter-34-process-groups-sessions/`  
> （勿用 `…-jobcontrol` 等别名 — 与 [CHAPTER-MAP](../CHAPTER-MAP.md) 一致用本路径）  
> 书名原文：**Process Groups, Sessions, and Job Control**  
> ⚠️ **层级：Session → Process Group → Process。** 进程组 Leader **不能** `setsid`；守护进程标准：`fork` → 子 `setsid`。

**编号纠偏：** 大纲若写「后置 Ch35 守护进程」→ 本仓库守护进程是 **[Ch37](../chapter-37-daemons/notes.md)**；Ch35 是调度。

**优先级**：🔴（Shell 作业控制、`setsid`、SIGHUP、daemon 地基）  
**前置**：[Ch33 线程收束](../chapter-33-threads-further/notes.md) · [Ch20–22 信号](../chapter-20-signals-fundamentals/notes.md)  
**后置**：[Ch35 调度](../chapter-35-process-priorities-scheduling/notes.md) · [Ch37 Daemons](../chapter-37-daemons/notes.md)

---

## 章节目标

会话/进程组 API；控制终端与前台组；作业控制信号；SIGHUP 与孤儿进程组；为 daemon 的 `setsid` 打底。

---

## 34.1–34.2 进程组

Job ≈ 一进程组（如管道 `cmd1|cmd2|cmd3`）。  
每进程唯一 PGID；**Leader**：`PID == PGID`。组长退出 ≠ 组立刻消失（组内还有成员）。  
`fork` 默认继承 PGID。

```c
pid_t getpgrp(void);
pid_t getpgid(pid_t pid);
int setpgid(pid_t pid, pid_t pgid);
```

`setpgid` 约束：只改自身或子；子 **exec 后不能再改**；不跨会话；`setpgid(0,0)` → 自立为新组长。  
`kill(-pgid, sig)` → 整组。

Demo：[`code/print_ids.c`](./code/print_ids.c)

---

## 34.3 会话

SID；Leader：`PID == SID`。通常对应一次登录/SSH。

```c
pid_t setsid(void);
```

`setsid` 三件事：成新会话 Leader；成新进程组 Leader；**脱离原控制终端**（新会话暂无终端）。

❌ **进程组 Leader 不能 `setsid`** → 标准：`fork()`，**子进程** `setsid()`。

Demo：[`code/setsid_demo.c`](./code/setsid_demo.c)

---

## 34.4 控制终端

会话最多一控制终端；终端属一会话。  
Leader 首次打开终端（无 `O_NOCTTY`）→ 控制终端；Leader = 控制进程。  
`/dev/tty` = 控制终端别名；无终端 → `ENXIO`。  
`ioctl(…, TIOCNOTTY)` 解除；控制进程解除可连锁 `SIGHUP`。

---

## 34.5 前台 / 后台

| | |
|--|--|
| 前台组 | 独占终端读写；Ctrl+C/\\ 打前台组 |
| 后台组 | 读写终端受限 |

```c
pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgid);
```

后台读/写终端 → `SIGTTIN` / `SIGTTOU`（作业控制下）。

---

## 34.6 `SIGHUP`

默认：终止。典型：

1. **控制进程退出** → 向前台组发 `SIGHUP`；会话失终端  
2. 关终端 → shell 收 `SIGHUP`，常转发作业  
3. **孤儿进程组**内有停止进程 → 组内 `SIGHUP` 再 `SIGCONT`  

`nohup`：忽略 `SIGHUP`。  
挂断 ≠ 仅「网络断」；终端关闭/控制进程死都会走相关路径。

---

## 34.7 作业控制

Job = 进程组；`fg`/`bg`/`Ctrl+Z` 调前台组与停止/继续。

| 信号 | 典型 | 默认 |
|------|------|------|
| `SIGTSTP` | Ctrl+Z | 停止 |
| `SIGCONT` | fg/bg | 继续 |
| `SIGTTIN` | 后台读终端 | 停止 |
| `SIGTTOU` | 后台写终端 | 停止 |

### 孤儿进程组

组内成员的父都不在本组、也不在同会话（无人「管」）。  
若有停止成员：内核发 `SIGHUP`+`SIGCONT`，避免永停无人唤醒。

```text
Session (SID ≈ login shell)
├─ foreground PG  (job)
├─ background PG…
└─ controlling tty /dev/pts/N
```

---

## 易错清单

1. Leader 禁 `setsid` → fork 后子调  
2. exec 后不能再 `setpgid` 该子  
3. `kill(-pgid)` 整组；`kill(pid)` 单个  
4. Ctrl+C 只打**前台**组  
5. daemon：双重 fork + `setsid`（Ch37）  

---

## 实验清单

1. 打印 PID/PGID/SID  
2. fork + 子 `setsid`，对比父子 SID  
3. （选）Shell 下 Ctrl+Z / fg 观察  
4. （选）`kill(-pgid, …)`  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | Session ⊃ Process Group ⊃ Process |
| 2 | Leader：PID==PGID / PID==SID |
| 3 | `setsid`：新会话+新组+无终端；组长禁调 |
| 4 | 前台组吃终端信号；后台 TTIN/TTOU |
| 5 | SIGHUP：控制进程死 / 关终端 / 孤儿停组 |
| 6 | 孤儿组：SIGHUP+SIGCONT 防永停 |

---

## 参考

- Kerrisk · TLPI Ch34  
- `man 2 setpgid` · `man 2 setsid` · `man 3 tcgetpgrp` · `man 7 signal`（作业控制信号）
