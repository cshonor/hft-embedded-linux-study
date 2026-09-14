/* TLPI 第 2 章 §2.17 —— 客户端/服务器架构：最小的请求-响应回路
 *
 * 编译：gcc -O0 -Wall -Wextra c2_17_client_server.c -o c2_17
 * 运行：./c2_17
 *
 * 本节要钉死的事实：
 *   ① C/S 的本体是四步循环：socket → bind+listen → accept → read/write。
 *      「服务器」不是特殊的进程，只是先 bind 到一个已知地址、再循环 accept。
 *   ② 这里的「迭代服务器」一次只服务一个连接；生产要么多进程/多线程，
 *      要么用 epoll 做事件驱动。
 *   ③ AF_UNIX（同机，走文件系统路径）与 AF_INET（跨机，走 IP:port）是两套地址，
 *      但 accept/read/write 的代码形状完全一样。
 *   ④ 客户端只要 connect 成功就能收发 —— 不需要 bind。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define UNIX_PATH "/tmp/c2_17.sock"
#define TCP_PORT  47123

/* ---------- 通用：把一条请求处理掉（server 侧） ---------- */
static void serve_one(int cfd, const char *tag)
{
    char buf[256];
    ssize_t n = read(cfd, buf, sizeof(buf) - 1);
    if (n <= 0) { printf("  [%s] 读请求失败/为空 (n=%zd)\n", tag, n); return; }
    buf[n] = '\0';
    printf("  [%s] 收到请求: \"%s\"\n", tag, buf);

    char resp[300];
    snprintf(resp, sizeof(resp), "ACK:%.*s", (int)n, buf);
    if (write(cfd, resp, strlen(resp)) < 0) perror("write response");
    printf("  [%s] 已回响应: \"%s\"\n", tag, resp);
}

/* ================= AF_UNIX：同机 ================= */
static void demo_unix(void)
{
    printf("=== ① AF_UNIX：同一台机器上的 C/S ===\n");
    unlink(UNIX_PATH);

    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); return; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UNIX_PATH, sizeof(addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); close(sfd); return; }
    printf("  server: bind(\"%s\") 成功 —— 这个路径现在是个 socket 文件\n", UNIX_PATH);

    /* 看一眼它真的变成了文件树上的 socket 类型 */
    struct stat st_buf;
    if (stat(UNIX_PATH, &st_buf) == 0)
        printf("  server: stat() 看到 st_mode=%04o  S_ISSOCK=%d\n",
               st_buf.st_mode & 07777, S_ISSOCK(st_buf.st_mode));

    if (listen(sfd, 8) < 0) { perror("listen"); close(sfd); return; }
    printf("  server: listen(backlog=8) —— 这一步之后内核才开始排队连接\n");

    /* 客户端另起一个进程，模拟"另一个程序" */
    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) {
        close(sfd);
        int c = socket(AF_UNIX, SOCK_STREAM, 0);
        if (c < 0) _exit(1);
        if (connect(c, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            printf("  [client] connect 失败: %s\n", strerror(errno));
            fflush(NULL);
            _exit(1);
        }
        printf("  [client] connect 成功 —— 我没 bind，直接连就行\n");
        const char *req = "GET /price/AAPL";
        if (write(c, req, strlen(req)) < 0) perror("write");
        char rb[256];
        ssize_t k = read(c, rb, sizeof(rb) - 1);
        if (k > 0) { rb[k] = '\0'; printf("  [client] 收到响应: \"%s\"\n", rb); }
        close(c);
        fflush(NULL);
        _exit(0);
    }

    int cfd = accept(sfd, NULL, NULL);
    if (cfd < 0) { perror("accept"); }
    else { serve_one(cfd, "server"); close(cfd); }

    int st; waitpid(pid, &st, 0);
    close(sfd);
    unlink(UNIX_PATH);
    printf("  -> 四步走完：socket -> bind -> listen -> accept -> read/write\n");
}

/* ================= AF_INET：跨机（这里用 loopback） ================= */
static void demo_inet(void)
{
    printf("\n=== ② AF_INET：换一套地址，代码形状不变 ===\n");
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { printf("  socket(AF_INET) 失败: %s\n", strerror(errno)); return; }

    int on = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(TCP_PORT);
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);        /* 127.0.0.1 */

    if (bind(sfd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        printf("  bind(127.0.0.1:%d) 失败: %d (%s)\n", TCP_PORT, errno, strerror(errno));
        printf("  -> 本环境不允许建 INET socket（无网络命名空间权限），如实记录\n");
        close(sfd);
        return;
    }
    if (listen(sfd, 8) < 0) { perror("listen"); close(sfd); return; }
    printf("  server: bind(127.0.0.1:%d) + listen 成功\n", TCP_PORT);

    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) {
        close(sfd);
        int c = socket(AF_INET, SOCK_STREAM, 0);
        if (c < 0) _exit(1);
        if (connect(c, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
            printf("  [client] connect 127.0.0.1:%d 失败: %s\n", TCP_PORT, strerror(errno));
            fflush(NULL);
            _exit(1);
        }
        /* 看看连上之后本地地址是什么：内核自动分配了源端口 */
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        if (getsockname(c, (struct sockaddr *)&peer, &plen) == 0)
            printf("  [client] 连接成功，内核给我分配的源端口 = %u\n",
                   ntohs(peer.sin_port));
        const char *req = "PING";
        if (write(c, req, strlen(req)) < 0) perror("write");
        char rb[128];
        ssize_t k = read(c, rb, sizeof(rb) - 1);
        if (k > 0) { rb[k] = '\0'; printf("  [client] 收到响应: \"%s\"\n", rb); }
        close(c);
        fflush(NULL);
        _exit(0);
    }

    int cfd = accept(sfd, NULL, NULL);
    if (cfd < 0) perror("accept");
    else { serve_one(cfd, "server"); close(cfd); }
    int st; waitpid(pid, &st, 0);
    close(sfd);
}

/* ================= 服务器形态对照 ================= */
static void compare(void)
{
    printf("\n=== ③ 服务器形态：从「够用」到「能扛」 ===\n");
    printf("  %-18s %-30s %s\n", "model", "one line of idea", "cost / limit");
    printf("  %-18s %-30s %s\n", "------------------", "------------------------------",
           "--------------------------------");
    printf("  %-18s %-30s %s\n", "iterative", "accept, handle, accept...",
           "one client at a time; a slow client stalls all");
    printf("  %-18s %-30s %s\n", "fork-per-conn", "accept then fork",
           "simple; process creation + N x memory");
    printf("  %-18s %-30s %s\n", "thread-per-conn", "accept then pthread_create",
           "lighter; shared state needs locking");
    printf("  %-18s %-30s %s\n", "process/thread pool", "pre-create workers, accept in them",
           "bounded resources; still per-conn cost");
    printf("  %-18s %-30s %s\n", "event loop (epoll)", "one thread, many fds, non-blocking",
           "best throughput/latency; callbacks get hairy");
    printf("  %-18s %-30s %s\n", "io_uring", "submit batches, reap completions",
           "fewest syscalls; newest, least portable");
    printf("\n  HFT 里行情/下单服务器几乎都在最后两行：单线程 + epoll / io_uring，\n");
    printf("  把「每次请求的上下文切换」压到零。\n");

    printf("\n=== ④ 为什么服务端要处理 SIGPIPE ===\n");
    printf("  如果客户端提前 close，服务器再 write() 会收到 SIGPIPE，默认动作是\n");
    printf("  **终止整个进程** —— 一个异常客户端就能打挂服务器。\n");
    printf("  标准做法有两条（一般两条都上）：\n");
    printf("    · signal(SIGPIPE, SIG_IGN) / sigaction 忽略它，改看 write 的返回值\n");
    printf("    · 每个连接 socket 用 MSG_NOSIGNAL：send(fd, buf, n, MSG_NOSIGNAL)\n");
    printf("  本 demo 只在 ① 里跑了正常路径，故意不制造断连（会终止进程）。\n");
}

int main(void)
{
    demo_unix();
    demo_inet();
    compare();
    return 0;
}
