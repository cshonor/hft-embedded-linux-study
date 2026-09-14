/*
 * c5_3_read_semantics.c —— 「卡住」的源头：read(2) 的三个反直觉返回值
 *
 * 用途：strace 里最常看到的一行是 `read(0, "...", 4096) = N`。新手最大的
 *       困惑是「为什么 N 不等于 4096」「为什么 N 会是 0」。这份程序把 read 的
 *       三种返回值**自己打印出来**，和 strace 的那一行一一对应：
 *
 *         N > 0            读到了 N 字节（可能远小于请求值 —— 部分读）
 *         N == 0           EOF：对端关闭 / 文件读完（★ 不是「读失败」）
 *         N == -1          失败，看 errno（EINTR=被信号打断、EAGAIN=暂无可读）
 *
 *       而 strace 里「进程卡住」的现场，就是一行**没有返回值**的 read —— 
 *       因为它还没返回。
 *
 * 用法：./c5_3_read_semantics < data.txt
 *
 * 编译：gcc -g -O0 -Wall -Wextra -o c5_3_read_semantics c5_3_read_semantics.c
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    char buf[16];                 /* 故意开得很小，逼出「部分读」 */
    ssize_t n;
    int     round = 0;

    printf("每轮请求 16 字节，逐轮打印 read 的真实返回值\n");
    printf("（对照 strace：read(0, \"...\", 16) = N  —— N 就是下面这个数）\n\n");

    for (;;) {
        errno = 0;
        n = read(STDIN_FILENO, buf, sizeof buf);

        if (n > 0) {
            printf("第 %d 轮: read(0, buf, 16) = %2ld   请求 16 得到 %ld"
                   " %s  内容=\"%.*s\"\n",
                   ++round, (long)n, (long)n,
                   n < 16 ? "← 部分读（不足请求值，正常！）" : "",
                   (int)n, buf);
        } else if (n == 0) {
            printf("第 %d 轮: read(0, buf, 16) =  0   ← EOF（对端关闭 / 数据读完）\n", ++round);
            printf("\nread 返回 0 = 正常的「读完了」，不是错误。\n");
            printf("strace 里这一行之后通常就是 close / exit_group。\n");
            break;
        } else {
            printf("第 %d 轮: read(0, buf, 16) = -1   errno=%d(%s)\n",
                   ++round, errno, strerror(errno));
            if (errno == EINTR) {
                printf("  EINTR = 被信号打断。这不是数据错误，重试即可（记得处理！）\n");
                continue;
            }
            printf("  这是真错误，退出。\n");
            return 1;
        }
    }
    return 0;
}
