# TLPI 第 57 章 — Sockets: UNIX Domain

**优先级**：🔴（本机低延迟 IPC）  
**前置**：[Ch56 Socket 导论](../chapter-56-sockets-intro/notes.md)  
**后置**：[Ch58 TCP/IP 基础](../chapter-58-tcpip-fundamentals/notes.md)

---

## 小节目录

- [57.1 `struct sockaddr_un`](./notes/57.1-struct-sockaddrun.md)
- [57.2 STREAM](./notes/57.2-stream.md)
- [57.3 DGRAM（高频）](./notes/57.3-dgram.md)
- [57.4 权限](./notes/57.4-permission.md)
- [57.5 `socketpair`](./notes/57.5-socketpair.md)
- [57.6 抽象命名空间（Linux）](./notes/57.6-namespace-abstraction.md)

---

## 章节目标


`sockaddr_un`；STREAM/DGRAM；权限；`socketpair`；抽象命名空间；工程选型。

---


---

## 选型（嵌入式 + HFT）


1. 本机组件通信 → UDS（优先于 127.0.0.1 TCP）  
2. 守护进程 → 抽象名（Linux）  
3. 父子临时双向 → `socketpair`  
4. 要消息边界且本机 → UNIX DGRAM  

---


---

## 思考题要点


1. 崩溃残留：启动 `unlink` 或改用抽象名。  
2. 抽象不受 umask/文件权限。  
3. fork 后关无用 fd；需半关闭用 `shutdown`。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 路径型 bind 前 unlink |
| 2 | UNIX DGRAM ≠ UDP：本地可靠 |
| 3 | connect 需写权限 + 目录 x |
| 4 | socketpair：无名互连对 |
| 5 | 抽象：`path[0]=0`，无文件 |
| 6 | 本机延迟：UDS > loopback TCP |

---


---

## 参考


- Kerrisk · TLPI Ch57  
- `man 7 unix` · `man 2 socketpair`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/* Ch57 UNIX 域套接字 — socketpair + fd 传递 (SCM_RIGHTS)。
 * UNIX 域套接字仅限本机, 但支持传递文件描述符。
 * 编译: gcc -o ch57_demo ch57_demo.c */

int main(void) {
    /* socketpair: 创建一对已连接的套接字 */
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        perror("socketpair"); return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程: 通过 sv[1] 接收 fd */
        close(sv[0]);

        /* 接收普通消息 */
        char buf[64];
        read(sv[1], buf, 12);
        printf("Child: received message: %s\n", buf);

        /* 接收文件描述符 (SCM_RIGHTS) */
        struct msghdr msg = {0};
        char mbuf[256];
        struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = mbuf;
        msg.msg_controllen = sizeof(mbuf);

        recvmsg(sv[1], &msg, 0);

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_level == SOL_SOCKET &&
            cmsg->cmsg_type == SCM_RIGHTS) {
            int recv_fd;
            memcpy(&recv_fd, CMSG_DATA(cmsg), sizeof(int));
            printf("Child: received fd=%d\n", recv_fd);

            /* 使用接收到的 fd 读文件 */
            char fbuf[64];
            int n = read(recv_fd, fbuf, sizeof(fbuf) - 1);
            if (n > 0) { fbuf[n] = '\0'; printf("Child: fd content: %s\n", fbuf); }
            close(recv_fd);
        }
        close(sv[1]);
        _exit(0);
    }

    /* 父进程: 通过 sv[0] 发送消息 + fd */
    close(sv[1]);

    /* 发送普通消息 */
    write(sv[0], "hello fd!", 12);

    /* 打开一个文件, 通过套接字传递 fd */
    int file_fd = open("/etc/hostname", O_RDONLY);

    /* 构造带 ancillary data 的消息 */
    struct msghdr msg = {0};
    char buf[] = "fd coming";
    struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    char cbuf[CMSG_SPACE(sizeof(int))];
    msg.msg_control = cbuf;
    msg.msg_controllen = sizeof(cbuf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &file_fd, sizeof(int));

    sendmsg(sv[0], &msg, 0);
    printf("Parent: sent fd=%d\n", file_fd);

    close(file_fd);
    close(sv[0]);
    waitpid(pid, NULL, 0);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
