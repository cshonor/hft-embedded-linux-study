# TLPI 第 52 章 — POSIX Message Queues

> 对应目录：`chapter-52-posix-message-queues/`  
> 书名原文：**POSIX Message Queues**  
> ⚠️ **高优先级先收**；不能像 SysV 按 type 挑选。`mq_notify` 仅在**空→第一条**触发，且**一次失效**须重注册。Linux `mqd_t` 可 epoll；**勿与 mq_notify 混用**。收缓冲须 ≥ `mq_msgsize`。链接常需 `-lrt`。

**优先级**：🔴（epoll、优先级、notify）  
**前置**：[Ch51 POSIX IPC 导论](../chapter-51-posix-ipc-intro/notes.md)  
**后置**：[Ch53 POSIX 信号量](../chapter-53-posix-semaphores/notes.md)

---

## 章节目标

`mq_open`/`send`/`receive`/`close`/`unlink`；属性；优先级；`mq_notify`；epoll；vs SysV mq。

---

## 52.1 特性

- 消息边界；优先级（数值越大越高；同级 FIFO）  
- `mqd_t` ≈ fd → Linux **select/poll/epoll**  
- unlink 删名；**全 close 才销毁**（丢残留消息）  
- 创建时固定 `mq_maxmsg` / `mq_msgsize`，运行期不可改  

---

## 52.2 API 要点

```c
mqd_t mq_open(const char *name, int oflag, /* mode, attr if O_CREAT */);
int mq_send(mqd_t, const char *ptr, size_t len, unsigned prio);
ssize_t mq_receive(mqd_t, char *ptr, size_t len, unsigned *prio);
int mq_close(mqd_t);  int mq_unlink(const char *name);
```

- 名：`/myqueue`；`O_NONBLOCK` 挂在**描述符**上  
- `msg_len` ≤ `mq_msgsize` 否则 `EMSGSIZE`；0 长合法  
- `mq_setattr` **只能改** `mq_flags`（NONBLOCK）  
- timed*：超时为 **绝对时间**（`CLOCK_REALTIME`）

`mq_getattr`：可读 `mq_curmsgs` 等。

Demo：[`code/`](./code/)

---

## 52.3 fork / exec

| | |
|--|--|
| `fork` | 复制 `mqd_t`；**不**继承 `mq_notify` |
| `exec` / 退出 | 自动 `mq_close`；清除 notify |

---

## 52.7 `mq_notify`（难点）

空队列收到**第一条**消息时通知一次。

1. 每队列同时只一个注册者（新盖旧）  
2. 触发后**自动注销** → 须再 `mq_notify`  
3. 队列非空时再发**不**触发  

模式：`SIGEV_SIGNAL` / `SIGEV_THREAD` / `SIGEV_NONE`。  
标准环：注册 → 等信号 → **立刻重注册** → 循环 `mq_receive` 抽干。

---

## 52.8 epoll（Linux）

有可读消息则 `mqd_t` 可读。规范未强制；**工程上 notify 与 epoll 二选一**。

---

## 52.9 限额

`/proc/sys/fs/mqueue/`：`msg_max`、`msgsize_max`、`queues_max`。  
另：`RLIMIT_MSGQUEUE`。

---

## vs System V mq

| | SysV | POSIX |
|--|------|-------|
| 句柄 | 非 fd | fd / epoll |
| 筛选 | mtype | 仅最高优先级 |
| 非阻塞 | 每次 IPC_NOWAIT | 描述符 O_NONBLOCK |
| 删除 | IPC_RMID | unlink+引用计数 |
| 通知 / 超时 | 弱 | notify + timed* |

新事件驱动优先 POSIX；要按类型挑消息仍看 SysV / 自建协议。

---

## 陷阱

1. receive 缓冲 < msgsize → 失败  
2. notify 一次性 + 仅空→首条  
3. notify ∥ epoll 竞争  
4. timed 用绝对时间  
5. 名不能 `/a/b`  
6. setattr 改不了容量  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 优先级收；无 type 筛选 |
| 2 | mqd_t 可 epoll（Linux） |
| 3 | unlink / 全 close 销毁 |
| 4 | notify：空→首条；一次失效 |
| 5 | maxmsg/msgsize 创建固定 |
| 6 | 常 `-lrt`；timed 绝对时钟 |

---

## 参考

- Kerrisk · TLPI Ch52  
- `man 3 mq_open` · `mq_notify` · `man 7 mq_overview`
