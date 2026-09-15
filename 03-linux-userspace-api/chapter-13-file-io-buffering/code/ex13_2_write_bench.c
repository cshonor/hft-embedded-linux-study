/* ex13_2_write_bench.c — Ch13 习题 13-2：给 write_bytes.c 加计时
 *
 * 原书习题 13-2（p.250）：
 *   「给本书源码分发包里的 `filebuff/write_bytes.c` 计时，
 *     换不同的缓冲大小与不同的文件系统。」
 *
 * 麻烦在于：**书上的 `write_bytes.c` 自己不打印任何耗时** ——
 * 它只负责写，计时要用 shell 的 `time` 内建（`time ./write_bytes ofile 100000000 4096`）。
 * CE 里既没有 shell 也没有 `time`，所以本程序做两件事：
 *   ① 用与 `write_bytes.c` **完全相同**的写入循环，自己在程序内计时；
 *   ② 把书上那三个编译期开关（`-DUSE_O_SYNC` / `-DUSE_FSYNC` / `-DUSE_FDATASYNC`）
 *      的效果摆在一张表里对比。
 * 想验证「真的原书程序」，本目录下就是逐字镜像的 `write_bytes.c`，
 * `run_ch13.py` 里也有带各开关的作业（见 code/README.md）。
 *
 * ⚠️ 规模是**故意按模式缩的**，不是笔误：
 *    - 普通 / O_SYNC：1 MiB
 *    - fsync / fdatasync（每次 write 后都调）：256 KiB
 *   因为后两种会让同步次数 = write 次数，1 MiB + 512 字节缓冲 = 2048 次同步，
 *   在沙箱里可能跑几十秒。**按模式定规模**是这类 benchmark 的正确做法。
 *
 * 编译： gcc -O0 -Wall -Wextra -o ex13_2_write_bench ex13_2_write_bench.c
 * 取材： TLPI 习题 13-2；§13.3 + Table 13-3（O_SYNC / fsync / fdatasync 的开销）
 *       man-pages 6.19 write(2) / fsync(2) / open(2)（O_SYNC）
 *       Linux v6.6 fs/sync.c:218-226（fsync / fdatasync 的 syscall 入口）
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define OUT "/app/ex13_2.bin"

/* 四种模式，对应书里 write_bytes.c 的四个编译分支 */
enum { M_PLAIN = 0, M_OSYNC, M_FSYNC, M_FDATASYNC };

static double now_sec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

/* 与 write_bytes.c 一模一样的循环，只是外面套了计时 */
static double bench(int mode, size_t numBytes, size_t bufSize, size_t *writes)
{
    char *buf = malloc(bufSize);
    int flags = O_CREAT | O_WRONLY | O_TRUNC | (mode == M_OSYNC ? O_SYNC : 0);
    double t0, t1;
    size_t thisWrite, totWritten;
    int fd;
    int ok = 1;

    if (buf == NULL)
        return -1.0;
    fd = open(OUT, flags, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        free(buf);
        return -1.0;
    }
    *writes = 0;
    t0 = now_sec();
    for (totWritten = 0; totWritten < numBytes; totWritten += thisWrite) {
        thisWrite = (bufSize < numBytes - totWritten) ? bufSize : (numBytes - totWritten);
        if (write(fd, buf, thisWrite) != (ssize_t) thisWrite) {
            printf("  write 失败 errno=%d(%s)\n", errno, strerror(errno));
            ok = 0;
            break;
        }
        (*writes)++;
        if (mode == M_FSYNC && fsync(fd) != 0) {
            printf("  fsync 失败 errno=%d(%s)\n", errno, strerror(errno));
            ok = 0;
            break;
        }
        if (mode == M_FDATASYNC && fdatasync(fd) != 0) {
            printf("  fdatasync 失败 errno=%d(%s)\n", errno, strerror(errno));
            ok = 0;
            break;
        }
    }
    t1 = now_sec();
    close(fd);
    free(buf);
    return ok ? (t1 - t0) : -1.0;
}

int main(void)
{
    printf("== 习题 13-2：写入型 benchmark（write_bytes.c 自带计时的版本） ==\n");
    printf("  输出文件 %s。计时用 CLOCK_MONOTONIC；同时给出 write() 次数与\n", OUT);
    printf("  「每次写平均耗时」——最后这一列最能说明同步的代价。\n\n");

    printf("  %-14s %-9s %-8s %-12s %-11s %s\n",
           "模式", "总字节", "buf-size", "write() 次数", "耗时(s)", "每次 write 平均(us)");
    {
        struct {
            int mode;
            const char *name;
            size_t total;
        } modes[] = {
            {M_PLAIN,     "普通",          1u << 20},
            {M_OSYNC,     "O_SYNC",        1u << 20},
            {M_FSYNC,     "每次 fsync",     1u << 18},
            {M_FDATASYNC, "每次 fdatasync", 1u << 18},
        };
        const size_t bufs[] = {512, 4096, 65536};

        for (size_t m = 0; m < sizeof(modes) / sizeof(modes[0]); m++) {
            for (size_t b = 0; b < sizeof(bufs) / sizeof(bufs[0]); b++) {
                size_t writes = 0;
                double dt = bench(modes[m].mode, modes[m].total, bufs[b], &writes);
                if (dt < 0) {
                    printf("  %-14s 失败\n", modes[m].name);
                    continue;
                }
                printf("  %-14s %-9zu %-8zu %-12zu %-11.4f %.2f\n",
                       modes[m].name, modes[m].total, bufs[b], writes, dt,
                       writes ? dt * 1e6 / (double) writes : 0.0);
            }
        }
    }

    printf("\n== 怎么读这张表 ==\n");
    printf("  1) 「write() 次数」只由 总字节/buf-size 决定，**与机器无关**。\n");
    printf("     先看这一列，再看耗时 —— 否则很容易把「调用次数多」误当成「磁盘慢」。\n");
    printf("  2) 最后一列「每次 write 平均」把「次数」这个混杂因素除掉了，\n");
    printf("     它才真正反映**单次同步**的代价：\n");
    printf("       普通模式     每次调用只是内存拷贝（亚微秒级）\n");
    printf("       O_SYNC       每次调用 = 一次设备往返（毫秒级，差 3~4 个数量级）\n");
    printf("       每次 fsync   同理，但它只在**循环里额外调**，O_SYNC 是内建在 write 上\n");
    printf("  3) 书上 Table 13-3 的结论与此一致：**别为了保险全局挂 O_SYNC**。\n");
    printf("     要么把缓冲加大（同样的同步次数摊到更多字节上），要么只在关键点显式\n");
    printf("     `fsync()` / `fdatasync()`。\n");
    printf("  4) `fdatasync` 通常比 `fsync` 便宜，但**不是**在所有文件系统上都便宜；\n");
    printf("     本容器 /app 是 ext4（data=ordered 默认），两者差别可能很小。\n");
    printf("     想知道差多少只能**在目标文件系统上测**（这正是习题 13-2 的后半问）。\n");
    printf("\n  ⚠️ 耗时的绝对值是 CE 宿主机的，只能做**同一次运行内**的横向比较。\n");
    printf("  ⚠️ 为什么 4 种模式的总字节数不一样：见文件头注释 —— 同步模式按规模缩，\n");
    printf("     否则「512 字节缓冲 + 每次 fsync」要跑几千次设备往返。\n");

    unlink(OUT);
    return EXIT_SUCCESS;
}
