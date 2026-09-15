/* c4_5_write.c — write() 的返回值、部分写、以及「成功 ≠ 落盘」
 *
 * 演示六件事：
 *   1) write 返回实际写入字节数；写入普通文件通常一次写满
 *   2) count == 0 时 write 返回 0（合法，不是错误）
 *   3) /dev/full：写永远报 ENOSPC（磁盘满是什么样）
 *   4) SIGPIPE：写「读端已关闭」的管道——不忽略会被信号杀掉
 *   5) 忽略 SIGPIPE 后，同样的操作变成 -1 + EPIPE
 *   6) write 成功只到页缓存；fsync 才是落盘
 *
 * 编译: gcc -O0 -Wall -o c4_5_write c4_5_write.c
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

/* 往「读端已关闭」的管道写，在 SIGPIPE 默认动作下的下场 */
static void default_sigpipe_child(void)
{
    int p[2];
    pipe(p);
    close(p[0]);                        /* 读端关掉：已经没人听了 */
    printf("  [子进程] 读端已关闭，现在 write(p[1], ...) ...\n");
    fflush(stdout);
    ssize_t n = write(p[1], "x", 1);    /* 这一步会触发 SIGPIPE */
    printf("  [子进程] write 返回 %zd （走不到这里）\n", n);
    _exit(0);                           /* 不用 exit()，它会刷 stdio */
}

int main(void)
{
    char big[4096];
    memset(big, 'A', sizeof big);
    ssize_t n;

    printf("== 1. 正常写：普通文件一次写满 ==\n");
    const char *path = "/tmp/c4_write.txt";
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    n = write(fd, big, sizeof big);
    printf("  write(4096) -> %zd  （普通文件通常一次写完）\n", n);

    printf("\n== 2. count = 0：合法返回 0 ==\n");
    errno = 0;
    n = write(fd, big, 0);
    printf("  write(fd, buf, 0) -> %zd  errno=%d  （0 不是错误，是「写了 0 个字节」）\n", n, errno);
    close(fd);

    printf("\n== 3. /dev/full：永远 ENOSPC，模拟磁盘满 ==\n");
    int full = open("/dev/full", O_WRONLY);
    if (full >= 0) {
        errno = 0;
        n = write(full, big, sizeof big);
        printf("  write(/dev/full, 4096) -> %zd  errno=%d (%s)\n",
               n, errno, strerror(errno));
        close(full);
    } else {
        printf("  （本环境没有 /dev/full）\n");
    }

    printf("\n== 4. SIGPIPE 的默认动作：直接杀进程 ==\n");
    /* ⚠️ fork 之前必须 flush：stdout 被重定向时是全缓冲，缓冲区内容会被
       子进程一起继承，父子各刷一遍 → 前面的输出重复两遍。 */
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        default_sigpipe_child();
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        printf("  [父进程] 子进程被信号杀死：signal = %d", WTERMSIG(status));
        if (WTERMSIG(status) == SIGPIPE) printf(" = SIGPIPE");
        printf("\n  → 服务器写 socket 前不忽略它，一个断连就能带走整个进程。\n");
    } else {
        printf("  [父进程] 子进程正常退出 status=%d\n", WEXITSTATUS(status));
    }

    printf("\n== 5. 忽略 SIGPIPE：变成可处理的 EPIPE ==\n");
    signal(SIGPIPE, SIG_IGN);
    int p[2];
    pipe(p);
    close(p[0]);
    errno = 0;
    n = write(p[1], "x", 1);
    printf("  signal(SIGPIPE, SIG_IGN) 后 write -> %zd  errno=%d (%s)\n",
           n, errno, strerror(errno));
    printf("  → 进程活着，错误变成可判断、可恢复的返回值。这是网络服务的标准做法。\n");
    close(p[1]);

    printf("\n== 6. write 成功 ≠ 落盘 ==\n");
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    n = write(fd, "durable?", 8);
    printf("  write 返回 %zd —— 数据此刻在页缓存（脏页）里，还没进磁盘\n", n);
    int rc = fsync(fd);
    printf("  fsync 返回 %d —— 到这里才算真正刷到存储设备\n", rc);
#ifdef __linux__
    rc = fdatasync(fd);
    printf("  fdatasync 返回 %d —— 只保证数据部分，不保证元数据\n", rc);
#endif
    printf("  → 断电测试：write 之后立刻拔电，数据可能丢；fsync 之后拔电才安全。\n");
    close(fd);
    unlink(path);
    return 0;
}
