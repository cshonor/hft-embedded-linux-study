/* ex13_1_copy_bench.c — Ch13 习题 13-1：copy.c 的缓冲大小 / O_SYNC 计时
 *
 * 原书习题 13-1（p.250）：
 *   用 shell 的 `time` 内建给 Listing 4-1 的 `copy.c` 计时，然后
 *     (a) 用 `-DBUF_SIZE=nbytes` 换不同缓冲大小再测；
 *     (b) 给 `open()` 加上 `O_SYNC` 再测，看不同缓冲大小下差多少；
 *     (c) 在多种文件系统上重复，看趋势是否一致。
 *
 * ⚠️ 两处必须自己动手的改动：
 *   1) CE 没有 shell，用不了 `time` 内建 → 本程序**自己**用
 *      `clock_gettime(CLOCK_MONOTONIC)` 计时，并顺带把「系统调用次数」
 *      这个更本质的量也打印出来（它不随机器变）。
 *   2) `O_SYNC` 在小缓冲下会慢到不可接受 —— 原书 Table 13-3（写 1 MB）里
 *      `BUF_SIZE=1` 从 0.73 秒变成 1030 秒。所以本程序**故意跳过**
 *      「小缓冲 + O_SYNC」的组合，并在输出里说明为什么。这是工程判断，
 *      不是偷懒：**不要**在沙箱里跑一个可能要上千秒的实验。
 *
 * 也可以用 -DBUF_SIZE=n 只跑单一配置（与书上做法一致，便于对照）：
 *     gcc -O0 -Wall -Wextra -DBUF_SIZE=10 -o ex13_1_10 ex13_1_copy_bench.c
 *
 * 编译： gcc -O0 -Wall -Wextra -o ex13_1_copy_bench ex13_1_copy_bench.c
 * 取材： TLPI 习题 13-1；§13.1 正文（"using a larger buffer size ... means
 *         performing fewer system calls"）
 *       TLPI §4.2 的 Listing 4-1（copy.c 的 BUF_SIZE 用法）
 *       TLPI §13.3 + Table 13-3（O_SYNC 的开销随 BUF_SIZE 变化极大）
 *       man-pages 6.19 read(2) / write(2) / open(2)（O_SYNC）
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef BUF_SIZE
#define BUF_SIZE 4096           /* 与 Listing 4-1 的默认做法一致 */
#endif

#define SRC  "/app/ex13_1_src.bin"
#define DST  "/app/ex13_1_dst.bin"
#define TOTAL (1u << 20)        /* 源文件 1 MiB（CE 的 RLIMIT_FSIZE = 16 MiB） */

static double now_sec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

static int make_src(void)
{
    size_t done = 0;
    char *b = malloc(65536);
    int fd;

    if (b == NULL)
        return -1;
    memset(b, 'C', 65536);
    fd = open(SRC, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        free(b);
        return -1;
    }
    while (done < TOTAL) {
        size_t n = (TOTAL - done < 65536) ? (TOTAL - done) : 65536;
        if (write(fd, b, n) != (ssize_t) n) {
            close(fd);
            free(b);
            return -1;
        }
        done += n;
    }
    close(fd);
    free(b);
    return 0;
}

/* 一次完整的「读-写」复制；返回耗时秒数，*calls 回填系统调用总次数 */
static double run_copy(size_t bufsize, int use_sync, size_t *calls)
{
    char *buf = malloc(bufsize);
    double t0, t1;
    size_t ncalls = 0;
    int in, out;

    if (buf == NULL)
        return -1.0;

    in = open(SRC, O_RDONLY);
    if (in == -1) {
        free(buf);
        return -1.0;
    }
    out = open(DST, O_CREAT | O_WRONLY | O_TRUNC | (use_sync ? O_SYNC : 0), 0644);
    if (out == -1) {
        close(in);
        free(buf);
        return -1.0;
    }

    t0 = now_sec();
    for (;;) {
        ssize_t n = read(in, buf, bufsize);
        ncalls++;
        if (n < 0) {
            printf("  read 失败 errno=%d(%s)\n", errno, strerror(errno));
            break;
        }
        if (n == 0)
            break;
        if (write(out, buf, (size_t) n) != n) {
            printf("  write 失败 errno=%d(%s)\n", errno, strerror(errno));
            break;
        }
        ncalls++;
    }
    t1 = now_sec();

    close(in);
    close(out);
    free(buf);
    *calls = ncalls;
    return t1 - t0;
}

int main(void)
{
    printf("== 习题 13-1：copy.c 的缓冲大小 / O_SYNC 计时（自带计时） ==\n");
    printf("  源文件 %s，大小 %u 字节；目标是 %s\n", SRC, (unsigned) TOTAL, DST);
    printf("  计时用 clock_gettime(CLOCK_MONOTONIC) —— CE 里没有 shell 的 time 可用。\n");
    printf("  同时打印**系统调用次数**（read+write 之和）：它是算得出来的，不随机器变。\n\n");

    if (make_src() == -1) {
        printf("  造源文件失败 errno=%d(%s)\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    printf("  %-10s %-8s %-14s %-10s %s\n", "buf-size", "O_SYNC", "系统调用次数", "耗时(s)", "说明");
    {
        const size_t sizes[] = {10, 512, 4096, 65536};
        for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
            size_t calls = 0;
            double dt = run_copy(sizes[i], 0, &calls);
            printf("  %-10zu %-8s %-14zu %-10.4f %s\n",
                   sizes[i], "否", calls, dt,
                   (sizes[i] == BUF_SIZE) ? "← 与 BUF_SIZE 相同" : "");
        }
        /* 只在大缓冲上试 O_SYNC：小缓冲 + O_SYNC 的耗时是**分钟到千秒级** */
        {
            const size_t sizes2[] = {4096, 65536};
            for (size_t i = 0; i < sizeof(sizes2) / sizeof(sizes2[0]); i++) {
                size_t calls = 0;
                double dt = run_copy(sizes2[i], 1, &calls);
                printf("  %-10zu %-8s %-14zu %-10.4f %s\n",
                       sizes2[i], "是", calls, dt, "← 每次 write 都等到设备确认");
            }
        }
    }

    printf("\n== 怎么读这张表 ==\n");
    printf("  (a) 只看「系统调用次数」那一列（它与机器无关）：\n");
    printf("      buf-size 10 -> 512 -> 4096 -> 65536，次数按**反比**下降。\n");
    printf("      1 MiB 用 10 字节缓冲要 20 多万次调用，用 64 KiB 只要 32 次 ——\n");
    printf("      这就是 §13.1 说的「减少系统调用」的全部含义，耗时差异是它的**结果**，\n");
    printf("      不是独立的一条结论。\n");
    printf("  (b) O_SYNC 那两行：同样的 buf-size，耗时**涨一大截**。\n");
    printf("      因为每次 write() 都要等设备确认，而小缓冲意味着 write 次数多，\n");
    printf("      于是「同步的代价 x 次数」被乘起来 —— 这是 O_SYNC 最主要的陷阱。\n");
    printf("  (c) 书上 Table 13-3（写 1 MB）的数量级：BUF_SIZE=1 时不带 O_SYNC 约 0.7 秒，\n");
    printf("      带 O_SYNC 约 1030 秒；BUF_SIZE=4096 时约 0.01 秒 vs 0.34 秒。\n");
    printf("      → 所以本程序**故意没跑**「小缓冲 + O_SYNC」：\n");
    printf("        那不是「测不出来」，是「测出来要上千秒」，不该在沙箱里做。\n");
    printf("  ⚠️ 耗时列是 CE 宿主机上的值，只能做**同一次运行内**的横向比较；\n");
    printf("     换次运行、换台宿主机都会变。要可移植的结论请用「系统调用次数」。\n");
    printf("  ⚠️ 习题 (c) 要你在多种文件系统上重复。本容器里 /app 是 ext4、/tmp 是 tmpfs\n");
    printf("     （见 13.6 的实测），但对 /tmp 只能写 16 MiB 以下，且 tmpfs 无真实设备，\n");
    printf("     O_SYNC 的语义在这里已经失真 —— 所以那一步得在真机上做，本笔记不代劳。\n");

    unlink(SRC);
    unlink(DST);
    return EXIT_SUCCESS;
}
