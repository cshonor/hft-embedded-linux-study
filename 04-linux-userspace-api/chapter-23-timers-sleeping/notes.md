# TLPI 第 23 章 — Timers and Sleeping

> 对应目录：`chapter-23-timers-sleeping/`  
> 书名原文：**Timers and Sleeping**  
> ⚠️ **`sleep`/`alarm`/`setitimer(ITIMER_REAL)` 共享 `SIGALRM`，勿混用。** 休眠首选 `nanosleep`；定时首选 POSIX `timer_create` + `CLOCK_MONOTONIC`。

**优先级**：🔴（超时、周期任务、与信号/EINTR 交互）  
**前置**：[Ch22 信号高级](../chapter-22-signals-advanced/notes.md)  
**后置**：[Ch24 Process Creation](../chapter-24-process-creation/notes.md) · [Ch63 多路 I/O](../chapter-63-alternative-io/notes.md)

---

## 章节目标

区分休眠 / 间隔定时器 / POSIX 定时器；认清 `SIGALRM` 冲突；掌握 `CLOCK_REALTIME` vs `CLOCK_MONOTONIC`；会处理休眠被信号打断（`EINTR`）。

---

## 23.1 休眠接口

### `sleep` / `usleep`（少用）

| API | 问题 |
|-----|------|
| `sleep` | 内部常走 `alarm`+`SIGALRM`，与其它 `alarm` 冲突；打断返回剩余秒数 |
| `usleep` | 已废弃；新代码用 `nanosleep` |

### `nanosleep`（推荐短休眠）

```c
int nanosleep(const struct timespec *req, struct timespec *rem);
```

- **不占 `SIGALRM`**  
- 中断：`-1`/`EINTR`，`rem` 剩余时间可重试  
- 线程友好  

Demo：[`code/nanosleep_retry.c`](./code/nanosleep_retry.c)

### `clock_nanosleep`

```c
int clock_nanosleep(clockid_t clock_id, int flags,
                    const struct timespec *req, struct timespec *rem);
```

| flags | 含义 |
|-------|------|
| `0` | 相对休眠（≈ `nanosleep`，常等价 `CLOCK_REALTIME` 相对） |
| `TIMER_ABSTIME` | 等到绝对时刻 |

定时/超时逻辑优先 **`CLOCK_MONOTONIC`**（不受墙钟跳变影响）。

---

## 23.2 `setitimer` / `alarm`

```c
int setitimer(int which, const struct itimerval *new_val,
              struct itimerval *old_val);
unsigned int alarm(unsigned int seconds);  /* 简化版 ITIMER_REAL */
```

| which | 计时 | 信号 |
|-------|------|------|
| `ITIMER_REAL` | 墙钟 | `SIGALRM` |
| `ITIMER_VIRTUAL` | 用户态 CPU | `SIGVTALRM` |
| `ITIMER_PROF` | 用户+内核 CPU | `SIGPROF` |

`it_value` 启动/停止；`it_interval` 为 0 则单次。  
每类 **which 全局一个**；依赖信号、标准信号不排队 → 新代码优先 POSIX 定时器。

---

## 23.3 POSIX 定时器 `timer_create`

```c
int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
int timer_settime(timer_t timerid, int flags,
                  const struct itimerspec *new_val, struct itimerspec *old_val);
int timer_delete(timer_t timerid);
```

### `sigevent` 通知

| 方式 | 要点 |
|------|------|
| `SIGEV_SIGNAL` | 指定信号 + 可带数据；多定时器可并存，但仍受信号排队限制 |
| `SIGEV_THREAD` | 到期调线程回调，**无 async-safe 限制** |
| `SIGEV_NONE` | 只计数 |

优势：多实例、可选单调钟、纳秒 `itimerspec`。  
链接：老 glibc 可能需 `-lrt`。

Demo：[`code/posix_timer_thread.c`](./code/posix_timer_thread.c)

---

## 23.4 时钟类型

| 时钟 | 用途 |
|------|------|
| `CLOCK_REALTIME` | 墙钟；NTP 可跳；**不宜**做间隔超时 |
| `CLOCK_MONOTONIC` | 单调递增；**定时/超时首选** |
| `*_CPUTIME_ID` | CPU 时间；不是墙钟延时 |

---

## 23.5 休眠与 `EINTR`

相对休眠被信号打断 → `EINTR`；`nanosleep` 用 `rem` 重试：

```c
while (nanosleep(&req, &req) == -1 && errno == EINTR)
    ;
```

`TIMER_ABSTIME`：打断后通常**重算绝对时间**，不是简单用 rem。

---

## 23.6 选型速查

| API | 粒度 | 靠信号？ | 多实例 | 场景 |
|-----|------|----------|--------|------|
| `sleep` | 秒 | 常 `SIGALRM` | ❌ | 尽量不用 |
| `nanosleep` | 纳秒 | 否 | ✅ | **短休眠首选** |
| `setitimer` | 微秒 | 是 | 每 which 一个 | 遗留 |
| `timer_create` | 纳秒 | 可选 | ✅ | **周期/多定时器** |
| `clock_nanosleep` | 纳秒 | 否 | ✅ | 绝对时间 / 单调钟 |

---

## 23.7 易错清单

1. `alarm`/`sleep`/`ITIMER_REAL` 冲突  
2. 墙钟做定时 → NTP 回拨/快进灾难  
3. `SIGEV_SIGNAL` + 标准信号仍可能丢事件；高频用线程通知慎开销  
4. 定时器非硬实时，有调度延迟  
5. `fork`/`exec` 对定时器继承/清除有特殊规则（见手册）  

---

## 练习

1. `nanosleep` + `EINTR` 重试  
2. （选）`setitimer` 周期 `SIGALRM`  
3. `timer_create` + `CLOCK_MONOTONIC` + `SIGEV_THREAD`  
4. （选）对比 REALTIME / MONOTONIC 在时间跳变下行为  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 休眠用 `nanosleep`，勿混 `alarm`/`sleep` |
| 2 | 定时用 `timer_create`；多实例 + 可选线程通知 |
| 3 | 超时用 `CLOCK_MONOTONIC` |
| 4 | `EINTR` 用 rem 重试相对休眠 |
| 5 | `ITIMER_*` 每类全局一个，靠信号 |
| 6 | 定时器不保证硬准时 |

---

## 参考

- Kerrisk · TLPI Ch23  
- `man 2 nanosleep` · `man 2 clock_nanosleep` · `man 2 setitimer` · `man 2 timer_create` · `man 2 clock_gettime`
