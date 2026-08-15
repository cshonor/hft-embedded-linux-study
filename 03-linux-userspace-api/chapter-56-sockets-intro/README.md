# TLPI 第 56 章 — Sockets: Introduction

**优先级**：🔴（Socket API 总入口）  
**前置**：[Ch55 文件锁](../chapter-55-file-locking/README.md) · 本地 IPC  
**后置**：[Ch57 UNIX 域](../chapter-57-sockets-unix-domain/README.md) → [Ch58 TCP/IP](../chapter-58-tcpip-fundamentals/README.md) → Ch59 Internet

---

## 小节目录

- [56.1 概念](notes/56.1-overview.md)
- [56.2 –56.4 创建与地址](notes/56.2-creating-a-socket-socket.md)
- [56.5 面向连接 API](notes/56.5-stream-sockets.md)
- [56.6 –56.8 I/O 与关闭](notes/56.6-datagram-sockets.md)

---

## 章节目标


`socket`/`bind`/`listen`/`accept`/`connect`；地址结构；读写与关闭；STREAM vs DGRAM；UNIX vs INET。

---


---

## UNIX vs Internet（导论）


| | AF_UNIX | AF_INET(6) |
|--|---------|------------|
| 范围 | 本机 | 跨机 |
| 地址 | 路径 / 抽象名 | IP:port |
| DGRAM | 可靠 | UDP 可丢 |
| 权限 | 文件权限 | 应用层 |
| 特色 | 可传 fd/凭证 | 协议栈 |

本机优先 UNIX；要远程用 Internet。

---


---

## 陷阱


1. STREAM 无消息边界  
2. 勿关 listener；关的是 conn fd  
3. UNIX DGRAM ≠ UDP 可靠性  
4. fork 复制 fd → close≠断连  
5. backlog≠最大客户端数  
6. addrlen 类型用 socklen_t  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | socket→bind→listen→accept / connect |
| 2 | STREAM 字节流；DGRAM 有边界 |
| 3 | accept 新 fd；listener 常留 |
| 4 | shutdown 半关闭；close 看引用 |
| 5 | UNIX DGRAM 可靠；UDP 不 |
| 6 | 本机 UNIX；跨机 INET |

---


---

## 参考


- Kerrisk · TLPI Ch56  
- `man 2 socket` · `bind` · `listen` · `accept` · `connect` · `shutdown`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

/* Ch56 套接字入门 — socket/bind/listen/accept/connect。
 * 演示 AF_UNIX 面向连接的服务器端基本流程。
 * 编译: gcc -o ch56_demo ch56_demo.c */

#define SOCK_PATH "/tmp/ch56_sock"

int main(void) {
    /* 创建套接字: AF_UNIX + SOCK_STREAM */
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); return 1; }

    /* 绑定地址 */
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCK_PATH);  /* 确保路径不存在 */
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    /* 监听, backlog=5 */
    if (listen(sfd, 5) < 0) { perror("listen"); return 1; }
    printf("Server listening on %s\n", SOCK_PATH);

    /* fork 子进程作为客户端 */
    pid_t pid = fork();
    if (pid == 0) {
        int cfd = socket(AF_UNIX, SOCK_STREAM, 0);
        connect(cfd, (struct sockaddr *)&addr, sizeof(addr));
        write(cfd, "hello socket", 12);
        char buf[64];
        int n = read(cfd, buf, sizeof(buf));
        if (n > 0) { buf[n] = '\0'; printf("Client: got '%s'\n", buf); }
        close(cfd);
        _exit(0);
    }

    /* 服务器: accept + 读写 */
    int cfd = accept(sfd, NULL, NULL);
    if (cfd >= 0) {
        char buf[64];
        int n = read(cfd, buf, sizeof(buf));
        if (n > 0) { buf[n] = '\0'; printf("Server: got '%s'\n", buf); }
        write(cfd, "reply from server", 17);
        close(cfd);
    }

    waitpid(pid, NULL, 0);
    close(sfd);
    unlink(SOCK_PATH);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
