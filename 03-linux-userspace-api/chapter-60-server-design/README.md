# TLPI 第 60 章 — Sockets: Server Design

**优先级**：🔴（TCP 服务架构选型）  
**前置**：Ch59 Internet Domains  
**后置**：[Ch61 Socket Advanced](../chapter-61-sockets-advanced/README.md)

---

## 小节目录

- [60.1 迭代服务器](notes/60.1-iterative-and-concurrent-servers.md)
- [60.2 –60.3 fork 并发](notes/60.2-an-iterative-udp-echo-server.md)
- [60.4 多线程](notes/60.4-other-concurrent-server-designs.md)
- [60.5 事件驱动（select/poll 入门）](notes/60.5-the-inetd-internet-superserver-daemon.md)
- [60.6 工程问题](notes/60.6-summary.md)
- [60.7 对比](notes/60.7-exercises.md)

---

## 章节目标


迭代 / fork / pthread / select·poll；僵尸与 fd 纪律；`SO_REUSEADDR`；惊群；四模型对比。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 迭代：一慢堵全站 |
| 2 | fork：子关 listen、父关 conn |
| 3 | SIGCHLD + WNOHANG 防僵尸 |
| 4 | 线程勿传 &connfd |
| 5 | SO_REUSEADDR 利重启 |
| 6 | 高并发 → 事件驱动 / epoll |

---


---

## 参考


- Kerrisk · TLPI Ch60（非「第 53 章」误标）  
- `man 2 accept` · `waitpid` · `setsockopt`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>

/* Ch60 服务器设计 — 迭代 vs 并发 vs prefork。
 * 演示并发服务器 (fork per client) 模型。
 * 编译: gcc -o ch60_demo ch60_demo.c */

#define PORT 9998
#define MAX_CLIENTS 3

/* SIGCHLD 处理器: 回收僵尸子进程 */
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int main(void) {
    signal(SIGCHLD, sigchld_handler);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
        .sin_port = htons(PORT)
    };
    bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sfd, 5);

    printf("Concurrent server on port %d (fork per client)\n", PORT);

    /* 模拟: 创建 MAX_CLIENTS 个客户端子进程 */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        pid_t cpid = fork();
        if (cpid == 0) {
            /* 客户端 */
            int cfd = socket(AF_INET, SOCK_STREAM, 0);
            connect(cfd, (struct sockaddr *)&addr, sizeof(addr));
            char msg[32];
            snprintf(msg, sizeof(msg), "client %d", i);
            write(cfd, msg, strlen(msg) + 1);
            char buf[64];
            int n = read(cfd, buf, sizeof(buf));
            if (n > 0) printf("Client %d: reply='%s'\n", i, buf);
            close(cfd);
            _exit(0);
        }
    }

    /* 服务器: accept 循环, 每个 client fork 一个子进程处理 */
    int handled = 0;
    while (handled < MAX_CLIENTS) {
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);
        int cfd = accept(sfd, (struct sockaddr *)&cli, &len);
        if (cfd < 0) continue;

        pid_t pid = fork();
        if (pid == 0) {
            /* 子进程: 处理单个客户端 */
            close(sfd);  /* 子进程不需要监听套接字 */
            char buf[64];
            int n = read(cfd, buf, sizeof(buf));
            if (n > 0) printf("Server child: handling '%s'\n", buf);
            write(cfd, "served", 7);
            close(cfd);
            _exit(0);
        }
        close(cfd);  /* 父进程不需要连接套接字 */
        handled++;
    }

    /* 等待所有客户端完成 */
    while (wait(NULL) > 0)
        ;
    close(sfd);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
