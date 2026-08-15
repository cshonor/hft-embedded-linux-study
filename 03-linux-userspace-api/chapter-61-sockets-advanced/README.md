# TLPI 第 61 章 — Sockets: Advanced Topics

**优先级**：🔴（选项、msghdr、UDP connect、短读写）  
**前置**：[Ch60 Server Design](../chapter-60-server-design/README.md)  
**后置**：[Ch62 Terminals](../chapter-62-terminals/README.md)

---

## 小节目录

- [61.1 API](notes/61.1-partial-reads-and-writes-on-stream-socke.md)
- [61.2 SOL_SOCKET（必考）](notes/61.2-the-shutdown-system-call.md)
- [61.3 TCP 选项](notes/61.12-tcp-versus-udp.md)
- [61.4 OOB](notes/61.4-the-sendfile-system-call.md)
- [61.5 `sendmsg` / `recvmsg`](notes/61.5-retrieving-socket-addresses.md)
- [61.6 短读写](notes/61.6-a-closer-look-at-tcp.md)
- 61.7 –61.9 地址 · UDP connect · IPv6
- [61.10 `ioctl`](notes/61.10-the-so-reuseaddr-socket-option.md)

---

## 章节目标


`setsockopt`/`getsockopt`；SOL_SOCKET / TCP 选项；OOB；`sendmsg`/`recvmsg`；短读写；`getsockname`/`getpeername`；UDP connect；`IPV6_V6ONLY`。

---


---

## 陷阱


1. REUSEADDR 设晚于 bind  
2. REUSEADDR vs REUSEPORT  
3. TCP 不循环写满  
4. TCP 当包边界  
5. send 传 fd  
6. linger=0 粗暴 RST  
7. UDP connect≠可靠  
8. OOB 当大数据通道  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | REUSEADDR 在 bind 前；≠ REUSEPORT |
| 2 | TCP_NODELAY 关 Nagle |
| 3 | 短读写循环；UDP 整报 |
| 4 | fd 传递 → sendmsg 控制信息 |
| 5 | UDP connect 无握手仍可丢 |
| 6 | linger=0 → RST |

---


---

## 参考


- Kerrisk · TLPI Ch61  
- `man 7 socket` · `tcp` · `man 2 sendmsg` · `getsockname`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>

/* Ch61 套接字高级 — select/poll/epoll 多路复用。
 * 演示 poll 同时监控多个套接字。
 * 编译: gcc -o ch61_demo ch61_demo.c */

#define PORT 9997

int main(void) {
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

    /* fork 2 个客户端 */
    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            int cfd = socket(AF_INET, SOCK_STREAM, 0);
            connect(cfd, (struct sockaddr *)&addr, sizeof(addr));
            char msg[32];
            snprintf(msg, sizeof(msg), "hello %d", i);
            sleep(i + 1);  /* 错开发送时间 */
            write(cfd, msg, strlen(msg) + 1);
            char buf[64];
            read(cfd, buf, sizeof(buf));
            printf("Client %d: got '%s'\n", i, buf);
            close(cfd);
            _exit(0);
        }
    }

    /* 服务器: 用 poll 监控监听套接字 + 已连接套接字 */
    struct pollfd fds[16];
    int nfds = 1;
    fds[0].fd = sfd;
    fds[0].events = POLLIN;

    printf("Server: polling for connections...\n");

    for (int served = 0; served < 2; ) {
        poll(fds, nfds, 10000);  /* 10秒超时 */

        for (int i = 0; i < nfds; i++) {
            if (!(fds[i].revents & POLLIN))
                continue;

            if (i == 0) {
                /* 监听套接字可读 -> 新连接 */
                int cfd = accept(sfd, NULL, NULL);
                fds[nfds].fd = cfd;
                fds[nfds].events = POLLIN;
                nfds++;
                printf("Server: accepted new client (fd=%d)\n", cfd);
            } else {
                /* 已连接套接字可读 -> 数据到达 */
                char buf[64];
                int n = read(fds[i].fd, buf, sizeof(buf));
                if (n > 0) {
                    printf("Server: received '%s' from fd=%d\n",
                           buf, fds[i].fd);
                    write(fds[i].fd, "ok", 3);
                    served++;
                }
                close(fds[i].fd);
                fds[i].fd = -1;  /* poll 忽略 fd=-1 */
            }
        }
    }

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
