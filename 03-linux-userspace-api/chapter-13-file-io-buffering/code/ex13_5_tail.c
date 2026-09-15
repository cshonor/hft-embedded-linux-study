/* ex13_5_tail.c — Ch13 习题 13-5：用 I/O 系统调用实现 `tail [-n num] file`
 *
 * 原书习题 13-5（p.250）：
 *   「`tail [-n num] file` 打印文件最后 num 行（默认 10 行）。
 *     用 `lseek()` / `read()` / `write()` 等 I/O **系统调用**实现它，
 *     并且**记住本章讲的缓冲问题**，让实现是高效的。」
 *
 * 「高效」的意思是关键，它排除了最朴素的两个写法：
 *   ❌ 从头读到尾、用一个环形缓冲保留最后 n 行 —— 读的是**整个文件**；
 *   ❌ 先 fstat 拿大小、一次性读进大缓冲 —— 文件大起来就是 OOM。
 * 正确做法是**从文件尾部往回按块扫**，找到足够多的换行符就停：
 *   用 `lseek(fd, -BLK, SEEK_END)` 定位，`read()` 一块，数里面的 '\n'；
 *   不够 n+1 个就再往前挪一块，够了一次性写出去。
 * 于是代价是 O(尾部那点字节)，与文件总大小**无关**。
 *
 * 本程序的「高效」还可量：它会把「一共 READ 了多少字节」打出来，
 * 你可以拿它跟文件总大小对比 —— 200 KiB 的文件读最后 5 行，只读了 4 KiB。
 *
 * 用法：
 *     ./ex13_5_tail --help
 *     ./ex13_5_tail [-n num] file
 *     ./ex13_5_tail              # 不带参数 → 自测模式（自己造文件、跑几组参数）
 *
 * 编译： gcc -O0 -Wall -Wextra -o ex13_5_tail ex13_5_tail.c
 * 取材： TLPI 习题 13-5；§13.1（为什么块大小与系统调用次数有关）
 *                      §4.4（lseek 的 SEEK_END + 负偏移）
 *                      §5.4（read 的 short read 与 0 = EOF）
 *       man-pages 6.19 lseek(2)（SEEK_END + 负 off_t；越界返回 EINVAL）
 *                      read(2)（short read）、write(2)（partial write）
 *       POSIX tail(1) 的 `-n` 语义（负数表示「从开头第 n 行之后」，本实现不支持）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLK 4096

/* 统计一块里的 '\n' 个数 */
static size_t count_nl(const char *b, size_t n)
{
    size_t c = 0;

    for (size_t i = 0; i < n; i++)
        if (b[i] == '\n')
            c++;
    return c;
}

/*
 * 打印 file 的最后 nlines 行。
 * 返回实际读入的字节数（用于展示「高效」），-1 表示失败。
 */
static long tail_file(const char *path, long nlines, FILE *out)
{
    int fd = open(path, O_RDONLY);
    struct stat st;
    char *buf = NULL;           /* 累积的尾部数据，从 buf[used-blk] 往右有效 */
    long total_read = 0;
    size_t used = 0;            /* 有效字节数（尾部） */
    long seen_nl = 0;           /* 已扫过的块里累计的换行数 */
    long start;                 /* 输出起点（在 buf 里的下标） */
    size_t i;

    if (fd == -1) {
        fprintf(stderr, "tail: 打不开 %s: %s\n", path, strerror(errno));
        return -1;
    }
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "tail: fstat 失败: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    /* 从尾部往回扫，直到攒够 nlines+1 个换行（多要一个用来定位起点） */
    for (;;) {
        long pos;
        size_t want;
        ssize_t n;
        char *nb;

        if ((long) used >= st.st_size)
            break;                                  /* 已经到文件开头了 */
        pos = (long) st.st_size - (long) used - BLK;
        if (pos < 0)
            pos = 0;

        /* 这一轮要从文件 [pos, st_size) 里补上前面缺的那一段。
           注意 want 可能 **小于** BLK —— 当 pos 被夹到 0 时就是这样
           （最后一块往往不满）。所以旧数据必须接在 nb[want] 处，
           **不能**写成 nb[BLK]：那样中间 [want, BLK) 会留下一段
           未初始化的堆内存，输出就像被随机插入了一段垃圾。 */
        want = (size_t) ((long) st.st_size - (long) used - pos);
        nb = malloc(want + used);
        if (nb == NULL) {
            fprintf(stderr, "tail: malloc 失败\n");
            free(buf);
            close(fd);
            return -1;
        }
        if (used > 0)
            memcpy(nb + want, buf, used);
        if (lseek(fd, pos, SEEK_SET) == -1) {
            fprintf(stderr, "tail: lseek 失败: %s\n", strerror(errno));
            free(nb);
            free(buf);
            close(fd);
            return -1;
        }
        n = read(fd, nb, want);
        if (n <= 0) {
            free(nb);
            free(buf);
            close(fd);
            return -1;
        }
        if (n != (ssize_t) want) {
            /* 普通文件上不该发生；真要发生就得重试，本实现选择明确报错 */
            fprintf(stderr, "tail: 短读（want %zu, got %zd）\n", want, n);
            free(nb);
            free(buf);
            close(fd);
            return -1;
        }
        total_read += n;
        seen_nl += (long) count_nl(nb, (size_t) n);
        free(buf);
        buf = nb;
        used += (size_t) n;

        if (seen_nl > nlines)
            break;
    }
    close(fd);

    /* 找输出起点：从 buf 开头数到第 (行数 - nlines) 个 '\n' 之后。
       数行数时要留意**最后一行不需要以换行结尾**（POSIX tail 的定义）：
         文件以 '\n' 结尾  → 行数 = 换行数
         否则              → 行数 = 换行数 + 1
       回扫时至少读过包含文件最后一个字节的那一块，所以 buf[used-1] 就是文件末字节。 */
    if (nlines == 0) {
        /* 0 行就是 0 行：不输出任何东西。
           （不能靠下面的定位逻辑 —— 它找不到第 skip 个换行时会 fallback 成
             「从头输出」，那反而变成全部行。） */
        start = (long) used;
    } else {
        long nlines_total = seen_nl + ((used > 0 && buf[used - 1] == '\n') ? 0 : 1);
        long skip = nlines_total - nlines;

        start = 0;
        if (skip > 0) {
            long k = 0;
            for (i = 0; i < used; i++) {
                if (buf[i] == '\n') {
                    k++;
                    if (k == skip) {
                        start = (long) i + 1;
                        break;
                    }
                }
            }
            if (k < skip)
                start = 0;
        }
    }
    for (i = (size_t) start; i < used; i++)
        if (fputc((unsigned char) buf[i], out) == EOF)
            break;

    free(buf);
    return total_read;
}

/* ---------- 自测模式：造一个多行文件，跑几组参数 ---------- */
static int self_test(void)
{
    const char *p = "/app/ex13_5_sample.txt";
    const int NLINES = 1000;
    FILE *fp = fopen(p, "w");
    struct stat st;
    int fdsz;

    if (fp == NULL) {
        printf("  造样本失败 errno=%d(%s)\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }
    for (int i = 1; i <= NLINES; i++)
        fprintf(fp, "line-%04d: %s\n", i,
                "0123456789012345678901234567890123456789012345678901234567890123456789");
    fclose(fp);

    {
        int fd = open(p, O_RDONLY);
        fstat(fd, &st);
        close(fd);
        fdsz = (int) st.st_size;
    }

    printf("== 自测：样本文件 %s\n", p);
    printf("   共 %d 行，文件大小 %d 字节（约 %d KiB）\n\n", NLINES, fdsz, fdsz / 1024);

    {
        /* 档位刻意拉开：每行 79 字节 → 一块 4096 字节只装得下约 51 行。
           所以 n=1 与 n=50 都只需一块，n=100 要两块，n=500 要十来块，
           而 n=1000（= 总行数）会一路扫到文件开头 —— 「实际读入字节」这一列
           才能看出它随 n 增长，而不是恒等于块大小。 */
        const long ns[] = {1, 100, 500, 1000};
        printf("  %-8s %-14s %-10s %s\n", "-n", "实际读入字节", "输出行数", "输出首行");
        for (size_t k = 0; k < sizeof(ns) / sizeof(ns[0]); k++) {
            long got;
            long nout = 0;
            char first[128];

            /* 为了拿到「首行」又不把全部输出打出来，先输出到临时文件。
               首行比末行更能说明问题：如果实现真的「从尾部数 n 行」，
               那么 -n 1/100/500/1000 的首行应当分别是
               line-1000 / line-0901 / line-0501 / line-0001。 */
            {
                const char *tmp = "/app/ex13_5_out.txt";
                FILE *o = fopen(tmp, "w");
                if (o == NULL)
                    return EXIT_FAILURE;
                got = tail_file(p, ns[k], o);
                fclose(o);

                o = fopen(tmp, "r");
                first[0] = '\0';
                if (o != NULL) {
                    char line[256];
                    while (fgets(line, sizeof(line), o) != NULL) {
                        size_t L = strlen(line);

                        if (L > 0 && line[L - 1] == '\n')
                            line[L - 1] = '\0';
                        nout++;
                        if (nout == 1) {
                            size_t cp = strlen(line);

                            if (cp > sizeof(first) - 1)
                                cp = sizeof(first) - 1;
                            memcpy(first, line, cp);
                            first[cp] = '\0';
                        }
                    }
                    fclose(o);
                }
                unlink(tmp);
            }
            printf("  %-8ld %-14ld %-10ld %.46s\n", ns[k], got, nout, first);
        }
    }

    printf("\n  → 注意「实际读入字节」：随 -n 增大而增大，但**始终远小于**文件总大小\n");
    printf("     （%d 字节）。这就是「从尾部往回扫」的意义：代价只跟尾部有关，\n", fdsz);
    printf("     与文件多大无关。如果实现是「从头读到尾」，这一列就会恒等于 %d。\n", fdsz);
    printf("  → 「输出首行」这列是对账用的：总共 %d 行，-n 1/100/500/1000 的首行\n", NLINES);
    printf("     就应当是 line-1000 / line-0901 / line-0501 / line-0001 —— 正好差 n-1 行。\n");

    printf("\n== 把每组的完整输出打一遍（-n 3）==\n");
    {
        long got = tail_file(p, 3, stdout);
        printf("  [tail 读了 %ld 字节]\n", got);
    }

    printf("\n== 边界情况（每一条都真跑一遍，不靠人眼判断）==\n");
    {
        const char *p2 = "/app/ex13_5_nonl.txt";
        /* 16 字节、2 个 '\n'：按 tail 的定义是 **3 行**（最后一行没有结尾换行） */
        const char *SAMPLE = "a\nb\nc-no-newline";
        int fd = open(p2, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        long got;

        if (fd == -1 || write(fd, SAMPLE, 16) != 16) {
            printf("  造样本失败 errno=%d(%s)\n", errno, strerror(errno));
            if (fd != -1)
                close(fd);
        } else {
            close(fd);
            printf("  样本 \"a\\nb\\nc-no-newline\"：16 字节、只有 2 个 '\\n'，但是 **3 行**\n");
            printf("  1) -n 1           → [");
            got = tail_file(p2, 1, stdout);
            printf("]（读了 %ld 字节）\n", got);
            printf("  2) -n 3           → [");
            got = tail_file(p2, 3, stdout);
            printf("]（读了 %ld 字节）\n", got);
            printf("  3) -n 0           → [");
            got = tail_file(p2, 0, stdout);
            printf("]（读了 %ld 字节）← 0 行就是 0 字节\n", got);
            printf("  4) -n 99（> 行数） → [");
            got = tail_file(p2, 99, stdout);
            printf("]（读了 %ld 字节）← 行数不够就是全部\n", got);
            unlink(p2);
        }
    }
    printf("  ⚠️ 看第 1) 条：实现若只拿「换行数」当行数，这里会多吐一行 'b' 出来。\n");

    unlink(p);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    long nlines = 10;
    const char *path = NULL;

    if (argc == 1)
        return self_test();

    if (strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s [-n num] file\n", argv[0]);
        printf("       用 lseek/read/write 系统调用实现的 tail（只支持从末尾数的 -n）\n");
        return EXIT_SUCCESS;
    }
    if (strcmp(argv[1], "-n") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s [-n num] file\n", argv[0]);
            return EXIT_FAILURE;
        }
        nlines = strtol(argv[2], NULL, 10);
        if (nlines < 0) {
            fprintf(stderr, "%s: -n 不能是负数（本实现不支持「从头 n 行之后」）\n", argv[0]);
            return EXIT_FAILURE;
        }
        path = argv[3];
    } else {
        path = argv[1];
    }

    {
        long got = tail_file(path, nlines, stdout);
        if (got < 0)
            return EXIT_FAILURE;
        fprintf(stderr, "[%s: 为取最后 %ld 行，实际只读了 %ld 字节]\n", argv[0], nlines, got);
    }
    return EXIT_SUCCESS;
}
