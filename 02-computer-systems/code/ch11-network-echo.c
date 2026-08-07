/*
 * CSAPP Ch11 · 网络编程 — Socket echo 服务器 + TCP_NODELAY
 *
 * 对照笔记:
 *   chapter-11/notes/section-11.4-套接字接口.md
 *   chapter-11/notes/section-11.5-Web服务器.md
 *
 * 编译:
 *   gcc -Wall -Wextra -std=c11 -O2 -o ch11_echo ch11-network-echo.c -lpthread
 * 运行:
 *   终端1: ./ch11_echo
 *   终端2: nc 127.0.0.1 8080
 *
 * HFT 关联: TCP_NODELAY 禁用 Nagle → 小包立即发送
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <errno.h>

#define PORT    8080
#define BUFSZ   4096

static volatile int running = 1;

static void sigint_handler(int sig) { (void)sig; running = 0; }

/* ---------- 设置 TCP_NODELAY (禁用 Nagle 算法) ---------- */
static int set_tcp_nodelay(int fd)
{
    int flag = 1;
    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    if (ret == 0)
        printf("  [conn %d] TCP_NODELAY = ON (Nagle 禁用)\n", fd);
    else
        printf("  [conn %d] TCP_NODELAY 失败: %s\n", fd, strerror(errno));
    return ret;
}

/* ---------- 设置 SO_REUSEADDR ---------- */
static int set_reuseaddr(int fd)
{
    int flag = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}

/* ---------- echo 处理函数 ---------- */
static void handle_client(int client_fd)
{
    char buf[BUFSZ];
    ssize_t n;

    /* 设置 TCP_NODELAY — HFT 必须禁用 Nagle */
    set_tcp_nodelay(client_fd);

    printf("  [conn %d] 连接建立\n", client_fd);

    while ((n = recv(client_fd, buf, sizeof(buf), 0)) > 0) {
        /* 回显 */
        ssize_t sent = 0;
        while (sent < n) {
            ssize_t s = send(client_fd, buf + sent, n - sent, 0);
            if (s <= 0) break;
            sent += s;
        }
        buf[n] = '\0';
        printf("  [conn %d] recv %zd bytes: %s", client_fd, n, buf);
    }

    printf("  [conn %d] 连接关闭\n", client_fd);
    close(client_fd);
}

int main(void)
{
    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, SIG_IGN);  /* 忽略 SIGPIPE (对端关闭时 send 返回 -1) */

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return 1; }

    set_reuseaddr(listen_fd);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT),
        .sin_addr   = { .s_addr = htonl(INADDR_LOOPBACK) },
    };

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    if (listen(listen_fd, 5) < 0) {
        perror("listen"); return 1;
    }

    printf("=== CSAPP Ch11 · Echo Server (port %d) ===\n\n", PORT);
    printf("  测试: nc 127.0.0.1 %d\n", PORT);
    printf("  Ctrl+C 退出\n\n");

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(listen_fd,
                               (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) break;
            perror("accept");
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        printf("  新连接: %s:%d (fd=%d)\n", ip, ntohs(client_addr.sin_port), client_fd);

        /* 简单处理: 单连接 echo (CSAPP Tiny 风格) */
        handle_client(client_fd);
    }

    printf("\n  服务器关闭\n");
    close(listen_fd);
    return 0;
}
