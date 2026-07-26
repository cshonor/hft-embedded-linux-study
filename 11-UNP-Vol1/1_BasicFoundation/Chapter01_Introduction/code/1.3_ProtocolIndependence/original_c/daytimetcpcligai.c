/*
 * 【程序是干什么的】
 *   同样是 TCP Daytime 客户端，但命令行参数传的是“主机名”（可以是域名或 IP 文本），
 *   由 getaddrinfo 查 DNS/本机配置，得到 IPv4 或 IPv6 地址后再连接。
 *   一份代码可以同时适应双栈网络（协议无关编程的入门写法）。
 *
 * 【你怎么运行】
 *   daytimetcpcligai localhost
 *   daytimetcpcligai time.example.com
 *
 * 【和硬编码版的对比】
 *   不再出现 sockaddr_in / sockaddr_in6 变量；
 *   不再写死 AF_INET 或 AF_INET6；
 *   用 addrinfo 链表里的 ai_family、ai_addr 等字段，循环尝试 connect。
 */

#include "unp.h"

int
main(int argc, char **argv)
{
    int                 err, sockfd, n;
    char                recvline[MAXLINE + 1];

    /*
     * hints — 你告诉 getaddrinfo“我想要什么样的结果”的筛选条件（输入）。
     * res   — 函数填好的结果链表的头指针（输出）。
     * rp    — 用来从 res 开始一条一条遍历链表的游标指针。
     *
     * 指针复习（新手）：
     *   int *p   — p 里存的是“某个 int 的地址”；
     *   struct addrinfo *res — res 指向一个 addrinfo 节点，节点里有 ai_next 指向下一个。
     */
    struct addrinfo     hints, *res, *rp;

    if (argc != 2)
        err_quit("usage: daytimetcpcligai <hostname>");

    /* 把整个 hints 结构清零，未赋值的字段就是“不限制/默认” */
    bzero(&hints, sizeof(struct addrinfo));

    hints.ai_family   = AF_UNSPEC;      /* 不指定 v4 或 v6，解析结果里两种都可能出现 */
    hints.ai_socktype = SOCK_STREAM;    /* 只要 TCP 流套接字对应的地址 */

    /*
     * getaddrinfo(主机名, 服务名, hints, &res)：
     *   "daytime" 会在 /etc/services（Windows 上类似文件）里查到端口 13；
     *   成功时 res 指向链表；失败时返回非 0 的错误码 err（不是 errno！）。
     * gai_strerror(err) — 把错误码转成可读英文字符串。
     */
    if ((err = getaddrinfo(argv[1], "daytime", &hints, &res)) != 0)
        err_quit("getaddrinfo error for %s: %s", argv[1], gai_strerror(err));

    /*
     * for 循环遍历链表：rp 从 res 开始，每次 rp = rp->ai_next 走到下一节点。
     * rp == NULL 表示链表走完了。
     */
    for (rp = res; rp != NULL; rp = rp->ai_next) {

        /*
         * 用这一条解析结果里的“族、类型、协议”创建套接字，
         * 可能是 IPv4 也可能是 IPv6，代码里不用写 if (v4) ... else (v6) ...
         */
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd < 0)
            continue;   /* 例如系统未启用 IPv6，创建失败就试链表下一条 */

        /*
         * connect 直接用 rp->ai_addr（已经是通用的 struct sockaddr*）
         * 和 rp->ai_addrlen（正确长度），不要自己 sizeof(sockaddr_in) 瞎猜。
         */
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;      /* 连接成功，跳出 for；此时 sockfd 就是可用的连接 */

        /*
         * 这一条地址连不上（超时、拒绝等），必须 close 掉这个 sockfd，
         * 否则 fd 泄漏（进程能打开的文件/套接字数量有限）。
         */
        close(sockfd);
    }

    /* 若 rp 变成 NULL，说明链表里没有一条能连上 */
    if (rp == NULL)
        err_quit("connect error");

    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0;
        if (fputs(recvline, stdout) == EOF)
            err_sys("fputs error");
    }
    if (n < 0)
        err_sys("read error");

    /*
     * freeaddrinfo(res) — 释放 getaddrinfo 在堆上分配的整条链表。
     * 和 malloc 成对，不调就会内存泄漏；这是新手最容易忘的一步之一。
     */
    freeaddrinfo(res);

    exit(0);
}
