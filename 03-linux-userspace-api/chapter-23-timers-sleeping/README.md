# TLPI 第 23 章 — Timers and Sleeping

**优先级**：🔴（超时、周期任务、与信号/EINTR 交互）  
**前置**：[Ch22 信号高级](../chapter-22-signals-advanced/notes.md)  
**后置**：[Ch24 Process Creation](../chapter-24-process-creation/notes.md) · [Ch63 多路 I/O](../chapter-63-alternative-io/notes.md)

---

## 小节目录

- [23.1 休眠接口](./notes/23.1-section-23-1.md)
- [23.2 `setitimer` / `alarm`](./notes/23.2-setitimer-alarm.md)
- [23.3 POSIX 定时器 `timer_create`](./notes/23.3-timercreate.md)
- [23.4 时钟类型](./notes/23.4-clock-types.md)
- [23.5 休眠与 `EINTR`](./notes/23.5-eintr.md)

---

## 章节目标


区分休眠 / 间隔定时器 / POSIX 定时器；认清 `SIGALRM` 冲突；掌握 `CLOCK_REALTIME` vs `CLOCK_MONOTONIC`；会处理休眠被信号打断（`EINTR`）。

---


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


---

## 23.7 易错清单


1. `alarm`/`sleep`/`ITIMER_REAL` 冲突  
2. 墙钟做定时 → NTP 回拨/快进灾难  
3. `SIGEV_SIGNAL` + 标准信号仍可能丢事件；高频用线程通知慎开销  
4. 定时器非硬实时，有调度延迟  
5. `fork`/`exec` 对定时器继承/清除有特殊规则（见手册）  

---


---

## 练习


1. `nanosleep` + `EINTR` 重试  
2. （选）`setitimer` 周期 `SIGALRM`  
3. `timer_create` + `CLOCK_MONOTONIC` + `SIGEV_THREAD`  
4. （选）对比 REALTIME / MONOTONIC 在时间跳变下行为  

---


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


---

## 参考


- Kerrisk · TLPI Ch23  
- `man 2 nanosleep` · `man 2 clock_nanosleep` · `man 2 setitimer` · `man 2 timer_create` · `man 2 clock_gettime`


---

## 代码示例

```c
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

/* Ch23 定时器与睡眠 — alarm/timer_create/nanosleep。
 * 演示 alarm 定时 + nanosleep 精确睡眠。
 * 编译: gcc -o ch23_demo ch23_demo.c */

static volatile sig_atomic_t alarm_fired = 0;

void alarm_handler(int sig) {
    alarm_fired = 1;
}

void sigusr1_handler(int sig) {
    alarm_fired++;
}

int main(void) {
    /* alarm: 秒级定时器 */
    signal(SIGALRM, alarm_handler);
    alarm(2);
    printf("alarm set for 2 seconds, waiting...\n");
    pause();
    printf("alarm fired!\n");

    /* nanosleep: 纳秒级睡眠 */
    struct timespec req = {1, 500 * 1000 * 1000};  /* 1.5 秒 */
    struct timespec rem;
    printf("nanosleep(1.5s)...\n");
    nanosleep(&req, &rem);

    /* timer_create: POSIX 定时器（更精确） */
    timer_t timerid;
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    signal(SIGUSR1, sigusr1_handler);

    timer_create(CLOCK_MONOTONIC, &sev, &timerid);
    struct itimerspec its = {
        .it_interval = {0, 0},         /* 不重复 */
        .it_value = {1, 0}             /* 1秒后触发 */
    };
    timer_settime(timerid, 0, &its, NULL);
    printf("POSIX timer set for 1 second...\n");
    pause();
    printf("POSIX timer fired!\n");
    timer_delete(timerid);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
