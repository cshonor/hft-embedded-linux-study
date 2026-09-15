/* ex5_7_readv_by_read.c — 原书习题 5-7
 *
 * 题目（逐字）：
 *   Implement readv() and writev() using read() and write(), and
 *   suitable functions from the malloc package (Section 7.1.2).
 *
 * 做法：
 *   my_writev = 先把各段"聚集"到一块 malloc 出来的连续内存，再一次 write()
 *   my_readv  = 先 read() 到一块连续内存，再按 iov 切分"分散"拷到各段
 * 注意短读/短写：n 可能小于总长，切分时不能越界。
 *
 * 编译: gcc -O0 -Wall -Wextra -o ex5_7_readv_by_read ex5_7_readv_by_read.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

static size_t iov_total(const struct iovec *iov, int cnt)
{
    size_t t = 0;

    for (int i = 0; i < cnt; i++)
        t += iov[i].iov_len;
    return t;
}

/* 用 read() + malloc 实现 readv()：读进连续缓冲，再分散到各段 */
static ssize_t my_readv(int fd, const struct iovec *iov, int cnt)
{
    size_t total = iov_total(iov, cnt);
    char *buf = malloc(total > 0 ? total : 1);
    ssize_t n;

    if (buf == NULL) {
        errno = ENOMEM;
        return -1;
    }
    n = read(fd, buf, total);
    if (n > 0) {
        ssize_t done = 0;

        for (int i = 0; i < cnt && done < n; i++) {
            size_t chunk = iov[i].iov_len;

            if ((size_t) (n - done) < chunk)
                chunk = (size_t) (n - done);        /* 短读：只拷剩下的 */
            memcpy(iov[i].iov_base, buf + done, chunk);
            done += (ssize_t) chunk;
        }
    }
    free(buf);
    return n;
}

/* 用 write() + malloc 实现 writev()：先聚集到连续缓冲，再一次写出 */
static ssize_t my_writev(int fd, const struct iovec *iov, int cnt)
{
    size_t total = iov_total(iov, cnt);
    char *buf = malloc(total > 0 ? total : 1);
    ssize_t n;
    size_t off = 0;

    if (buf == NULL) {
        errno = ENOMEM;
        return -1;
    }
    for (int i = 0; i < cnt; i++) {
        memcpy(buf + off, iov[i].iov_base, iov[i].iov_len);
        off += iov[i].iov_len;
    }
    n = write(fd, buf, total);
    free(buf);
    return n;
}

static char *slurp(const char *path, ssize_t *len)
{
    int fd = open(path, O_RDONLY);
    static char buf[256];
    ssize_t n;

    if (fd < 0)
        return NULL;
    n = read(fd, buf, sizeof buf - 1);
    close(fd);
    if (n < 0)
        n = 0;
    buf[n] = '\0';
    *len = n;
    return buf;
}

int main(void)
{
    const char *pa = "/tmp/c5_iov_real.bin";     /* 用真 writev 写 */
    const char *pb = "/tmp/c5_iov_mine.bin";     /* 用 my_writev 写 */
    const char *hdr = "REC1|";
    const char *tail = "|END";
    char body[8];

    memcpy(body, "abcdefgh", 8);

    printf("== 1. 真 writev 与手写 my_writev 的对照 ==\n");
    {
        struct iovec iov[3];
        int fa = open(pa, O_RDWR | O_CREAT | O_TRUNC, 0644);
        int fb = open(pb, O_RDWR | O_CREAT | O_TRUNC, 0644);
        ssize_t na, nb;

        iov[0].iov_base = (void *) hdr;  iov[0].iov_len = strlen(hdr);
        iov[1].iov_base = body;          iov[1].iov_len = sizeof body;
        iov[2].iov_base = (void *) tail; iov[2].iov_len = strlen(tail);

        na = writev(fa, iov, 3);
        nb = my_writev(fb, iov, 3);
        printf("    真 writev  -> %ld 字节\n", (long) na);
        printf("    my_writev  -> %ld 字节\n", (long) nb);
        close(fa);
        close(fb);

        {
            ssize_t la, lb;
            char *ca = slurp(pa, &la);
            char *cb = slurp(pb, &lb);

            printf("    两个文件内容：\"%s\" / \"%s\"\n", ca ? ca : "?", cb ? cb : "?");
            printf("    memcmp -> %s\n",
                   (la == lb && memcmp(ca, cb, (size_t) la) == 0) ? "完全相同" : "不同");
        }
    }

    printf("\n== 2. my_readv 把整条记录分散读回来 ==\n");
    {
        int fd = open(pa, O_RDONLY);
        char h[8] = { 0 }, t[8] = { 0 }, b[16] = { 0 };
        struct iovec iov[3];
        ssize_t n;

        iov[0].iov_base = h;  iov[0].iov_len = 5;
        iov[1].iov_base = b;  iov[1].iov_len = 8;
        iov[2].iov_base = t;  iov[2].iov_len = 4;
        n = my_readv(fd, iov, 3);
        printf("    my_readv -> %ld 字节（请求 %zu）\n",
               (long) n, iov_total(iov, 3));
        printf("    三段 = \"%s\" / \"%s\" / \"%s\"\n", h, b, t);
        close(fd);
    }

    printf("\n== 3. 短读时 my_readv 不能越界 ==\n");
    {
        int fd = open(pa, O_RDONLY);
        char b[64];
        struct iovec iov[3];
        ssize_t n;

        memset(b, 0x7f, sizeof b);               /* 先填成 0x7f，方便看有没有被写坏 */
        lseek(fd, 0, SEEK_END);
        printf("    文件总长 = %ld\n", (long) lseek(fd, 0, SEEK_CUR));
        lseek(fd, (off_t) (lseek(fd, 0, SEEK_CUR) - 3), SEEK_SET);   /* 只剩 3 字节 */

        iov[0].iov_base = b;      iov[0].iov_len = 5;    /* 第一段就要 5，最多只给 3 */
        iov[1].iov_base = b + 8;  iov[1].iov_len = 5;
        iov[2].iov_base = b + 16; iov[2].iov_len = 5;
        n = my_readv(fd, iov, 3);
        printf("    my_readv 请求 15 字节 -> 返回 %ld\n", (long) n);
        printf("    第一段前 3 字节 = \"%.3s\"，第二段首字节 = 0x%02x（没被碰）\n",
               b, (unsigned char) b[8]);
        close(fd);
    }

    printf("\n== 4. 代价：真 readv/writev 是 1 次系统调用，我们这版是 1 次 malloc + 1 次系统调用 ==\n");
    printf("    内核自己的实现不用中间缓冲——它直接把用户态的各段拼进 page 里，\n");
    printf("    所以段很多时，readv/writev 省的不只是系统调用，还有一次全量内存拷贝。\n");

    unlink(pa);
    unlink(pb);
    return 0;
}
