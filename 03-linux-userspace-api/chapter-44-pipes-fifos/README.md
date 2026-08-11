# TLPI 第 44 章 — Pipes and FIFOs

**优先级**：🔴（最老 IPC、shell `|` 原理）  
**前置**：[Ch43 IPC 综述](../chapter-43-ipc-overview/notes.md)  
**后置**：[Ch45 System V IPC 导论](../chapter-45-sysv-ipc-intro/notes.md)

---

## 小节目录

- [44.1 Pipe vs FIFO](./notes/44.1-pipe-fifo.md)
- [44.2 `pipe()` 范式](./notes/44.2-pipe.md)
- [44.3 阻塞读写规则](./notes/44.3-blocking.md)
- [44.4 `PIPE_BUF` vs 容量](./notes/44.4-pipebuf.md)
- [44.5 –44.6 `O_NONBLOCK` · `popen`](./notes/44.5-ononblock-popen.md)
- [44.6 管道 + stdio](./notes/44.6-stdio.md)
- [44.7 –44.8 FIFO](./notes/44.7-fifo.md)
- [44.9 局限](./notes/44.9-limitations.md)

---

## 章节目标


`pipe`/`mkfifo`；读写阻塞与 EOF/SIGPIPE；`PIPE_BUF` 原子写；`popen` 风险；stdio 缓冲；FIFO `open` 配对；局限与选型。

---


---

## 思考题要点


1. 不关写端 → 读不到 EOF，永久阻塞。  
2. 无读端写 → SIGPIPE/`EPIPE`；可忽略 SIGPIPE 并查 `EPIPE`。  
3. `PIPE_BUF`≠总容量。  
4. 消息 ≤`PIPE_BUF` 或外层加锁/协议。  
5. FIFO `O_WRONLY|O_NONBLOCK` → `ENXIO`。  
6. `popen` 走 shell；改自行 exec。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | `fd[0]` 读 / `fd[1]` 写；关无用端 |
| 2 | 写端全关 → read EOF；读端全关 → SIGPIPE |
| 3 | `PIPE_BUF` 原子写；容量另计 |
| 4 | FIFO 路径入口；数据不落盘 |
| 5 | FIFO open 读写配对；`ENXIO` |
| 6 | `popen` 有注入；管道上注意块缓冲 |

---


---

## 参考


- Kerrisk · TLPI Ch44（非 Ch18）  
- `man 2 pipe` · `man 3 mkfifo` · `man 3 popen` · `man 7 pipe`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
