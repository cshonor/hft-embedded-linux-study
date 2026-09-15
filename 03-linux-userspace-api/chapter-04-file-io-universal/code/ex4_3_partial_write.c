/* ex4_3_partial_write.c — 亲手制造一次「部分写」，看 writeAll 怎么救回来
 *
 * 【为什么需要这个 demo】
 *   原书 Listing 4-1（copy.c）写的是：
 *        if (write(outputFd, buf, numRead) != numRead)
 *            fatal("write() returned error or partial write occurred");
 *   它敢这么写，是因为它只拷普通文件——普通文件上部分写几乎不出现。
 *   但只要换成管道/socket/磁盘快满，部分写就是**常态**，`!=` 直接报错
 *   就等于把「可恢复的慢」当成「不可恢复的错」。
 *
 *   这个 demo 用 pipe + 缩小管道缓冲 + 非阻塞，强制内核只写一部分，
 *   让「部分写」从书本概念变成屏幕上看得见的数字。
 *
 * 编译: gcc -O0 -Wall -o ex4_3_partial_write ex4_3_partial_write.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

/* 通用写法：循环直到写完（处理部分写 + EINTR） */
static ssize_t writeAll(int fd, const char *buf, size_t len)
{
    size_t written = 0;
    while (written < len) {
        ssize_t n = write(fd, buf + written, len - written);
        if (n < 0) {
            if (errno == EINTR)    continue;   /* 被打断：重试 */
            if (errno == EAGAIN)   break;      /* 非阻塞且缓冲满：先收工，稍后再来 */
            return -1;
        }
        written += (size_t) n;
    }
    return (ssize_t) written;
}

int main(void)
{
    char buf[8192];
    memset(buf, 'Z', sizeof buf);

    printf("== 1. 造一个「容量很小」的管道 ==\n");
    int p[2];
    if (pipe(p) < 0) { perror("pipe"); return 1; }

    int defsz = fcntl(p[1], F_GETPIPE_SZ);
    printf("  默认管道容量 F_GETPIPE_SZ = %d 字节\n", defsz);

    /* 调小到一页，便于观察「写不下」 */
    int want = 4096;
    if (fcntl(p[1], F_SETPIPE_SZ, want) < 0)
        printf("  F_SETPIPE_SZ(%d) 失败: %s（继续用默认容量）\n", want, strerror(errno));
    int nowsz = fcntl(p[1], F_GETPIPE_SZ);
    printf("  现在容量 F_GETPIPE_SZ = %d 字节\n", nowsz);

    printf("\n== 2. 非阻塞写「远超容量」的数据 → 必然部分写 ==\n");
    int fl = fcntl(p[1], F_GETFL);
    fcntl(p[1], F_SETFL, fl | O_NONBLOCK);       /* 不阻塞，写不下就立刻返回 */

    size_t want_write = sizeof buf;              /* 8192 字节 */
    errno = 0;
    ssize_t n = write(p[1], buf, want_write);    /* 先来一次「裸 write」 */
    printf("  裸 write(p[1], buf, %zu) -> %zd 字节", want_write, n);
    if (n < (ssize_t) want_write)
        printf("   ← 部分写！请求 %zu，只写了 %zd\n", want_write, n);
    else
        printf("\n");

    printf("\n== 3. 同一件事：writeAll 循环版，把剩余的都补上 ==\n");
    /* 先把管道清空，重新来过。
       注意：读端也要设非阻塞，否则「读空管道」会一直阻塞下去
       （这就是本 demo 第一版在 CE 上跑满 20 秒被 kill 的原因）。 */
    fcntl(p[0], F_SETFL, fcntl(p[0], F_GETFL) | O_NONBLOCK);
    { char drain[8192]; while (read(p[0], drain, sizeof drain) > 0) ; }

    size_t total = 0;
    for (int spins = 0; spins < 100 && total < want_write; spins++) {
        ssize_t w = writeAll(p[1], buf + total, want_write - total);
        if (w < 0) { perror("writeAll"); break; }
        total += (size_t) w;
        printf("  writeAll 一轮写出 %zd 字节，累计 %zu / %zu\n", w, total, want_write);
        if (total < want_write) {
            char drain[8192];                      /* 缓冲满了：真实程序此时该等「可写事件」 */
            ssize_t got = read(p[0], drain, sizeof drain);
            printf("    管道满，读端消费 %zd 字节后继续\n", got);
            if (got <= 0) break;                   /* 真读不到就收手，别空转 */
        }
    }
    printf("  writeAll 最终写出 %zu 字节（请求 %zu）\n", total, want_write);

    printf("\n== 4. 结论 ==\n");
    printf("  同一份数据、同一次请求，内核可以只写一部分就返回。\n");
    printf("  所以：write 的返回值必须用，且要用循环；`!= count 就报错` 只适用于\n");
    printf("  「只写普通文件」这种受控场景（原书 Listing 4-1 正是这个场景）。\n");
    close(p[0]);
    close(p[1]);
    return 0;
}
