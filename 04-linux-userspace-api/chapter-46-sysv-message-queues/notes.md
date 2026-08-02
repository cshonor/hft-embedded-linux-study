# TLPI 第 46 章 — System V Message Queues

> 对应目录：`chapter-46-sysv-message-queues/`  
> 书名原文：**System V Message Queues**  
> ⚠️ **面向消息 + `mtype` 筛选**；`msqid` 非 fd，无 epoll。内核持久，须 `IPC_RMID`。`msgsnd`/`msgrcv` 被信号打断 → **`EINTR`，`SA_RESTART` 不帮重启**。新项目优先 POSIX mq / Unix 域 socket。

**优先级**：🔴（首个完整 SysV 实例；消息 vs 字节流）  
**前置**：[Ch45 SysV IPC 导论](../chapter-45-sysv-ipc-intro/notes.md)  
**后置**：[Ch47 SysV 信号量](../chapter-47-sysv-semaphores/notes.md)

---

## 章节目标

`msgget`/`msgsnd`/`msgrcv`/`msgctl`；`msgtyp` 三规则；`msqid_ds` 与限额；与 pipe 对比；局限。

---

## 46.1 特性（相对 Pipe/FIFO）

| | Pipe/FIFO | SysV mq |
|--|-----------|---------|
| 模型 | 字节流 | **整条消息** |
| 筛选 | 无 | **`mtype`** |
| 句柄 | fd → epoll | msqid → **不能** |
| 持久 | 进程持久 | **内核持久** |

数据在内核缓冲，不落盘。

---

## 46.2 API

### `msgget` / `msgctl`

复用 Ch45：`IPC_CREAT|IPC_EXCL` + mode。  
`msgctl`：`IPC_STAT` / `IPC_SET` / **`IPC_RMID`**（立即标记删；残留消息丢弃；持有 id 者可继续用完）。  
⚠️ shm 的 `IPC_RMID` 是延迟到全部 `shmdt`；mq/sem 是立即标记。

### `msgsnd`

```c
struct msgbuf { long mtype; /* >0 */ char mtext[]; };
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
```

- `msgsz` = **正文长度**（不含 `mtype`）；可为 0  
- `0` 阻塞满队列；`IPC_NOWAIT` → `EAGAIN`  
- 信号中断 → `EINTR`（**无 SA_RESTART**）

### `msgrcv`（重点）

```c
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
```

| `msgtyp` | 行为 |
|----------|------|
| `0` | 队首（FIFO） |
| `>0` | 第一条 **类型等于** msgtyp |
| `<0` | 类型 **≤ \|msgtyp\|** 中 **最小类型** 的那条 |

例：队列类型 `[100,300,200,400]`，`msgtyp=-300` → 先 100，再 200，再 300；跳过 400。

| 标志 | |
|------|--|
| `IPC_NOWAIT` | 无匹配 → `ENOMSG` |
| `MSG_NOERROR` | 过长则截断；否则 `E2BIG` |

返回：正文字节数。

Demo：[`code/`](./code/)

---

## 46.3–46.5 `msqid_ds` · 限额 · 运维

关注：`msg_qnum`、`msg_qbytes`（可 `IPC_SET`）、`msg_cbytes`、权限与时间戳。

| `/proc/sys/kernel/` | 含义（量级示意） |
|---------------------|------------------|
| `msgmni` | 队列个数上限 |
| `msgmax` | 单条最大字节 |
| `msgmnb` | 单队列默认总字节 |

超限：`msgget`→`ENOSPC` 等。  
`ipcs -q` · `ipcrm -q id` / `-M key`。

---

## 46.6–46.7 模型与缺陷

多进程对同一队列 `msgrcv`：一条消息只被一个消费者拿走（简陋分发）。  
缺陷：无 epoll、易泄漏、无原生超时、本机 only、`ftok` 坑。TLPI：**新项目少用 SysV mq**。

---

## 思考题要点

1. `msgtyp` 0 / >0 / <0（上表）。  
2. `EINTR`；SA_RESTART 无效。  
3. mq RMID 立即标记 vs shm 等 detach。  
4. `MSG_NOERROR` vs `E2BIG`。  
5. id≠fd。  
6. 崩溃 + 内核持久 → `ipcrm`/`IPC_RMID`。  
7. **0 长度消息合法**。

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 消息边界 + `mtype>0` |
| 2 | `msgtyp`：0 / 等于 / ≤\|n\| 最小 |
| 3 | `msgsz` 不含 mtype；正文可 0 |
| 4 | 中断 → EINTR，无 RESTART |
| 5 | 非 fd；内核持久；IPC_RMID |
| 6 | 新项目优先 POSIX mq / UDS |

---

## 参考

- Kerrisk · TLPI Ch46（非「第 19 章」误标）  
- `man 2 msgget` · `msgsnd` · `msgrcv` · `msgctl`
