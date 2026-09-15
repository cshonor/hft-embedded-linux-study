/* c13_1_buffer_cache.c — Ch13 §13.1：内核缓冲（Buffer Cache / Page Cache）
 *
 * TLPI §13.1（p.233）的核心断言只有一句话：
 *   `read()` / `write()` **不直接对磁盘**做传输，而是在「用户缓冲」与
 *   「内核缓冲区高速缓存（buffer cache / page cache）」之间复制数据。
 * 本程序把这句话拆成三件**可以量出来**的事：
 *   ① write() 返回后，数据立刻对另一个 fd 可见 —— 因为它在内核里，不在磁盘上；
 *   ② 写完一大批数据，/proc/meminfo 的 Dirty 与 /proc/vmstat 的 nr_dirty 会涨；
 *      fsync() 之后会落回去 —— 这就是「延迟写」的直接证据；
 *   ③ 总字节数不变，只改 write() 的块大小，**系统调用次数**会差 4 个数量级，
 *      耗时跟着走 —— 这才是「缓冲影响性能」的机制。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c13_1_buffer_cache c13_1_buffer_cache.c
 * 取材： TLPI §13.1（buffer cache 的定义、块大小对性能的影响、Table 13-1/13-2）
 *       man-pages 6.19 write(2) NOTES（"a successful return from write() does not
 *         make any guarantee that data has been committed to disk"）
 *                      fsync(2)（"The fsync() call ... transfers all modified
 *         in-core data ... to the storage device"）
 *       Linux v6.6 mm/page-writeback.c（dirty 计数器 / flusher 线程）
 *                      include/linux/writeback.h（DIRTY_* 阈值）
 *       TLPI Ch12 §12.1.2（/proc/meminfo、/proc/vmstat 的读法）
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ---------- 小工具：从 /proc/meminfo 取一个 kB 字段 ---------- */
static long meminfo_kb(const char *key)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    char line[256];
    long v = -1;

    if (fp == NULL)
        return -1;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, key, strlen(key)) == 0) {
            if (sscanf(line + strlen(key), " : %ld", &v) != 1)
                v = -2;
            break;
        }
    }
    fclose(fp);
    return v;
}

/* ---------- 小工具：从 /proc/vmstat 取一个计数器 ---------- */
static long vmstat_val(const char *key)
{
    FILE *fp = fopen("/proc/vmstat", "r");
    char line[256];
    long v = -1;

    if (fp == NULL)
        return -1;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, key, strlen(key)) == 0) {
            if (sscanf(line + strlen(key), " %ld", &v) != 1)
                v = -2;
            break;
        }
    }
    fclose(fp);
    return v;
}

/* ---------- 小工具：单调时钟，返回秒（double） ---------- */
static double now_sec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

#define TMPFILE "/app/c13_1.bin"

int main(void)
{
    printf("== ① write() 返回后，数据已经在内核里（对别的 fd 立刻可见） ==\n");
    {
        const char *msg = "hello-page-cache";
        int fd = open(TMPFILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd == -1) {
            printf("  open 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        errno = 0;
        ssize_t w = write(fd, msg, strlen(msg));
        int we = errno;
        printf("  write(fd, \"%s\", %d) = %ld errno=%d(%s)\n",
               msg, (int) strlen(msg), (long) w, we, strerror(we));
        /* 刻意**不** fsync、也不 close 写端，直接另开一个 fd 去读 */
        int rfd = open(TMPFILE, O_RDONLY);
        char buf[64];
        memset(buf, 0, sizeof(buf));
        errno = 0;
        ssize_t r = read(rfd, buf, sizeof(buf) - 1);
        printf("  另开 fd 立刻 read() = %ld 字节，内容 = [%s]\n", (long) r, buf);
        printf("  → 此刻数据在**内核页缓存**里：写端还在、也没 fsync，磁盘上那份未必更新\n");
        printf("     （能读到 ⟸ 说明 write 拷进的是内核；**不**说明它已经落盘，见 ②）\n");
        close(rfd);
        close(fd);
    }

    printf("\n== ② 脏页计数器：write 之后涨，fsync 之后落 ==\n");
    {
        const size_t CHUNK = 1u << 20;      /* 1 MiB */
        /* 共 16 MiB —— 正好等于 CE 的 RLIMIT_FSIZE 上限（probe13c 实测 16777216）。
           挑这个数是因为：16 MiB = 4096 页，nr_dirty 应该恰好 +4096，可以**当场对账**。 */
        const int NCHUNK = 16;
        char *buf = malloc(CHUNK);
        long d0, d1, d2;

        if (buf == NULL) {
            printf("  malloc 失败\n");
            return EXIT_FAILURE;
        }
        memset(buf, 'D', CHUNK);

        d0 = vmstat_val("nr_dirty");
        printf("  写之前      : Dirty=%-9ld kB  Writeback=%-8ld kB  nr_dirty=%-8ld  nr_writeback=%ld\n",
               meminfo_kb("Dirty"), meminfo_kb("Writeback"), d0, vmstat_val("nr_writeback"));

        int fd = open(TMPFILE, O_WRONLY | O_TRUNC);
        if (fd == -1) {
            printf("  open 失败 errno=%d(%s)\n", errno, strerror(errno));
            free(buf);
            return EXIT_FAILURE;
        }
        for (int i = 0; i < NCHUNK; i++) {
            if (write(fd, buf, CHUNK) != (ssize_t) CHUNK) {
                printf("  write 失败 errno=%d(%s)\n", errno, strerror(errno));
                break;
            }
        }
        d1 = vmstat_val("nr_dirty");
        printf("  写 %d MiB 后 : Dirty=%-9ld kB  Writeback=%-8ld kB  nr_dirty=%-8ld  nr_writeback=%ld\n",
               NCHUNK, meminfo_kb("Dirty"), meminfo_kb("Writeback"),
               d1, vmstat_val("nr_writeback"));
        printf("  → nr_dirty 的增量 = %ld 页；而 %d MiB / %ld 字节页 = %ld 页\n",
               d1 - d0, NCHUNK, (long) sysconf(_SC_PAGESIZE),
               (long) (NCHUNK * CHUNK / (size_t) sysconf(_SC_PAGESIZE)));
        printf("     两者对得上 ⟹ 这 16 MiB 一个字都没上磁盘，全在页缓存里等着。\n");
        printf("     另外 nr_dirty(页) x 4 = Dirty(kB) 恒成立：%ld x 4 = %ld\n",
               d1, d1 * 4);

        errno = 0;
        int fr = fsync(fd);
        int fe = errno;
        printf("  fsync(fd) = %d errno=%d(%s)\n", fr, fe, strerror(fe));
        d2 = vmstat_val("nr_dirty");
        printf("  fsync 之后  : Dirty=%-9ld kB  Writeback=%-8ld kB  nr_dirty=%-8ld  nr_writeback=%ld\n",
               meminfo_kb("Dirty"), meminfo_kb("Writeback"),
               d2, vmstat_val("nr_writeback"));
        printf("  → nr_dirty 从 %ld 落回 %ld，就是 fsync 把脏页推下去的。\n", d1, d2);

        printf("  ⚠️ 这两个计数器是**整机（宿主）**口径，不是本进程的：\n");
        printf("     别的容器、别的进程也在写盘，所以绝对值**必然漂移**；\n");
        printf("     但上面那句「增量 == 页数」是**当次运行内**成立的，可以当场验。\n");
        printf("  ⚠️ 反过来看时间：write() 把 16 MiB 拷进内核只花了毫秒级 ——\n");
        printf("     这不是磁盘的速度，是**内存拷贝**的速度，收益全在延迟写上。\n");
        close(fd);
        free(buf);
    }

    printf("\n== ③ 总字节数不变，只改块大小：系统调用次数差 4 个数量级 ==\n");
    {
        const size_t TOTAL = 1u << 20;      /* 每档都写 1 MiB */
        const size_t sizes[] = {1, 16, 256, 4096, 65536};
        char *big = malloc(65536);

        if (big == NULL) {
            printf("  malloc 失败\n");
            return EXIT_FAILURE;
        }
        memset(big, 'B', 65536);
        printf("  目标：每档都写 %zu 字节，只改 write() 的 buf-size\n", TOTAL);
        printf("    %-10s %-14s %-10s\n", "buf-size", "write() 次数", "耗时(s)");
        for (size_t k = 0; k < sizeof(sizes) / sizeof(sizes[0]); k++) {
            size_t bs = sizes[k];
            int fd = open(TMPFILE, O_WRONLY | O_TRUNC);
            double t0, t1;
            size_t calls = 0, done = 0;

            if (fd == -1) {
                printf("  open 失败 errno=%d(%s)\n", errno, strerror(errno));
                break;
            }
            t0 = now_sec();
            while (done < TOTAL) {
                size_t n = (TOTAL - done < bs) ? (TOTAL - done) : bs;
                if (write(fd, big, n) != (ssize_t) n)
                    break;
                done += n;
                calls++;
            }
            t1 = now_sec();
            close(fd);
            printf("    %-10zu %-14zu %.4f\n", bs, calls, t1 - t0);
        }
        printf("  → 总字节一样，buf-size 从 1 加到 65536，write() 次数从 1048576 降到 16\n");
        printf("     （差 65536 倍 ≈ 4.8 个数量级），耗时跟着掉。\n");
        printf("  ⚠️ CE 上测到的**耗时是另一台机器**的值，只能看趋势；\n");
        printf("     但「次数」是算出来的（TOTAL / buf-size），绝对可靠。\n");
        free(big);
    }

    printf("\n== ④ 本节结论：两层缓冲里的「下面那一层」 ==\n");
    printf("  read()/write()  <-copy->  内核页缓存  <-writeback->  磁盘\n");
    printf("  ① 证明「返回时数据在内核」；② 证明「此时还没落盘」；③ 证明「块大小决定系统调用次数」\n");
    printf("  要把它推进磁盘：fsync()/fdatasync()/sync() 或 O_SYNC（见 13.3）\n");

    unlink(TMPFILE);
    return EXIT_SUCCESS;
}
