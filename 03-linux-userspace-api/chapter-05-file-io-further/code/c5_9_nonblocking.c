/* c5_9_nonblocking.c — §5.9 非阻塞 I/O：O_NONBLOCK 到底改了谁的行为
 *
 * 关键在于「内核发现现在做不了这件事」时怎么办：
 *   阻塞模式  —— 把进程挂起（睡在等待队列里），直到能做为止
 *   非阻塞模式 —— 立刻返回 -1，errno = EAGAIN (EWOULDBLOCK)
 *
 * 而且这个开关**可以在运行时改**（F_SETFL），不用重新 open。
 *
 * 演示四件事：
 *   1) 空管道读：非阻塞 → EAGAIN
 *   2) 同一根管道换回阻塞模式 → read 挂起，被 SIGALRM 打断（EINTR）
 *   3) 有数据时阻塞读立刻返回
 *   4) 管道写满：非阻塞 → EAGAIN（阻塞模式会一直等到有人读）
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_9_nonblocking c5_9_nonblocking.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static void on_alarm(int sig)
{
    (void) sig;                              /* 只用来打断阻塞的 read() */
}

int main(void)
{
    int pp[2];
    char buf[256];
    ssize_t n;

    if (pipe(pp) == -1) {
        perror("pipe");
        return 1;
    }

    printf("== 1. 空管道 + O_NONBLOCK：立刻 EAGAIN，绝不等待 ==\n");
    fcntl(pp[0], F_SETFL, fcntl(pp[0], F_GETFL) | O_NONBLOCK);
    errno = 0;
    n = read(pp[0], buf, sizeof buf);
    printf("    read(空管道, O_NONBLOCK) -> %ld errno=%d (%s)\n",
           (long) n, errno, strerror(errno));
    printf("    EAGAIN=%d，它的别名 EWOULDBLOCK=%d（同一数值）\n", EAGAIN, EWOULDBLOCK);

    printf("\n== 2. 换回阻塞模式：read 会挂起，靠信号把它打断 ==\n");
    fcntl(pp[0], F_SETFL, fcntl(pp[0], F_GETFL) & ~O_NONBLOCK);
    printf("    现在 F_GETFL 里的 O_NONBLOCK 位 = %d\n",
           !!(fcntl(pp[0], F_GETFL) & O_NONBLOCK));
    {
        struct sigaction sa;

        memset(&sa, 0, sizeof sa);
        sa.sa_handler = on_alarm;
        sigemptyset(&sa.sa_mask);
        /* 注意：**不能**带 SA_RESTART，否则被信号打断的系统调用会自动重来 */
        sa.sa_flags = 0;
        sigaction(SIGALRM, &sa, NULL);
        alarm(1);
        errno = 0;
        n = read(pp[0], buf, sizeof buf);        /* 挂在这一行上，1 秒后被 SIGALRM 打断 */
        alarm(0);
        printf("    阻塞 read(空管道, 无数据) -> %ld errno=%d (%s)\n",
               (long) n, errno, strerror(errno));
        printf("    EINTR=%d —— 慢系统调用被信号打断的经典现场\n", EINTR);
    }

    printf("\n== 3. 有数据时阻塞读立刻返回 ==\n");
    write(pp[1], "hello", 5);
    memset(buf, 0, sizeof buf);
    n = read(pp[0], buf, sizeof buf);
    printf("    read -> %ld 字节 \"%.*s\"\n", (long) n, (int) n, buf);

    printf("\n== 4. 写满的管道 + O_NONBLOCK：EAGAIN ==\n");
    {
        char chunk[1024];
        long total = 0;

        memset(chunk, 'w', sizeof chunk);
        fcntl(pp[1], F_SETFL, fcntl(pp[1], F_GETFL) | O_NONBLOCK);
        for (;;) {
            n = write(pp[1], chunk, sizeof chunk);
            if (n <= 0)
                break;
            total += n;
        }
        printf("    一口气塞进 %ld 字节后停下（errno=%d %s）\n",
               total, errno, strerror(errno));
        printf("    F_GETPIPE_SZ 报告的容量 = %d\n", fcntl(pp[1], F_GETPIPE_SZ));
        errno = 0;
        n = write(pp[1], "x", 1);
        printf("    再写 1 字节 -> %ld errno=%d (%s)\n", (long) n, errno, strerror(errno));
    }

    close(pp[0]);
    close(pp[1]);
    return 0;
}
