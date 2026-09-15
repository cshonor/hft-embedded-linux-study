/* c5_7_readv_writev.c — §5.7 分散/聚集 I/O：readv() / writev()
 *
 * 把 N 段内存一次交给内核：writev 是"聚集"（gather，多段 → 一次写），
 * readv 是"分散"（scatter，一次读填多段）。好处是**省系统调用**：
 * 写「头 + 体 + 尾」本来要 3 次 write，现在 1 次。
 *
 * 演示五件事：
 *   1) 一次 writev 写三段（头部 / 载荷 / 尾部）
 *   2) 一次 readv 把同样的三段读回来，逐字节比对
 *   3) iov 数组本身也能加下标（iovcnt 可以小于数组长度）
 *   4) IOV_MAX 上限是多少
 *   5) 管道上的部分写：readv/writev 一样会"写多少算多少"，返回值才是真相
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_7_readv_writev c5_7_readv_writev.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

#define BODY_LEN 100

int main(void)
{
    const char *p = "/tmp/c5_iov.bin";
    const char *hdr = "HDR:";
    const char *tail = ":END";
    char body_w[BODY_LEN], body_r[BODY_LEN];
    ssize_t n;

    for (int i = 0; i < BODY_LEN; i++)
        body_w[i] = (char) ('a' + i % 26);

    printf("== 1. 一次 writev() 写三段 ==\n");
    {
        int fd = open(p, O_RDWR | O_CREAT | O_TRUNC, 0644);
        struct iovec iov[3];
        struct stat st;
        ssize_t want;

        iov[0].iov_base = (void *) hdr;   iov[0].iov_len = strlen(hdr);
        iov[1].iov_base = body_w;         iov[1].iov_len = BODY_LEN;
        iov[2].iov_base = (void *) tail;  iov[2].iov_len = strlen(tail);
        want = (ssize_t) (iov[0].iov_len + iov[1].iov_len + iov[2].iov_len);

        n = writev(fd, iov, 3);
        printf("   writev(fd, iov, 3) -> %ld（三段合计 %ld 字节，一次系统调用搞定）\n",
               (long) n, (long) want);
        fstat(fd, &st);
        printf("   文件 st_size=%lld\n", (long long) st.st_size);

        printf("\n== 2. 一次 readv() 读回来，逐字节比对 ==\n");
        lseek(fd, 0, SEEK_SET);
        {
            char h[8], t[8];
            struct iovec riov[3];

            memset(h, 0, sizeof h);
            memset(t, 0, sizeof t);
            memset(body_r, 0, sizeof body_r);
            riov[0].iov_base = h;      riov[0].iov_len = strlen(hdr);
            riov[1].iov_base = body_r; riov[1].iov_len = BODY_LEN;
            riov[2].iov_base = t;      riov[2].iov_len = strlen(tail);
            n = readv(fd, riov, 3);
            printf("   readv(fd, riov, 3) -> %ld\n", (long) n);
            printf("   三段分别是 \"%s\" / %d 字节 / \"%s\"\n", h, BODY_LEN, t);
            printf("   memcmp 载荷 -> %s\n",
                   memcmp(body_w, body_r, BODY_LEN) == 0 ? "相同" : "不同");
        }

        printf("\n== 3. iovcnt 可以小于数组长度（iov 数组就是个普通数组）==\n");
        {
            struct iovec two[3];
            char b[16];

            memset(b, 0, sizeof b);
            two[0].iov_base = b;      two[0].iov_len = 7;
            two[1].iov_base = body_r; two[1].iov_len = 4;
            two[2].iov_base = b + 7;  two[2].iov_len = 4;   /* 这个不会被用到 */
            lseek(fd, 0, SEEK_SET);
            n = readv(fd, two, 2);                          /* 只取前 2 段 = 11 字节 */
            b[11] = '\0';
            printf("   readv(fd, two, 2) -> %ld，读到 \"%s\"\n", (long) n, b);
        }
        close(fd);
    }

    printf("\n== 4. IOV_MAX：一次最多几段 ==\n");
    printf("   sysconf(_SC_IOV_MAX) = %ld\n", sysconf(_SC_IOV_MAX));

    printf("\n== 5. 管道上的部分写：writev 一样会「写多少算多少」==\n");
    {
        int pp[2];

        if (pipe(pp) == 0) {
            char chunk[1024];
            struct iovec iov[2];
            ssize_t w;

            memset(chunk, 'f', sizeof chunk);
            fcntl(pp[1], F_SETFL, O_NONBLOCK);
            /* 把管道容量压到一页（4096），再塞 3000 字节，留出 1096 的空隙 ——
               这样 writev 请求 4096 既不会阻塞、也不会全写进去，正好看到部分写 */
            if (fcntl(pp[1], F_SETPIPE_SZ, 4096) == -1)
                printf("   F_SETPIPE_SZ(4096) 失败: %s（下面结果会不同）\n", strerror(errno));
            printf("   管道容量 F_GETPIPE_SZ = %d\n", fcntl(pp[1], F_GETPIPE_SZ));
            w = write(pp[1], chunk, 1024);
            w += write(pp[1], chunk, 1024);
            w += write(pp[1], chunk, 952);
            printf("   已先塞入 %ld 字节\n", (long) w);

            iov[0].iov_base = chunk; iov[0].iov_len = 2048;
            iov[1].iov_base = chunk; iov[1].iov_len = 2048;
            errno = 0;
            w = writev(pp[1], iov, 2);
            if (w >= 0)
                printf("   writev 请求 4096 字节 -> 返回 %ld（部分写！剩下的自己补）\n",
                       (long) w);
            else
                printf("   writev 请求 4096 字节 -> -1 errno=%d (%s)（一点空间都没有了）\n",
                       errno, strerror(errno));
            close(pp[0]);
            close(pp[1]);
        }
    }

    return 0;
}
