/* c5_2_fcntl.c — §5.2 fcntl()：一个系统调用，五类活
 *
 * fcntl 是 Ch5 的枢纽。本 demo 把它最常用的几组命令逐个跑一遍：
 *   F_GETFL / F_SETFL   —— 文件状态标志（后 3 位是访问模式）
 *   F_GETFD / F_SETFD   —— 文件描述符标志（只有 FD_CLOEXEC 一个）
 *   F_DUPFD / F_DUPFD_CLOEXEC —— 按「不小于 arg」复制 fd
 *   F_GETOWN / F_SETOWN —— 异步 I/O 的信号接收者（SIGIO/SIGURG 发给谁）
 *   错误路径：未知命令 → EINVAL，坏 fd → EBADF
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_2_fcntl c5_2_fcntl.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* 把 F_GETFL 的返回值里我们关心的位逐个人话化 */
static void show_fl(int fl)
{
    int acc = fl & O_ACCMODE;

    printf("    0x%05x 访问模式=%s%s%s%s%s\n", fl,
           acc == O_RDONLY ? "O_RDONLY" : acc == O_WRONLY ? "O_WRONLY" :
           acc == O_RDWR ? "O_RDWR" : "?",
           (fl & O_APPEND) ? " | O_APPEND" : "",
           (fl & O_NONBLOCK) ? " | O_NONBLOCK" : "",
           (fl & O_ASYNC) ? " | O_ASYNC(FASYNC)" : "",
           (fl & O_LARGEFILE) ? " | O_LARGEFILE(内核补的)" : "");
}

int main(void)
{
    const char *p = "/tmp/c5_fcntl.txt";
    int fd = open(p, O_RDWR | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        perror("open");
        return 1;
    }
    write(fd, "hello", 5);

    printf("== 1. F_GETFL / F_SETFL：文件状态标志 ==\n");
    printf("  open 时给的 O_RDWR，F_GETFL 读回来是：\n");
    show_fl(fcntl(fd, F_GETFL));
    printf("  F_SETFL(O_NONBLOCK) 之后：\n");
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1)
        perror("F_SETFL");
    show_fl(fcntl(fd, F_GETFL));
    printf("  注意 O_LARGEFILE(0x8000) 不是 open 时写的，是内核在 fs/open.c 里补的\n");

    printf("\n== 2. F_GETFD / F_SETFD：文件描述符标志（只有 CLOEXEC）==\n");
    printf("  初始 F_GETFD = 0x%x（FD_CLOEXEC=0x%x 未置位）\n", fcntl(fd, F_GETFD), FD_CLOEXEC);
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    printf("  F_SETFD(FD_CLOEXEC) 后 F_GETFD = 0x%x\n", fcntl(fd, F_GETFD));

    printf("\n== 3. F_DUPFD / F_DUPFD_CLOEXEC：按最小值复制 ==\n");
    {
        int a = fcntl(fd, F_DUPFD, 50);              /* 不小于 50 的最小可用号 */
        int b = fcntl(fd, F_DUPFD_CLOEXEC, 60);      /* 同上，且带 FD_CLOEXEC */

        printf("  F_DUPFD(50)         -> %d, F_GETFD=0x%x\n", a, a >= 0 ? fcntl(a, F_GETFD) : 0);
        printf("  F_DUPFD_CLOEXEC(60) -> %d, F_GETFD=0x%x（CLOEXEC 已在）\n",
               b, b >= 0 ? fcntl(b, F_GETFD) : 0);
        if (a >= 0) close(a);
        if (b >= 0) close(b);
    }

    printf("\n== 4. F_GETOWN / F_SETOWN：异步 I/O 的通知对象 ==\n");
    printf("  F_GETOWN 初始 = %d\n", fcntl(fd, F_GETOWN));
    if (fcntl(fd, F_SETOWN, getpid()) == -1)
        perror("F_SETOWN");
    printf("  F_SETOWN(getpid()=%ld) 后 F_GETOWN = %d\n", (long) getpid(), fcntl(fd, F_GETOWN));

    printf("\n== 5. 错误路径 ==\n");
    {
        /* 注意：调用必须先做完再取 errno。写成 printf(..., fcntl(...), errno)
           会因为求值顺序而读到调用之前的旧 errno（这里会错报成 0）。 */
        int rc = fcntl(fd, 0x7fffffff);          /* 未知命令 */

        printf("  fcntl(fd, 0x7fffffff) -> %d errno=%d (%s)\n",
               rc, errno, strerror(errno));
    }
    {
        int rc = fcntl(999, F_GETFL);            /* 坏 fd */

        printf("  fcntl(999, F_GETFL)   -> %d errno=%d (%s)\n",
               rc, errno, strerror(errno));
    }

    close(fd);
    return 0;
}
