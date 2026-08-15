/*
 * 【程序是干什么的】
 *   连到一台机器的 TCP 13 端口（Daytime 服务），把对方发来的“当前时间”文字打印到屏幕。
 *
 * 【你怎么运行】（命令行里，先编译再执行，下面只是说明格式）
 *   daytimetcpcli 127.0.0.1
 *   最后一个参数是服务器的 IPv4 地址（点分十进制，如 192.168.1.1）。
 *
 * 【和 1.3 其它两个文件的关系】
 *   daytimetcpcli.c   — 本文件，写死只用 IPv4
 *   daytimetcpcli6.c  — 写死只用 IPv6
 *   daytimetcpcligai.c — 不写死协议，用 getaddrinfo 自动选 v4/v6
 *
 * 【端口 13】RFC865 Daytime 标准端口；非 root 自测服务端可改 10013（见 1.2_Appendix_Daytime端口13.md）
 *
 * 【头文件 unp.h】
 *   原书 Stevens 提供的公共头，里面声明了 socket、err_quit、MAXLINE 等，还要链接 libunp。
 */

#include "unp.h"   /* 把原书封装好的网络/错误处理声明都包含进来 */

/*
 * main：C 程序入口。操作系统启动本程序时，从 main 开始执行。
 *
 * argc：参数个数（整数）。例如只输入程序名则 argc==1；程序名+IP 则 argc==2。
 * argv：参数字符串数组（指针的指针）。argv[0] 通常是程序名，argv[1] 是第一个额外参数（这里是要连的 IP）。
 */
int
main(int argc, char **argv)
{
    /*
     * 变量声明（在使用前先声明类型和名字，是经典 C 的写法）：
     *   sockfd — 整数，代表“套接字”在内核里的编号，像文件的 fd；
     *   n      — 整数，read() 一次读到了多少字节。
     */
    int                 sockfd, n;

    /*
     * char 数组 = 一串字符的缓冲区。
     * MAXLINE 在 unp.h 里定义（常见是 4096），+1 是为了最后多放一个 '\0'，
     * 这样 C 才把它当成“字符串”（以空字符结尾的文本）。
     */
    char                recvline[MAXLINE + 1];

    /*
     * struct sockaddr_in：专门存“IPv4 地址 + 端口”的结构体。
     * 本程序“写死”用这种结构，所以只能连 IPv4（这叫协议相关/硬编码）。
     */
    struct sockaddr_in  servaddr;

    /* argc != 2 表示：没有多给一个 IP 参数，用法不对，直接退出并打印提示 */
    if (argc != 2)
        err_quit("usage: daytimetcpcli <IPaddress>");

    /*
     * socket()：向内核申请一个套接字（网络通信的端点）。
     *   AF_INET      — 用 IPv4 地址族
     *   SOCK_STREAM  — 字节流类型，即 TCP（可靠、有序）
     *   第三个参数 0 — 让系统选默认协议（对 TCP 就是 IPPROTO_TCP）
     * 返回值赋给 sockfd；若 < 0 表示失败，err_sys 会打印 errno 并退出。
     */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_sys("socket error");

    /*
     * ② 填写“要连到哪台服务器”
     * bzero：把 servaddr 整块内存清零，避免结构体里残留垃圾字节。
     * sizeof(servaddr) 表示这一块有多少字节。
     */
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;   /* 必须和上面 socket 的 AF_INET 一致 */

    /*
     * 端口号 13 = Internet 标准里的 Daytime 服务。
     * htons：host to network short，把“本机字节序”的短整数转成“网络字节序”
     * （不同 CPU 存多字节数的顺序可能不同，在网络上要统一格式）。
     */
    servaddr.sin_port   = htons(13);

    /*
     * inet_pton：把人类可读的 IP 字符串（argv[1]）转成二进制，放进 sin_addr。
     *   AF_INET — 按 IPv4 解析
     *   返回值 <= 0 — 说明字符串不是合法 IPv4
     */
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        err_quit("inet_pton error for %s", argv[1]);

    /*
     * connect()：客户端主动连服务器，内核会完成 TCP 三次握手。
     *   (SA *) &servaddr — 把 sockaddr_in* 强转成通用 sockaddr*（BSD 传统写法）
     *   sizeof(servaddr) — 告诉内核这个地址结构有多大
     */
    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
        err_sys("connect error");

    /*
     * ④ 循环读数据
     * read(sockfd, recvline, MAXLINE)：
     *   从连接里最多读 MAXLINE 字节到 recvline；
     *   返回值 > 0 — 读到了 n 字节；
     *   返回值 == 0 — 对端关闭连接（常见是发了 FIN）；
     *   返回值 < 0 — 出错。
     * while (n > 0) 表示：只要还能读到数据，就继续循环。
     */
    while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0;    /* 在第 n 个位置放字符串结束符 '\0'，后面当文本用 */

        /* fputs：把字符串输出到标准输出 stdout（你的终端屏幕） */
        if (fputs(recvline, stdout) == EOF)
            err_sys("fputs error");
    }

    /* 若 while 是因为 n<0 退出的，说明 read 出错 */
    if (n < 0)
        err_sys("read error");

    /*
     * exit(0) — 正常结束进程，0 表示成功。
     * 进程结束时内核会关闭它打开的文件描述符（包括 sockfd）。
     * 学习阶段可记住：更规范的做法是 exit 前 close(sockfd)。
     */
    exit(0);
}
