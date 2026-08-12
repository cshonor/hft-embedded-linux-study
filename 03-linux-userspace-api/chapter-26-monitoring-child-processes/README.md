# TLPI 第 26 章 — Monitoring Child Processes

**优先级**：🔴（多进程服务、防僵尸耗尽 PID）  
**前置**：[Ch25 进程终止](../chapter-25-process-termination/notes.md) · [Ch24 fork](../chapter-24-process-creation/notes.md) · [Ch21 SIGCHLD](../chapter-21-signal-handlers/notes.md)  
**后置**：[Ch27 exec](../chapter-27-program-execution/notes.md) · [Ch37 守护进程](../chapter-37-daemons/notes.md)

---

## 小节目录

- [26.1 僵尸](./notes/26.1-section-26-1.md)
- [26.2 `wait`](./notes/26.2-wait.md)
- [26.3 `waitpid`（核心）](./notes/26.3-waitpid.md)
- [26.4 `wstatus` 宏（必用宏，勿裸打数值）](./notes/26.4-wstatus.md)
- [26.5 `SIGCHLD` 循环收割（经典）](./notes/26.5-sigchld.md)
- [26.6 `waitid`（了解）](./notes/26.6-waitid.md)
- [26.7 规则要点](./notes/26.7-section-26-7.md)

---

## 章节目标


掌握 `wait`/`waitpid`/`waitid`；解析 `wstatus`；用 `SIGCHLD`+循环 `WNOHANG` 防漏收；分清阻塞/非阻塞与停止/继续选项。

---


---

## 26.8 易错清单


1. SIGCHLD 只 wait 一次 → 漏僵尸  
2. 裸打 `wstatus`  
3. 默认 waitpid **只**看终止（除非 WUNTRACED/CONTINUED）  
4. `SA_NOCLDWAIT` 后别再指望拿退出状态/SIGCHLD  
5. 信号杀时勿用 `WEXITSTATUS`  

---


---

## 速查：`pid` / 状态


| waitpid pid | 对象 |
|-------------|------|
| `>0` / `0` / `<-1` / `-1` | 指定 / 同组 / 指定组 / 全部 |

| 想知道 | 先查 |
|--------|------|
| 退出码 | `WIFEXITED` + `WEXITSTATUS` |
| 哪个信号杀的 | `WIFSIGNALED` + `WTERMSIG` |

---


---

## 练习


1. 阻塞 waitpid + 解析退出码  
2. 子自杀 `SIGTERM`/`abort`，看 `WIFSIGNALED`  
3. SIGCHLD 循环 `WNOHANG`  
4. （选）`SA_NOCLDWAIT` 对比  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 僵尸 = 等父 wait 的表项 |
| 2 | 用 `waitpid`，别只用 `wait` |
| 3 | 状态必须用 `WIF*` 宏 |
| 4 | SIGCHLD：`while` + `WNOHANG` |
| 5 | `WNOHANG` 返回 0 = 暂无就绪 |
| 6 | 只能收直系子 |

---


---

## 参考


- Kerrisk · TLPI Ch26  
- `man 2 wait` · `man 2 waitpid` · `man 2 waitid` · `man 7 signal`（SIGCHLD）


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Ch26 监控子进程 — wait/waitpid/WIFEXITED/WIFSIGNALED。
 * 演示正常退出/信号终止/暂停状态获取。
 * 编译: gcc -o ch26_demo ch26_demo.c */

int main(void) {
    /* 子进程1: 正常退出 */
    pid_t p1 = fork();
    if (p1 == 0) _exit(7);

    /* 子进程2: 被信号杀死 */
    pid_t p2 = fork();
    if (p2 == 0) {
        /* 给自己发 SIGKILL */
        kill(getpid(), SIGKILL);
        _exit(0);
    }

    /* 子进程3: 被信号暂停 */
    pid_t p3 = fork();
    if (p3 == 0) {
        raise(SIGSTOP);  /* 暂停自己 */
        _exit(0);
    }

    sleep(1);  /* 等子进程完成动作 */

    /* 收割子进程1: 正常退出 */
    int status;
    waitpid(p1, &status, 0);
    if (WIFEXITED(status))
        printf("Child %d: exited normally, code=%d\n",
               (int)p1, WEXITSTATUS(status));

    /* 收割子进程2: 被信号杀死 */
    waitpid(p2, &status, 0);
    if (WIFSIGNALED(status))
        printf("Child %d: killed by signal %d\n",
               (int)p2, WTERMSIG(status));

    /* 收割子进程3: 先暂停后继续 */
    waitpid(p3, &status, WUNTRACED);
    if (WIFSTOPPED(status))
        printf("Child %d: stopped by signal %d, resuming...\n",
               (int)p3, WSTOPSIG(status));

    kill(p3, SIGCONT);  /* 让子进程3继续 */
    waitpid(p3, &status, 0);
    printf("Child %d: final status\n", (int)p3);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
