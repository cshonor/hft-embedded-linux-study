# TLPI 第 36 章 — Process Resources

**优先级**：🔴（服务调 `NOFILE`、剖析 CPU/RSS、daemon 启动设限）  
**前置**：[Ch35 调度](../chapter-35-process-priorities-scheduling/notes.md)  
**后置**：[Ch37 Daemons](../chapter-37-daemons/notes.md)

---

## 小节目录

- [36.1 `getrusage`](./notes/36.1-getrusage.md)
- [36.2 `getrlimit` / `setrlimit`](./notes/36.2-getrlimit-setrlimit.md)

---

## 章节目标


`getrusage`；软/硬 `rlimit`；超限信号/错误；fork/exec 继承；与 shell `ulimit` / systemd 的关系。

---


---

## 易错清单


1. CHILDREN 必须 wait 才进账  
2. `ru_maxrss` 是峰值  
3. 硬限降了回不去（非特权）  
4. `NOFILE` 与「最大 fd 编号」关系：大约 `cur-1`  
5. 启动时主动抬软限是常见服务套路  

---


---

## 实验清单


1. `getrusage` 看 CPU/RSS/切换  
2. CHILDREN + wait  
3. 打印默认 rlimit  
4. 抬高 `RLIMIT_NOFILE`（不超硬限）  
5. （选）硬限不可升  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | rusage：消耗；rlimit：上限 |
| 2 | 软 ≤ 硬；非特权硬限只降 |
| 3 | CHILDREN = 已 wait 子进程 |
| 4 | fork/exec 保留限制 |
| 5 | daemon 勿只靠交互 shell ulimit |
| 6 | 服务常调 NOFILE |

---


---

## 参考


- Kerrisk · TLPI Ch36  
- `man 2 getrusage` · `man 2 getrlimit` · `man 2 setrlimit` · `man 2 prlimit`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
