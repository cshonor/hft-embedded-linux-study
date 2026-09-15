/* ex5_1_large_file_offset_bits.c — 原书习题 5-1
 *
 * 题目（逐字）：
 *   Modify the program in listing 5-3 to use standard file I/O system
 *   calls (open() and lseek()) and the off_t data type.  Compile the
 *   program with the _FILE_OFFSET_BITS macro set to 64, and test it to
 *   show that a large file can be successfully created.
 *
 * 原书 Listing 5-3（large_file.c）用的是**显式 LFS API**：open64()/lseek64()/off64_t。
 * 本题要求改成标准接口 + off_t，靠编译期宏 _FILE_OFFSET_BITS=64 把 off_t 悄悄换成
 * 64 位——这样同一份源码在 32 位平台上也能处理大文件。
 *
 * 编译: gcc -O0 -Wall -Wextra -o ex5_1_large_file_offset_bits ex5_1_large_file_offset_bits.c
 */
#define _FILE_OFFSET_BITS 64        /* 关键：让 off_t / lseek / open 全部走 64 位 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>

static volatile sig_atomic_t xfsz = 0;

static void on_xfsz(int sig)
{
    (void) sig;
    xfsz = 1;
}

int main(int argc, char *argv[])
{
    const char *path;
    off_t off;
    int fd;
    struct rlimit rl;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s pathname offset\n", argv[0]);
        return 1;
    }
    path = argv[1];
    off = (off_t) atoll(argv[2]);

    printf("== 习题 5-1：标准接口 + _FILE_OFFSET_BITS=64 ==\n");
    printf("    _FILE_OFFSET_BITS = %d\n", _FILE_OFFSET_BITS);
    printf("    sizeof(off_t) = %zu 字节（32 位平台默认 4；靠上面的宏变 8）\n",
           sizeof(off_t));
    if (getrlimit(RLIMIT_FSIZE, &rl) == 0)
        printf("    RLIMIT_FSIZE soft=%lld hard=%lld\n",
               (long long) rl.rlim_cur, (long long) rl.rlim_max);

    fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    printf("\n    请求偏移 = %lld（4 GiB 的 off_t 只有 8 字节类型才装得下）\n",
           (long long) off);
    if (lseek(fd, off, SEEK_SET) == (off_t) -1) {
        perror("lseek");
        return 1;
    }
    printf("    lseek -> %lld 成功\n", (long long) lseek(fd, 0, SEEK_CUR));

    /* 装个 handler，好让超限时 write 返回 EFBIG 而不是把进程杀掉 */
    {
        struct sigaction sa;

        memset(&sa, 0, sizeof sa);
        sa.sa_handler = on_xfsz;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGXFSZ, &sa, NULL);
    }

    errno = 0;
    {
        ssize_t n = write(fd, "test", 4);

        if (n == -1) {
            printf("    write -> -1 errno=%d (%s)，SIGXFSZ handler=%d\n",
                   errno, strerror(errno), (int) xfsz);
            printf("    ↑ 拦住它的是 RLIMIT_FSIZE（沙箱 16 MiB），不是 off_t 宽度\n");
            printf("      在真实机器上把 ulimit -f 放开，这一步就能写出真正的大文件\n");
        } else {
            printf("    write -> %ld 字节，文件创建成功\n", (long) n);
        }
    }

    close(fd);
    return 0;
}
