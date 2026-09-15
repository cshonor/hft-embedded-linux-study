/* ex4_1_tee.c — TLPI 练习 4-1：用 I/O 系统调用实现 tee
 *
 * 【题干（原书 Exercise 4-1，逐字）】
 *   The tee command reads its standard input until end-of-file, writing a
 *   copy of the input to standard output and to the file named in its
 *   command-line argument.  Implement tee using I/O system calls.  By
 *   default, tee overwites any existing file with the given name.
 *   Implement the -a command-line option (tee -a file), which casuses tee
 *   to append text to the end of a file if it already exists.
 *
 * 【实现要点】
 *   - 一份输入，两处输出：stdout 一份、文件一份
 *   - 循环 read 到 0（EOF）；每轮把读到的 n 字节写两处
 *   - -a：打开文件时加 O_APPEND（而不是 O_TRUNC）
 *   - getopt(3) 解析选项；用完记得检查 optind == argc - 1（恰好一个文件参数）
 *
 * 【踩坑提示】写 stdout 也可能发生部分写（stdout 可能是管道且缓冲区满），
 *   所以两处都要用 writeAll 循环，不能只 `write(...)` 一下就走。
 *
 * 编译: gcc -O0 -Wall -o ex4_1_tee ex4_1_tee.c
 * 用法: echo hello | ./ex4_1_tee out.txt
 *       echo world | ./ex4_1_tee -a out.txt
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define BUF_SIZE 1024

/* 把 buf 里 len 字节完整写到 fd：处理部分写和 EINTR */
static ssize_t writeAll(int fd, const char *buf, size_t len)
{
    size_t written = 0;

    while (written < len) {
        ssize_t n = write(fd, buf + written, len - written);
        if (n < 0) {
            if (errno == EINTR)
                continue;               /* 被信号打断：数据没写、不是错误，重试 */
            return -1;                  /* 真错误 */
        }
        written += (size_t) n;
    }
    return (ssize_t) written;
}

int main(int argc, char *argv[])
{
    int append = 0;
    int opt;

    while ((opt = getopt(argc, argv, "a")) != -1) {
        switch (opt) {
        case 'a': append = 1;  break;
        default:
            fprintf(stderr, "Usage: %s [-a] file\n", argv[0]);
            return 1;
        }
    }

    if (optind != argc - 1) {
        fprintf(stderr, "Usage: %s [-a] file\n", argv[0]);
        return 1;
    }
    const char *path = argv[optind];

    int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
    int fd = open(path, flags, 0644);
    if (fd < 0) {
        fprintf(stderr, "open %s: %s\n", path, strerror(errno));
        return 1;
    }

    char buf[BUF_SIZE];
    ssize_t n;
    int rounds = 0;

    while ((n = read(STDIN_FILENO, buf, sizeof buf)) > 0) {
        rounds++;
        if (writeAll(fd, buf, (size_t) n) < 0) {   /* 去处一：文件 */
            fprintf(stderr, "write %s: %s\n", path, strerror(errno));
            return 1;
        }
        if (writeAll(STDOUT_FILENO, buf, (size_t) n) < 0) {  /* 去处二：屏幕 */
            fprintf(stderr, "write stdout: %s\n", strerror(errno));
            return 1;
        }
    }
    if (n < 0) {
        fprintf(stderr, "read: %s\n", strerror(errno));
        return 1;
    }

    fprintf(stderr, "[ex4_1_tee] 读了 %d 轮，模式 = %s\n",
            rounds, append ? "append (-a)" : "overwrite (默认 O_TRUNC)");
    close(fd);
    return 0;
}
