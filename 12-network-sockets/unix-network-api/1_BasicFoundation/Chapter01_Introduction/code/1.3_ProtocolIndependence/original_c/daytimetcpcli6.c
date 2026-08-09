/*
 * 【程序是干什么的】
 *   和 daytimetcpcli.c 一样：连 TCP 13 端口读 Daytime 时间字符串。
 *   区别：本文件写死使用 IPv6（AF_INET6 + sockaddr_in6）。
 *
 * 【你怎么运行】
 *   daytimetcpcli6 ::1
 *   或 daytimetcpcli6 2001:db8::1
 *   参数是 IPv6 地址（冒分十六进制），不是域名解析版。
 *
 * 【为什么原书还要这个文件】
 *   让你看到：从 IPv4 改成 IPv6 要换一整套类型和宏，仍然不能同时支持双栈。
 *   真正省事的做法是看 daytimetcpcligai.c（getaddrinfo）。
 */

#include "unp.h"

int
main(int argc, char **argv)
{
    int                  sockfd, n;
    char                 recvline[MAXLINE + 1];

    /*
     * sockaddr_in6：IPv6 版地址结构（128 位 IP，还有 flowinfo、scope_id 等字段）。
     * 和 IPv4 的 sockaddr_in 是两套不同的类型，不能混用。
     */
    struct sockaddr_in6  servaddr;

    if (argc != 2)
        err_quit("usage: daytimetcpcli6 <IPaddress>");

    /* 唯一和 IPv4 版差很多的地方之一：socket 的第一个参数改成 AF_INET6 */
    if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        err_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin6_family = AF_INET6;   /* 成员名是 sin6_ 开头，不是 sin_ */
    servaddr.sin6_port   = htons(13);  /* 端口号仍然用 htons，与 IP 版本无关 */

    /*
     * inet_pton 的第一个参数换成 AF_INET6，才能把 argv[1] 里的 IPv6 文本
     * 转成 16 字节的二进制地址放进 sin6_addr。
     */
    if (inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr) <= 0)
        err_quit("inet_pton error for %s", argv[1]);

    /*
     * 若连的是 fe80:: 这类“链路本地”地址，实际项目里往往还要设置 sin6_scope_id；
     * 原书 Daytime 示例为简化没有写，你知道有这回事即可。
     */

    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
        err_sys("connect error");

    /* 以下读—打印—退出逻辑与 IPv4 版完全相同，不再重复解释 C 语法 */
    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0;
        if (fputs(recvline, stdout) == EOF)
            err_sys("fputs error");
    }
    if (n < 0)
        err_sys("read error");

    exit(0);
}
