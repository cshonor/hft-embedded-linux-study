# TLPI 第 44 章 — Pipes and FIFOs

> 对应目录：`chapter-44-pipes-fifos/`  
> 书名原文：**Pipes and FIFOs**  
> ⚠️ **忘关无用写端 → 读端永远等不到 EOF。** 无读端写触发 **SIGPIPE**/`EPIPE`。FIFO 路径只是入口，数据在内核，不落盘。后置是 [Ch45 SysV IPC 导论](../chapter-45-sysv-ipc-intro/notes.md)（非直接跳消息队列）。

**优先级**：🔴（最老 IPC、shell `|` 原理）  
**前置**：[Ch43 IPC 综述](../chapter-43-ipc-overview/notes.md)  
**后置**：[Ch45 System V IPC 导论](../chapter-45-sysv-ipc-intro/notes.md)

---

## 章节目标

`pipe`/`mkfifo`；读写阻塞与 EOF/SIGPIPE；`PIPE_BUF` 原子写；`popen` 风险；stdio 缓冲；FIFO `open` 配对；局限与选型。

---

## 44.1 Pipe vs FIFO

| | 匿名管道 | FIFO |
|--|----------|------|
| 创建 | `pipe(fd[2])` | `mkfifo` / `mkfifo(3)` |
| 名称 | 无 | 路径（`ls` 类型 `p`） |
| 进程 | 仅亲缘（继承 fd） | 任意本地（路径 + 权限） |
| 打开 | 创建即有两端 fd | 读写双方互相等待（默认可阻塞） |
| IO | 半双工字节流 | 同左 |
| 持久 | 进程持久（全 close 毁） | 节点可留；缓冲仍进程/内核语义 |

FIFO 文件名 ≠ 磁盘数据；缓冲在内核。

---

## 44.2 `pipe()` 范式

```c
int fd[2];
pipe(fd);          /* fd[0] 读, fd[1] 写 */
fork();
/* 父写子读：父 close(fd[0]); 子 close(fd[1]); */
```

双向 → 两根管道。通信完 close。

Demo：[`code/pipe_demo.c`](./code/pipe_demo.c)

---

## 44.3 阻塞读写规则

**read**

| 条件 | 行为 |
|------|------|
| 有数据 | 返回已读长度 |
| 空且仍有写端 | 阻塞 |
| 空且写端全关 | **0（EOF）** |

**write**

| 条件 | 行为 |
|------|------|
| 有空间 | 写入 |
| 缓冲满 | 阻塞 |
| 读端全关 | **SIGPIPE**（默认同亡）；忽略则 `-1`/`EPIPE` |

---

## 44.4 `PIPE_BUF` vs 容量

| | |
|--|--|
| **PIPE_BUF** | Linux 常 4096；`≤` 此长度的 write **原子**（多写者不交错） |
| **Pipe capacity** | 总缓冲（Linux 常约 64KB）；满则阻塞 |

`> PIPE_BUF` 不保证原子。

---

## 44.5–44.6 `O_NONBLOCK` · `popen`

非阻塞：空读/`写满` → `-1`/`EAGAIN`（细节见 `man 7 pipe`）。

```c
FILE *popen(const char *command, const char *type);  /* "r"|"w" */
int pclose(FILE *stream);
```

内部 fork+pipe+shell → **注入风险**；单向；难控 fd 标志。替代：自管 `pipe`+`fork`+`exec`（无 shell）。

---

## 44.6 管道 + stdio

管道上 stdout 多为**块缓冲** → `printf` 不立刻进管道。  
`fflush` / `setvbuf`。

---

## 44.7–44.8 FIFO

```c
mkfifo(path, mode);
open(path, O_RDONLY);   /* 阻塞到有写端 */
open(path, O_WRONLY);   /* 阻塞到有读端 */
```

`O_NONBLOCK`：`O_RDONLY` 常立刻成功；`O_WRONLY` 无读端 → **`ENXIO`**。  
勿依赖 `O_RDWR` 开 FIFO（可移植性差）。

简单 C/S：一服务端读 FIFO，多客户端写 — 并发写仍可能交织（≤`PIPE_BUF` 可原子）。

Demo：[`code/fifo_demo.c`](./code/fifo_demo.c) + README 双进程用法。

---

## 44.9 局限

半双工；无消息边界；仅本机；匿名仅亲缘；无优先级。复杂并发 → socket / mq / shm+sync。

---

## 思考题要点

1. 不关写端 → 读不到 EOF，永久阻塞。  
2. 无读端写 → SIGPIPE/`EPIPE`；可忽略 SIGPIPE 并查 `EPIPE`。  
3. `PIPE_BUF`≠总容量。  
4. 消息 ≤`PIPE_BUF` 或外层加锁/协议。  
5. FIFO `O_WRONLY|O_NONBLOCK` → `ENXIO`。  
6. `popen` 走 shell；改自行 exec。

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

## 参考

- Kerrisk · TLPI Ch44（非 Ch18）  
- `man 2 pipe` · `man 3 mkfifo` · `man 3 popen` · `man 7 pipe`
