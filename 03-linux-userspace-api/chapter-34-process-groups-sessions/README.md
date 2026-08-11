# TLPI 第 34 章 — Process Groups, Sessions, and Job Control

**优先级**：🔴（Shell 作业控制、`setsid`、SIGHUP、daemon 地基）  
**前置**：[Ch33 线程收束](../chapter-33-threads-further/notes.md) · [Ch20–22 信号](../chapter-20-signals-fundamentals/notes.md)  
**后置**：[Ch35 调度](../chapter-35-process-priorities-scheduling/notes.md) · [Ch37 Daemons](../chapter-37-daemons/notes.md)

---

## 小节目录

- [34.1 –34.2 进程组](./notes/34.1-process-group.md)
- [34.3 会话](./notes/34.3-section-34-3.md)
- [34.4 控制终端](./notes/34.4-terminal.md)
- [34.5 前台 / 后台](./notes/34.5-section-34-5.md)
- [34.6 `SIGHUP`](./notes/34.6-sighup.md)
- [34.7 作业控制](./notes/34.7-section-34-7.md)

---

## 章节目标


会话/进程组 API；控制终端与前台组；作业控制信号；SIGHUP 与孤儿进程组；为 daemon 的 `setsid` 打底。

---


---

## 易错清单


1. Leader 禁 `setsid` → fork 后子调  
2. exec 后不能再 `setpgid` 该子  
3. `kill(-pgid)` 整组；`kill(pid)` 单个  
4. Ctrl+C 只打**前台**组  
5. daemon：双重 fork + `setsid`（Ch37）  

---


---

## 实验清单


1. 打印 PID/PGID/SID  
2. fork + 子 `setsid`，对比父子 SID  
3. （选）Shell 下 Ctrl+Z / fg 观察  
4. （选）`kill(-pgid, …)`  

---


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


---

## 参考


- Kerrisk · TLPI Ch34  
- `man 2 setpgid` · `man 2 setsid` · `man 3 tcgetpgrp` · `man 7 signal`（作业控制信号）


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
