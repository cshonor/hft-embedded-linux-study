# TLPI 第 64 章 — Pseudoterminals

**优先级**：🔴（ssh / 终端模拟器 / expect）  
**前置**：[Ch62 Terminals](../chapter-62-terminals/notes.md) · [Ch63 Alternative I/O](../chapter-63-alternative-io/notes.md)  
**后置**：地图内 TLPI 主线结束；附录/其他模块另见仓库路线

---

## 小节目录

- [64.1 –64.2 概念](./notes/64.1-concepts.md)
- [64.3 vs 管道](./notes/64.3-pipe.md)
- [64.4 –64.5 POSIX 打开 · 典型架构](./notes/64.4-architecture.md)
- [64.6 –64.7 特性](./notes/64.6-section-64-6.md)
- [64.9 BSD PTY](./notes/64.9-bsd-pty.md)

---

## 章节目标


主从模型；vs 管道；POSIX 打开流程；fork/setsid 架构；包模式与 winsize；BSD 旧式了解。

---


---

## 陷阱


1. 忘 `unlockpt`  
2. 无 `setsid` → 无控制终端 / 作业控制失效  
3. 在 master 调 tcgetattr  
4. 子未关 master → PTY 不销毁  
5. 用 pipe 冒充交互终端  

---


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


---

## 参考


- Kerrisk · TLPI Ch64  
- `man 3 posix_openpt` · `ptsname` · `man 4 pts`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
