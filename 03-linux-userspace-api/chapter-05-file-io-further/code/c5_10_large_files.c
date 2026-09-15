/* c5_10_large_files.c — §5.10 大文件：off_t 不够宽会怎样，以及 RLIMIT_FSIZE
 *
 * 32 位 off_t 最大表示到 2 GiB - 1。要碰更大的文件有两种办法：
 *   1) 编译期 #define _FILE_OFFSET_BITS 64 —— 把 off_t 悄悄换成 64 位（推荐）
 *   2) 用显式的 LFS API：open64() / lseek64() / off64_t（已过时）
 * 在 64 位系统上 off_t 本来就是 8 字节，所以"打开大文件"这件事天然成立——
 * 真正会拦住你的是别的东西：**RLIMIT_FSIZE**。
 *
 * 本 demo 演示五件事：
 *   1) 编译期事实：sizeof(off_t) / sizeof(long) / _FILE_OFFSET_BITS
 *   2) 运行时事实：RLIMIT_FSIZE（CE 沙箱里是 16 MiB）
 *   3) lseek() 到 4 GiB 是允许的，而且**不会**让文件变大（游标不等于文件大小）
 *   4) 在超限位置 write()：默认动作是收到 SIGXFSZ 被杀；装了 handler 才是 EFBIG
 *   5) F_GETFL 里的 O_LARGEFILE（0x8000）是内核补的，不是你 open 时写的
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_10_large_files c5_10_large_files.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define FOUR_GB ((off_t) 4 * 1024 * 1024 * 1024)

/* 注意这里有个「错位」：x86-64 上 glibc 把 O_LARGEFILE 定义成 **0**
   （off_t 本来就是 64 位，用户态没什么要做的），而内核侧的位仍是 0x8000。
   下面两个宏分别代表两边，别混。 */
#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif
#define KERNEL_O_LARGEFILE 0x8000

static volatile sig_atomic_t xfsz_hit = 0;

static void on_xfsz(int sig)
{
    (void) sig;
    xfsz_hit = 1;                       /* 只记一笔，系统调用本身会返回 EFBIG */
}

int main(void)
{
    const char *p = "/tmp/c5_large.bin";
    int fd;
    struct stat st;
    struct rlimit rl;

    printf("== 1. 编译期事实 ==\n");
    printf("    sizeof(off_t) = %zu 字节, sizeof(long) = %zu 字节\n",
           sizeof(off_t), sizeof(long));
    printf("    _FILE_OFFSET_BITS = %d（本文件里没自定义，看编译器默认）\n",
#ifdef _FILE_OFFSET_BITS
           _FILE_OFFSET_BITS
#else
           0
#endif
          );
    printf("    O_LARGEFILE 这个宏 = 0x%x\n", O_LARGEFILE);

    printf("\n== 2. 运行时事实：RLIMIT_FSIZE ==\n");
    if (getrlimit(RLIMIT_FSIZE, &rl) == 0)
        printf("    soft = %lld   hard = %lld（字节）\n",
               (long long) rl.rlim_cur, (long long) rl.rlim_max);
    printf("    超过这个大小写文件，内核会先发 SIGXFSZ，再让 write 返回 EFBIG\n");

    fd = open(p, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    write(fd, "small", 5);

    printf("\n== 3. lseek() 到 4 GiB：允许，且文件不会变大 ==\n");
    {
        off_t o = lseek(fd, FOUR_GB, SEEK_SET);

        printf("    lseek(fd, 4 GiB, SEEK_SET) -> %lld\n", (long long) o);
        fstat(fd, &st);
        printf("    此时 st_size = %lld（还是 5）、st_blocks*512 = %lld\n",
               (long long) st.st_size, (long long) st.st_blocks * 512);
        printf("    f_pos 是「游标」，不是「文件长度」——两者可以差很远\n");
    }

    printf("\n== 4. 在 4 GiB 处 write(). 两条路 ==\n");
    printf("  (a) 不处理 SIGXFSZ（默认动作 = 杀进程）：\n");
    fflush(stdout);
    {
        pid_t pid = fork();

        if (pid == 0) {
            signal(SIGXFSZ, SIG_DFL);       /* 子进程用默认动作 */
            if (write(fd, "test", 4) == -1)
                _exit(1);
            _exit(0);
        } else if (pid > 0) {
            int status = 0;

            waitpid(pid, &status, 0);
            if (WIFSIGNALED(status))
                printf("      子进程被信号 %d (%s) 杀死\n",
                       WTERMSIG(status), strsignal(WTERMSIG(status)));
            else if (WIFEXITED(status))
                printf("      子进程正常退出，code=%d\n", WEXITSTATUS(status));
        }
    }

    printf("  (b) 装上 SIGXFSZ 处理函数：write 自己返回 EFBIG\n");
    {
        struct sigaction sa;

        memset(&sa, 0, sizeof sa);
        sa.sa_handler = on_xfsz;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGXFSZ, &sa, NULL);

        errno = 0;
        ssize_t n = write(fd, "test", 4);

        printf("      write -> %ld errno=%d (%s)，handler 被调用 = %d\n",
               (long) n, errno, strerror(errno), (int) xfsz_hit);
        fstat(fd, &st);
        printf("      文件没被写脏：st_size = %lld（还是 5）\n", (long long) st.st_size);
    }

    printf("\n== 5. O_LARGEFILE：glibc 的宏是 0，内核的位是 0x8000 ==\n");
    printf("    glibc 的 O_LARGEFILE 宏        = 0x%x\n", (int) O_LARGEFILE);
    printf("    内核侧的位 (F_GETFL & 0x8000)  = %d\n",
           !!(fcntl(fd, F_GETFL) & KERNEL_O_LARGEFILE));
    printf("    F_GETFL = 0x%05x\n", fcntl(fd, F_GETFL));
    printf("    两边不一致不是 bug：用户态这个宏在 x86-64 上被定成 0（没事可做），\n");
    printf("    内核侧的 0x8000 由 fs/open.c 自己补，所以在 F_GETFL 里看得见、\n");
    printf("    在 glibc 的宏里看不见 —— 这个错位正是 Ch4 里 0x8000 之谜的答案\n");

    close(fd);
    unlink(p);
    return 0;
}
