/*
 * UNP 1.5 · 图 1-8 · Daytime TCP 迭代服务器
 * 流程：Socket → Bind(INADDR_ANY:13) → Listen → 循环 Accept → Write → Close(connfd)
 *
 * 依赖：unp.h / libunp（Socket Bind Listen Accept Write Close）
 * 配对客户端：1.2 daytimetcpcli
 * 端口：标准 13（RFC865 Daytime）；非 root 可改为 10013，见 1.2_Appendix_Daytime端口13.md
 */
#include "unp.h"
#include <time.h>

int
main(int argc, char **argv)
{
    int                 listenfd, connfd;
    struct sockaddr_in  servaddr;
    char                buff[MAXLINE];
    time_t              ticks;

    /* ① 监听套接字：只用于 bind/listen/accept，永不 read/write */
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); /* 0.0.0.0 本机所有网卡 */
    servaddr.sin_port        = htons(13);         /* Daytime 标准端口，网络序 */

    Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);

    for (;;) {
        /* 阻塞直到有客户完成三次握手；返回 connfd = 已连接套接字 */
        connfd = Accept(listenfd, NULL, NULL);

        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));

        Write(connfd, buff, strlen(buff));

        Close(connfd);   /* 只关本会话；listenfd 继续下一轮 accept */
    }
}
