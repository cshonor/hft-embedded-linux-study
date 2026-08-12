# TLPI 第 59 章 — Sockets: Internet Domains

**优先级**：🔴（INET TCP/UDP 实战）  
**前置**：[Ch58 TCP/IP 基础](../chapter-58-tcpip-fundamentals/notes.md)  
**后置**：[Ch60 Server Design](../chapter-60-server-design/notes.md)

---

## 小节目录

- [59.1 –59.3 回顾 · 序列化](./notes/59.1-serialization.md)
- [59.4 –59.6 地址与转换](./notes/59.4-conversion.md)
- [59.7 `getaddrinfo` / `getnameinfo`（核心）](./notes/59.7-getaddrinfo-getnameinfo.md)
- [59.8 –59.9 UDP / TCP 模型](./notes/59.8-udp-tcp.md)
- [59.13 SIGPIPE](./notes/59.13-sigpipe.md)

---

## 章节目标


`sockaddr_in(6)`；pton/ntop；`getaddrinfo`；UDP/TCP 迭代 C/S；SIGPIPE；序列化；vs UNIX。

---


---

## vs UNIX（汇总）


| | AF_UNIX | AF_INET(6) |
|--|---------|------------|
| 范围 | 本机 | 跨机 |
| DGRAM | 可靠 | UDP 可丢 |
| fd/凭证传递 | 可 | 否 |
| 字节序 | 不必 | 必须 |

---


---

## 陷阱


1. 忘 htons / memset  
2. 裸发结构体  
3. TCP 当消息边界  
4. SIGPIPE  
5. 旧解析 API  
6. 忘 `freeaddrinfo`  
7. UDP recv 缓冲短 → 余部丢弃  
8. connect 用 INADDR_ANY  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | getaddrinfo + freeaddrinfo |
| 2 | AI_PASSIVE 服务端 |
| 3 | TCP 迭代：accept 新 fd |
| 4 | SIGPIPE：IGN 或 MSG_NOSIGNAL |
| 5 | UDP 边界有、可靠无 |
| 6 | 跨机须序列化 |

---


---

## 参考


- Kerrisk · TLPI Ch59  
- `man 3 getaddrinfo` · `getnameinfo` · `man 7 tcp` · `udp`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/* Ch59 Internet 域套接字 — TCP 服务器 + 客户端。
 * 演示 AF_INET + SOCK_STREAM 完整通信流程。
 * 编译: gcc -o ch59_demo ch59_demo.c */

#define PORT 9999

int main(void) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); return 1; }

    /* 设置 SO_REUSEADDR, 避免重启时 "Address already in use" */
    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  /* 127.0.0.1 */
    addr.sin_port = htons(PORT);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    listen(sfd, 5);
    printf("TCP server listening on 127.0.0.1:%d\n", PORT);

    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程: 客户端 */
        int cfd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in srv = addr;
        if (connect(cfd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
            perror("connect"); _exit(1);
        }
        write(cfd, "TCP hello", 9);
        char buf[64];
        int n = read(cfd, buf, sizeof(buf));
        if (n > 0) { buf[n] = '\0'; printf("Client: reply='%s'\n", buf); }
        close(cfd);
        _exit(0);
    }

    /* 父进程: 服务器 */
    struct sockaddr_in cli;
    socklen_t cli_len = sizeof(cli);
    int cfd = accept(sfd, (struct sockaddr *)&cli, &cli_len);
    if (cfd >= 0) {
        char buf[64];
        int n = read(cfd, buf, sizeof(buf));
        if (n > 0) { buf[n] = '\0'; printf("Server: received='%s'\n", buf); }
        write(cfd, "TCP reply", 9);
        close(cfd);
    }

    waitpid(pid, NULL, 0);
    close(sfd);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
