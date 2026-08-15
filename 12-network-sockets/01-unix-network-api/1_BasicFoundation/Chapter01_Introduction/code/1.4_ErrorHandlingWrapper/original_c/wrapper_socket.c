/*
 * UNP 1.4 · 包裹函数示例（图 1-7 风格）
 * Socket / Connect / Accept — 失败统一 err_sys 并 exit
 *
 * 说明：err_sys 由 unp.h / libunp 提供；本文件展示「大写包裹」写法。
 * 可与 1.2 daytimetcpcli 对照：把 socket/connect 换成 Socket/Connect。
 */
#include "unp.h"

/* 创建套接字；成功返回 fd，失败 err_sys 不返回 */
int
Socket(int family, int type, int protocol)
{
    int fd;

    if ((fd = socket(family, type, protocol)) < 0)
        err_sys("socket error");
    return fd;
}

/* 主动连接；成功返回 0，失败 err_sys */
void
Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
    if (connect(fd, sa, salen) < 0)
        err_sys("connect error");
}

/*
 * 被动接受连接；成功返回已连接 fd，失败 err_sys
 * 注意：生产 accept 常需单独处理 EINTR，不能用本 Accept 一刀切（见 1.4 笔记 §七）
 */
int
Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
    int n;

    if ((n = accept(fd, sa, salenptr)) < 0)
        err_sys("accept error");
    return n;
}
