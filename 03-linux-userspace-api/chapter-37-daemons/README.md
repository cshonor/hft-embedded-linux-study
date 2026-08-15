# TLPI 第 37 章 — Daemons

**优先级**：🔴（后台服务、嵌入式常驻进程）  
**前置**：[Ch34 会话/`setsid`](../chapter-34-process-groups-sessions/README.md) · [Ch36 rlimit](../chapter-36-process-resources/README.md)  
**后置**：[Ch38 特权程序安全](../chapter-38-secure-privileged/README.md)

---

## 小节目录

- [37.1 特征](notes/37.1-overview.md)
- [37.2 标准 7 步（及原因）](notes/37.2-creating-a-daemon.md)
- [37.3 编写规范](notes/37.3-guidelines-for-writing-daemons.md)
- [37.4 `SIGHUP` 热重载](notes/37.4-using-sighup-to-reinitialize-a-daemon.md)
- [37.5 syslog](notes/37.5-logging-messages-and-errors-using-syslog.md)

---

## 章节目标


守护特征；标准守护化步骤与 `becomeDaemon()`；syslog；`SIGHUP`/`SIGTERM`；PID 文件单实例；对比 `daemon()`。

---


---

## 易错清单


1. 只 fork 一次  
2. 不 `chdir`  
3. 只 close 012 不重定向  
4. SIGHUP handler 里做重 IO  
5. 靠 `daemon()`  
6. 无 PID 锁多实例  
7. 依赖 stdout  

---


---

## 实验清单


1. 双重 fork 后查 SID/无 tty  
2. `becomeDaemon`  
3. syslog  
4. SIGHUP 标志位重载  
5. （选）PID 文件锁  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 无终端；双重 fork + setsid |
| 2 | 二次 fork：非会话首，防再抢 tty |
| 3 | chdir + 关 fd + 012→null |
| 4 | SIGHUP=重载；SIGTERM=退出 |
| 5 | 日志用 syslog |
| 6 | 自写 becomeDaemon，慎用 daemon() |

---


---

## 参考


- Kerrisk · TLPI Ch37  
- `man 3 daemon` · `man 3 syslog` · `man 2 setsid`


---

## 代码示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>

/* Ch37 守护进程 — 标准守护进程化步骤。
 * 1. fork + setsid 脱离终端
 * 2. 再次 fork 防止重新获取终端
 * 3. chdir("/") 不占用挂载点
 * 4. umask(0) 清除文件权限掩码
 * 5. 关闭/重定向标准 fd
 * 编译: gcc -o ch37_demo ch37_demo.c */

void daemonize(void) {
    /* Step 1: fork, 父进程退出 */
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);

    /* Step 2: setsid — 成为新会话首领 */
    if (setsid() < 0) exit(1);

    /* Step 3: 再次 fork, 不再是会话首领 */
    pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);

    /* Step 4: chdir + umask */
    chdir("/");
    umask(0);

    /* Step 5: 关闭所有继承的 fd, 重定向 0/1/2 */
    for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--)
        close(fd);

    int nullfd = open("/dev/null", O_RDWR);
    if (nullfd >= 0) {
        dup2(nullfd, STDIN_FILENO);
        dup2(nullfd, STDOUT_FILENO);
        dup2(nullfd, STDERR_FILENO);
        if (nullfd > 2) close(nullfd);
    }
}

int main(void) {
    printf("Becoming a daemon...\n");
    fflush(stdout);

    daemonize();

    /* 现在是守护进程，用 syslog 输出日志 */
    openlog("ch37_demo", LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_NOTICE, "Daemon started, pid=%d", (int)getpid());

    /* 模拟工作: 每 5 秒写一次日志 */
    for (int i = 0; i < 3; i++) {
        sleep(5);
        syslog(LOG_INFO, "Daemon working: iteration %d", i);
    }

    syslog(LOG_NOTICE, "Daemon exiting");
    closelog();
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
