/* c4_4_read.c — read() 的三种返回值：正数（可能短读）/ 0（EOF）/ -1（错误）
 *
 * 演示五件事：
 *   1) 循环读到 EOF 的正确写法
 *   2) 短读是正常现象：小 buffer 读大文件，一次读不满
 *   3) EBADF：fd 无效，或没以读方式打开
 *   4) EFAULT：buf 指针非法（用户态内存 bug 的暴露点）
 *   5) 读管道时的短读：有多少读多少，绝不等待
 *
 * 编译: gcc -O0 -Wall -o c4_4_read c4_4_read.c
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

int main(void)
{
    char buf[64];
    ssize_t n;

    printf("== 1. 循环读到 EOF ==\n");
    const char *path = "/tmp/c4_read.txt";
    int w = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    const char *text = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";   /* 36 字节 */
    write(w, text, 36);
    close(w);

    int fd = open(path, O_RDONLY);
    int rounds = 0, total = 0;
    while ((n = read(fd, buf, sizeof buf)) > 0) {     /* 只在 >0 时继续 */
        rounds++;
        total += (int) n;
        printf("  第 %d 次 read(64) -> %2zd 字节\n", rounds, n);
    }
    printf("  退出循环时 read 返回 %zd（0 = EOF，不是错误）\n", n);
    printf("  合计 %d 字节，%d 轮\n", total, rounds);

    printf("\n== 2. 短读：请求 8 字节，可文件只剩 6 字节 ==\n");
    off_t end = lseek(fd, 0, SEEK_END);
    lseek(fd, end - 6, SEEK_SET);            /* 挪到最后 6 个字节 */
    char small[8];
    n = read(fd, small, sizeof small);       /* 请求 8，实际只有 6 可读 */
    printf("  在「剩余 6 字节」处 read(8) -> %zd 字节  ← 短读！请求 8，只给 6\n", n);
    n = read(fd, small, sizeof small);
    printf("  再 read(8) -> %zd  ← 0 = EOF\n", n);
    printf("  → 短读不是错误，它是 read 的**正常**返回值之一。\n");
    printf("    管道/终端/socket 上「返回 < 请求」更是家常便饭。\n");
    close(fd);

    printf("\n== 3. EBADF：fd 侧的问题 ==\n");
    int wfd = open(path, O_WRONLY);        /* 以「只写」打开 */
    errno = 0;
    n = read(wfd, buf, 10);
    printf("  用只写 fd 去 read -> %zd  errno=%d (%s)\n", n, errno, strerror(errno));
    close(wfd);

    errno = 0;
    n = read(999, buf, 10);                /* 一个没打开过的号 */
    printf("  用 fd=999 去 read -> %zd  errno=%d (%s)\n", n, errno, strerror(errno));

    printf("\n== 4. EFAULT：buf 侧的问题（地址非法）==\n");
    fd = open(path, O_RDONLY);
    volatile long bad_addr = 0x1;            /* volatile：让 gcc 别静态推断出「目标在地址 0」 */
    errno = 0;
    n = read(fd, (void *) bad_addr, 10);     /* 这个地址落在内核区，access_ok 挡下 */
    printf("  read(fd, (void*)0x1, 10) -> %zd  errno=%d (%s)\n",
           n, errno, strerror(errno));
    printf("  → EBADF 查「句柄」，EFAULT 查「内存」。两个不同的坑。\n");
    close(fd);

    printf("\n== 5. 从管道读：有多少读多少 ==\n");
    int p[2];
    pipe(p);
    write(p[1], "abc", 3);                 /* 只放 3 字节 */
    n = read(p[0], buf, sizeof buf);        /* 却要 64 字节 */
    printf("  管道里只有 3 字节，read(64) -> %zd 字节\n", n);
    printf("  → 管道上 read 不会「等到凑满 64 字节」再返回。\n");
    write(p[1], "de", 2);
    n = read(p[0], buf, sizeof buf);
    printf("  再放 2 字节，read(64) -> %zd 字节\n", n);
    close(p[0]);
    close(p[1]);

    printf("\n== 6. count 远超 buffer 实际大小，会怎样 ==\n");
    fd = open(path, O_RDONLY);
    volatile size_t huge = (size_t) 1 << 30;   /* 1 GiB；volatile 防编译器静态告警 */
    errno = 0;
    n = read(fd, buf, huge);                   /* buf 实际只有 64 字节！ */
    printf("  read(fd, buf[64], 1GiB) -> %zd  errno=%d (%s)\n",
           n, errno, strerror(errno));
    printf("  → 居然成功。因为内核只知道「从 buf 起、长 1GiB」这个范围落在用户空间里，\n");
    printf("    它无从知道 buf 真正分配了多少字节。文件只有 36 字节，所以只拷了 36 字节。\n");
    printf("  ⚠️ 若这个文件真有 1GiB，这行就会往 64 字节的栈缓冲里灌 1GiB —— 静默栈溢出。\n");
    printf("  → count 必须由「缓冲区实际大小」决定，内核不替你把关。\n");
    close(fd);
    unlink(path);
    return 0;
}
