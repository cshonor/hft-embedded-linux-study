# TLPI 第 20 章 — Signals: Fundamental Concepts

> 对应目录：`chapter-20-signals-fundamentals/`  
> 书名原文：**Signals: Fundamental Concepts**  
> ⚠️ **异步通知：** 产生 → 未决 → 递送。`SIGKILL`/`SIGSTOP` 不可捕/忽/阻。弃用 `signal()`，用 `sigaction()`（[Ch21](../chapter-21-signal-handlers/notes.md)）。

**优先级**：🔴（进程控制、daemon、可靠异步事件的理论地基）  
**前置**：[Ch19 inotify](../chapter-19-monitoring-file-events/notes.md)（同属异步事件，机制不同）  
**后置**：[Ch21 Signal Handlers](../chapter-21-signal-handlers/notes.md) · [Ch22 高级信号](../chapter-22-signals-advanced/notes.md)

---

## 章节目标

建立信号模型与生命周期；区分标准信号 vs 实时信号；掌握信号集与掩码、未决集；理解递送时机；认清 `signal()` 缺陷与 `kill`/`raise` 语义。

---

## 20.1 概述

信号 = 内核发给进程的**异步通知**。处置：

| 处置 | 行为 |
|------|------|
| 默认 | 终止 / core / 停止 / 继续 / 忽略（因信号而异） |
| 忽略 | 丢弃 |
| 捕获 | 调自定义 handler |

**例外：** `SIGKILL`、`SIGSTOP` — 不能捕获、忽略、阻塞。

### 来源

终端（`SIGINT`/`SIGTSTP`）· 内核异常（`SIGSEGV`/`SIGFPE`/`SIGPIPE`）· `kill`/`raise`/`sigqueue` · 定时器（`SIGALRM`）等。

---

## 20.2 编号与分类

| | 标准信号（约 1–31） | 实时信号（`SIGRTMIN`–`SIGRTMAX`） |
|--|---------------------|----------------------------------|
| 排队 | **不排队**；阻塞时同号只留一份 | **可排队** |
| 附加数据 | 无 | `sigqueue` 可带值 |
| 递送 | — | 小号优先；同号 FIFO |

库/业务尽量勿随意占用实时信号编号。

---

## 20.3 生命周期

1. **产生** — 内核标记目标  
2. **未决（pending）** — 已产生但被阻塞，暂不递送  
3. **递送** — 中断用户流，执行默认动作或 handler  

> 递送点：进程从**内核态返回用户态前**（典型）。不是任意用户指令中间插入。

---

## 20.4 信号掩码与未决集

掩码中的信号 = **阻塞** → 抵达后进 pending，解除后才递送。  
**掩码是线程属性**；多线程下 `sigprocmask` 影响**当前线程**。

```c
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int sig);
int sigdelset(sigset_t *set, int sig);
int sigismember(const sigset_t *set, int sig);

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
/* SIG_BLOCK | SIG_UNBLOCK | SIG_SETMASK */

int sigpending(sigset_t *set);   /* 当前线程未决集 */
```

### `sigprocmask` how

| how | 行为 |
|-----|------|
| `SIG_BLOCK` | 新掩码 = 旧 ∪ set |
| `SIG_UNBLOCK` | 新掩码 = 旧 − set |
| `SIG_SETMASK` | 新掩码 = set |

Demo：[`code/block_pending.c`](./code/block_pending.c)

---

## 20.5 `signal()` 缺陷（警示）

```c
sighandler_t signal(int sig, sighandler_t handler);
```

跨系统：是否自动复位默认、handler 期间是否阻塞自身等**不一致**；无附加数据。  
✅ 生产统一 **`sigaction()`**（Ch21）。

---

## 20.6 发送信号

```c
int kill(pid_t pid, int sig);
/* pid>0 指定进程；0 同进程组；-1 权限内全部；<-1 进程组 |pid| */
int raise(int sig);   /* 发给自己 */
```

`kill` = **发信号**，≠ 必定杀死；常规结束常用 `SIGTERM`。  
`kill(pid, 0)`：不投递，只测权限/是否存在。

Demo：[`code/kill_probe.c`](./code/kill_probe.c)

---

## 20.7 进程组与终端（基础）

PGID 标识进程组；终端 `SIGINT`/`SIGTSTP` 常打**前台进程组**。  
`SIGHUP`：终端断开；daemon 常脱离终端以免误收。

---

## 20.8 易错清单

1. `SIGKILL`/`SIGSTOP` 不可捕/忽/阻  
2. 标准信号不排队；实时信号可排队  
3. 掩码是**线程**属性  
4. 阻塞 ≠ 忽略（pending 待递送 vs 直接丢）  
5. `kill(pid,0)` 探测存活  
6. 标准信号 pending 看不出“来了几次”  
7. handler 内只调异步信号安全函数（Ch21 展开）  

---

## 速查：标准 vs 实时 · 阻塞 vs 忽略

| | 阻塞 | 忽略 |
|--|------|------|
| 抵达后 | 进 pending | 丢弃 |
| 解除后 | 会递送 | — |

| 信号类 | 排队 | 数据 |
|--------|------|------|
| 标准 | 否 | 否 |
| 实时 | 是 | 可 |

---

## 练习

1. `sigprocmask` 阻塞 `SIGINT`，Ctrl+C 暂无效  
2. `sigpending` 再解除，观察递送  
3. （选）对比 `signal()` 局限  
4. `kill(pid,0)` 探测  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 产生 → 未决 → 递送；递送在返用户态前 |
| 2 | KILL/STOP 三不能 |
| 3 | 标准不排队；实时可排队 |
| 4 | 掩码是线程的；`sigprocmask` how 三选一 |
| 5 | 阻塞留 pending；忽略直接丢 |
| 6 | 弃用 `signal()` → `sigaction` |

---

## 参考

- Kerrisk · TLPI Ch20  
- `man 7 signal` · `man 2 sigprocmask` · `man 2 kill` · `man 2 sigpending`
