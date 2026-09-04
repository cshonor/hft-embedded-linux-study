# TLPI 第 20 章 — Signals: Fundamental Concepts

**优先级**：🔴（进程控制、daemon、可靠异步事件的理论地基）  
**前置**：[Ch19 inotify](../chapter-19-monitoring-file-events/README.md)（同属异步事件，机制不同）  
**后置**：[Ch21 Signal Handlers](../chapter-21-signal-handlers/README.md) · [Ch22 高级信号](../chapter-22-signals-advanced/README.md)

---

## 小节目录

- [20.1 概念与概述](notes/20.1-concepts-and-overview.md)
- [20.2 信号类型与默认动作](notes/20.2-signal-types-and-default-actions.md)
- [20.3 改变信号处置：`signal()`](notes/20.3-changing-signal-dispositions-signal.md)
- [20.4 信号处理器简介](notes/20.4-introduction-to-signal-handlers.md)
- [20.5 发送信号：`kill()`](notes/20.5-sending-signals-kill.md)
- [20.6 检查进程是否存在](notes/20.6-checking-for-the-existence-of-a-process.md)
- [20.7 其他发送信号的方式：`raise()` 与 `killpg()`](notes/20.7-other-ways-of-sending-signals-raise-and-.md)
- [20.8 显示信号描述](notes/20.8-displaying-signal-descriptions.md)
- [20.9 信号集（Signal Sets）](notes/20.9-signal-sets.md)
- [20.10 信号掩码（阻塞信号传递）](notes/20.10-the-signal-mask-blocking-signal-delivery.md)
- [20.11 待处理信号（Pending Signals）](notes/20.11-pending-signals.md)
- [20.12 信号不排队](notes/20.12-signals-are-not-queued.md)
- [20.13 改变信号处置：`sigaction()`](notes/20.13-changing-signal-dispositions-sigaction.md)
- [20.14 等待信号：`pause()`](notes/20.14-waiting-for-a-signal-pause.md)
- [20.15 本章小结](notes/20.15-summary.md)
- [20.16 练习题](notes/20.16-exercises.md)

---

## 章节目标


建立信号模型与生命周期；区分标准信号 vs 实时信号；掌握信号集与掩码、未决集；理解递送时机；认清 `signal()` 缺陷与 `kill`/`raise` 语义。

---


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


---

## 练习


1. `sigprocmask` 阻塞 `SIGINT`，Ctrl+C 暂无效  
2. `sigpending` 再解除，观察递送  
3. （选）对比 `signal()` 局限  
4. `kill(pid,0)` 探测  

---


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


---

## 参考


- Kerrisk · TLPI Ch20  
- `man 7 signal` · `man 2 sigprocmask` · `man 2 kill` · `man 2 sigpending`


---

## 代码示例

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

/* Ch20 信号基础 — signal/kill/raise + 信号概念。
 * 演示注册信号处理器 + 自发信号。
 * 编译: gcc -o ch20_demo ch20_demo.c */

static volatile sig_atomic_t got_signal = 0;

void handler(int sig) {
    got_signal = sig;
}

int main(void) {
    /* 注册 SIGUSR1 处理器 */
    signal(SIGUSR1, handler);

    /* 给自己发信号 */
    printf("Sending SIGUSR1 to self...\n");
    raise(SIGUSR1);

    /* 检查是否收到 */
    if (got_signal == SIGUSR1)
        printf("Caught SIGUSR1!\n");
    else
        printf("No signal caught\n");

    /* kill() 也可以给自己发信号 */
    printf("Sending SIGUSR1 via kill()...\n");
    kill(getpid(), SIGUSR1);

    if (got_signal == SIGUSR1)
        printf("Caught again!\n");

    /* SIGKILL (9) 和 SIGSTOP (19) 不能被捕获/忽略/阻塞 */
    printf("\nSIGKILL and SIGSTOP cannot be caught or ignored\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
