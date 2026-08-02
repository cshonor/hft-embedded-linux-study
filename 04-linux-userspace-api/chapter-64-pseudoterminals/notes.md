# TLPI 第 64 章 — Pseudoterminals

> 对应目录：`chapter-64-pseudoterminals/`  
> 书名原文：**Pseudoterminals**  
> ⚠️ **Slave 才是「真终端」**（`isatty`、termios、Ctrl+C）。Master 是透明通道。打开序：`posix_openpt → grantpt → unlockpt → open(slave)`。子进程：`setsid` + dup2(slave) 才能当控制终端。交互 shell **必须 PTY，不能用 pipe**。  
> [CHAPTER-MAP](../CHAPTER-MAP.md) 中本书主线至此为最后一章。

**优先级**：🔴（ssh / 终端模拟器 / expect）  
**前置**：[Ch62 Terminals](../chapter-62-terminals/notes.md) · [Ch63 Alternative I/O](../chapter-63-alternative-io/notes.md)  
**后置**：地图内 TLPI 主线结束；附录/其他模块另见仓库路线

---

## 章节目标

主从模型；vs 管道；POSIX 打开流程；fork/setsid 架构；包模式与 winsize；BSD 旧式了解。

---

## 64.1–64.2 概念

一对虚拟字符设备：  
**master**（`/dev/ptmx`）↔ **slave**（`/dev/pts/N`）。  
写 master → 到 slave；写 slave → 到 master。  
Slave 走终端子系统（规范模式、ECHO、SIGINT…）。  

场景：xterm、sshd、tmux、expect、`script`。

---

## 64.3 vs 管道

| | Pipe | PTY slave |
|--|------|-----------|
| `isatty` | false | **true** |
| termios / 信号 / 作业控制 | 无 | **有** |

纯字节流 → pipe；跑交互 shell → **PTY**。

---

## 64.4–64.5 POSIX 打开 · 典型架构

```c
mfd = posix_openpt(O_RDWR | O_NOCTTY);
grantpt(mfd); unlockpt(mfd);
slave = ptsname(mfd);
sfd = open(slave, O_RDWR | O_NOCTTY);
```

父持 master；子：`close(master)` → `setsid()` → dup2(slave, 0/1/2) → `exec` shell。  
`setsid` 后打开 slave → 常成为**控制终端**。  
**termios 只改 slave**，勿在 master 上改。

Demo：[`code/`](./code/)

---

## 64.6–64.7 特性

- 内核处理在 **slave** 侧  
- `TIOCPKT`：master 收状态事件前缀（SSH 等）  
- `TIOCGWINSZ` / `TIOCSWINSZ`：窗尺寸；改后子进程可收 **`SIGWINCH`**

---

## 64.9 BSD PTY

`/dev/ptyXX`+`/dev/ttyXX`：数量固定、扫描空闲 — **新项目禁用**，用 `posix_openpt`。

---

## 陷阱

1. 忘 `unlockpt`  
2. 无 `setsid` → 无控制终端 / 作业控制失效  
3. 在 master 调 tcgetattr  
4. 子未关 master → PTY 不销毁  
5. 用 pipe 冒充交互终端  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | master↔slave；slave=终端 |
| 2 | openpt→grant→unlock→open |
| 3 | 子 setsid+dup2(slave)+exec |
| 4 | 交互 shell 必须 PTY |
| 5 | winsize → SIGWINCH |
| 6 | 禁用 BSD 固定 pty 对 |

---

## 参考

- Kerrisk · TLPI Ch64  
- `man 3 posix_openpt` · `ptsname` · `man 4 pts`
