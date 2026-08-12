# TLPI 第 64 章 — Pseudoterminals

**优先级**：🔴（ssh / 终端模拟器 / expect）  
**前置**：[Ch62 Terminals](../chapter-62-terminals/notes.md) · [Ch63 Alternative I/O](../chapter-63-alternative-io/notes.md)  
**后置**：地图内 TLPI 主线结束；附录/其他模块另见仓库路线

---

## 小节目录

- [64.1 –64.2 概念](./notes/64.1-concepts.md)
- [64.3 vs 管道](./notes/64.3-pipe.md)
- [64.4 –64.5 POSIX 打开 · 典型架构](./notes/64.4-architecture.md)
- [64.6 –64.7 特性](./notes/64.6-section-64-6.md)
- [64.9 BSD PTY](./notes/64.9-bsd-pty.md)

---

## 章节目标


主从模型；vs 管道；POSIX 打开流程；fork/setsid 架构；包模式与 winsize；BSD 旧式了解。

---


---

## 陷阱


1. 忘 `unlockpt`  
2. 无 `setsid` → 无控制终端 / 作业控制失效  
3. 在 master 调 tcgetattr  
4. 子未关 master → PTY 不销毁  
5. 用 pipe 冒充交互终端  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | master↔slave；slave=终端 |
| 2 | openpt→grant→unlock→open |
| 3 | 子 setsid+dup2(slave)+exec |
| 4 | 交互 shell 必须 PTY |
| 5 | winsize → SIGWINCH |
| 6 | 禁用 BSD 固定 pty 对 |

---


---

## 参考


- Kerrisk · TLPI Ch64  
- `man 3 posix_openpt` · `ptsname` · `man 4 pts`


---

## 代码示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <termios.h>

/* Ch64 伪终端 — openpty/forkpty/posix_openpt。
 * 伪终端 (PTY) 让子进程以为自己在和真实终端交互。
 * 常用于 ssh, xterm, script 等程序。
 * 编译: gcc -o ch64_demo ch64_demo.c -lutil */

int main(void) {
    int master_fd, slave_fd;
    char slave_name[256];

    /* === openpty: 打开一对伪终端 === */
    if (openpty(&master_fd, &slave_fd, slave_name, NULL, NULL) < 0) {
        perror("openpty (need -lutil)");
        return 1;
    }

    printf("PTY pair created:\n");
    printf("  master fd: %d\n", master_fd);
    printf("  slave fd:  %d\n", slave_fd);
    printf("  slave name: %s\n", slave_name);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* 子进程: 打开 slave 端, 作为自己的终端 */
        close(master_fd);

        /* 设置 slave 为控制终端 */
        setsid();
        ioctl(slave_fd, TIOCSCTTY, 0);

        /* 重定向 stdin/stdout/stderr 到 slave */
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        if (slave_fd > 2) close(slave_fd);

        /* 子进程以为自己在和真实终端交互 */
        /* 运行 tty 命令查看终端名 */
        execlp("tty", "tty", NULL);
        _exit(1);
    }

    /* 父进程: 从 master 端读取子进程输出 */
    close(slave_fd);

    char buf[256];
    ssize_t n = read(master_fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("Master received from PTY: %s", buf);
    }

    waitpid(pid, NULL, 0);

    /* === posix_openpt: POSIX 标准方式 === */
    printf("\n=== posix_openpt (POSIX standard) ===\n");
    int pty_master = posix_openpt(O_RDWR | O_NOCTTY);
    if (pty_master >= 0) {
        if (grantpt(pty_master) == 0 && unlockpt(pty_master) == 0) {
            char *name = ptsname(pty_master);
            printf("POSIX PTY slave: %s\n", name ? name : "?");
        }
        close(pty_master);
    }

    close(master_fd);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
