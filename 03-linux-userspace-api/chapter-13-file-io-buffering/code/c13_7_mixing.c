/* c13_7_mixing.c — Ch13 §13.7：混用 stdio 函数与 I/O 系统调用
 *
 * TLPI §13.7（p.248）的结论很硬：
 *   **在同一文件上混用 stdio 与 read/write，如果不加保护，就会乱序甚至丢数据。**
 * 原因是两层缓冲互不可见（13.4 讲的那张图），而且两者**都要推进文件偏移**。
 *
 * 原书用两个小程序演示（`filebuff/mix23io.c`、`filebuff/mix23_linebuff.c`，
 * 后者是习题 13-4 的官方解答），它们的现象**只在 stdout 是终端时**才看得全。
 * 本程序换一个不依赖终端的做法：**把顺序"物化"到文件里** ——
 *   每次实验都往一个真实文件写，写完把文件读回来，用内容比对判断顺序对不对。
 * 这样无论 stdout 是终端、管道还是 socket，结论都一样，而且在 CE 上可复现。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c13_7_mixing c13_7_mixing.c
 * 取材： TLPI §13.7（混用的三种后果与两种修法；File 13-1/13-2 的演示程序）
 *       man-pages 6.19 stdio(3)（"the stream and the file descriptor share the
 *         file offset"；混用时的位置同步要求）
 *                      fflush(3)（把用户态缓冲交给内核）
 *                      fdopen(3) / fileno(3)（两层之间的桥）
 *       Linux v6.6 fs/read_write.c（ksys_write / vfs_write 对 f_pos 的推进）
 *       C99 7.19.5.3/7.21.5.3（fflush / lseek 对流的语义要求）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define A "AAAA"      /* 走 stdio */
#define B "BBBB"      /* 走 write() */
#define WANT A B      /* 期望的正确顺序 */

/* 把读回来的原始字节转成「看得懂」的形式：
   连续 NUL（外部 lseek 造成空洞时留下的）压成 ~N~，换行写成 \n，其余不可打印写成 ?。
   注意：顺序判定仍然用**原始字节**做 strcmp，这里只改变**显示**。 */
static void sanitize(const char *in, size_t n, char *out, size_t cap)
{
    size_t i = 0, j = 0;

    while (i < n && j + 24 < cap) {
        if (in[i] == '\0') {
            size_t k = i;

            while (k < n && in[k] == '\0')
                k++;
            j += (size_t) snprintf(out + j, cap - j, "~%zu~", k - i);
            i = k;
        } else if (in[i] == '\n') {
            out[j++] = '\\';
            out[j++] = 'n';
            i++;
        } else if ((unsigned char) in[i] < 0x20 || (unsigned char) in[i] > 0x7e) {
            out[j++] = '?';
            i++;
        } else {
            out[j++] = in[i++];
        }
    }
    out[j] = '\0';
}

/* 把结果读回来，并判定顺序 */
static void verdict(const char *tag, const char *path, const char *detail)
{
    char got[128];
    char shown[512];
    struct stat st;
    FILE *fp = fopen(path, "r");
    size_t n;

    memset(got, 0, sizeof(got));
    memset(&st, 0, sizeof(st));
    st.st_size = -1;
    if (fp == NULL) {
        printf("  %-34s 读回失败 errno=%d(%s)\n", tag, errno, strerror(errno));
        return;
    }
    n = fread(got, 1, sizeof(got) - 1, fp);
    got[n] = '\0';
    (void) fstat(fileno(fp), &st);
    fclose(fp);
    sanitize(got, n, shown, sizeof(shown));

    printf("  %-34s 文件内容 = [%s]（文件 %ld 字节，读回 %zu 字节）\n",
           tag, shown, (long) st.st_size, n);
    printf("  %-34s 期望 = [%s]  → %s\n", "", WANT,
           strcmp(got, WANT) == 0 ? "✓ 顺序正确" : "✗ 顺序错了");
    printf("  %-34s %s\n", "", detail);
}

int main(void)
{
    printf("== 把「两层缓冲打架」物化到文件里 ==\n");
    printf("  每次实验：先用 fprintf 写 \"%s\"，再用 write() 写 \"%s\"，最后读回来看顺序。\n", A, B);
    printf("  期望的正确顺序是 [%s]。\n\n", WANT);

    printf("-- ① 反面教材：fprintf 之后直接 write 同一个 fd --\n");
    {
        const char *p = "/app/c13_7_a.txt";
        FILE *fp = fopen(p, "w");
        int fd;

        if (fp == NULL) {
            printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        fd = fileno(fp);
        fprintf(fp, "%s", A);                       /* 进 stdio 缓冲 */
        if (write(fd, B, strlen(B)) != (ssize_t) strlen(B))
            printf("  write 失败 errno=%d(%s)\n", errno, strerror(errno));
        fclose(fp);                                 /* 这时才隐式 fflush */
        verdict("① debug 乱序（未 fflush）", p,
                "→ write() 先到内核先落位；fprintf 的字节一直等到 fclose 才出去");
        unlink(p);
    }

    printf("\n-- ② 修法一：混用前先 fflush --\n");
    {
        const char *p = "/app/c13_7_b.txt";
        FILE *fp = fopen(p, "w");
        int fd;

        if (fp == NULL) {
            printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        fd = fileno(fp);
        fprintf(fp, "%s", A);
        fflush(fp);                                 /* ← 关键的一行 */
        if (write(fd, B, strlen(B)) != (ssize_t) strlen(B))
            printf("  write 失败 errno=%d(%s)\n", errno, strerror(errno));
        fclose(fp);
        verdict("② fflush 之后再 write", p,
                "→ 标准做法：**每次切换接口前 fflush**（读方向则要处理 lseek）");
        unlink(p);
    }

    printf("\n-- ③ 修法二：干脆关掉 stdio 缓冲 --\n");
    {
        const char *p = "/app/c13_7_c.txt";
        FILE *fp = fopen(p, "w");
        int fd;

        if (fp == NULL) {
            printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        fd = fileno(fp);
        setvbuf(fp, NULL, _IONBF, 0);               /* 无缓冲，写一个走一个 */
        fprintf(fp, "%s", A);
        if (write(fd, B, strlen(B)) != (ssize_t) strlen(B))
            printf("  write 失败 errno=%d(%s)\n", errno, strerror(errno));
        fclose(fp);
        verdict("③ setvbuf(_IONBF) 后混用", p,
                "→ 顺序对，但每次 fprintf 都变成一次 write()：混用不再危险，代价是性能");
        unlink(p);
    }

    printf("\n-- ④ 换一个 fd 不解决问题（dup 出来的 fd 共享同一个文件偏移） --\n");
    {
        const char *p = "/app/c13_7_d.txt";
        FILE *fp = fopen(p, "w");
        int dupfd;

        if (fp == NULL) {
            printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        dupfd = dup(fileno(fp));
        fprintf(fp, "%s", A);
        if (write(dupfd, B, strlen(B)) != (ssize_t) strlen(B))
            printf("  write 失败 errno=%d(%s)\n", errno, strerror(errno));
        close(dupfd);
        fclose(fp);
        verdict("④ 用 dup 出来的 fd 去 write", p,
                "→ **照样乱序**：dup 出来的 fd 与原来共享文件偏移，避开的是 fd 不是缓冲");
        unlink(p);
    }

    printf("\n-- ⑤ 反向的坑：用另一个 fd 去 lseek，会让 stdio 写错位置 --\n");
    {
        const char *p = "/app/c13_7_e.txt";
        FILE *fp = fopen(p, "w");
        int fd;

        if (fp == NULL) {
            printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        fd = fileno(fp);
        fflush(fp);                              /* 先清空，免得把「续写」和「空洞」混在一起看 */
        if (lseek(fd, 100, SEEK_SET) == -1)       /* 用裸 fd 把偏移挪到 100 */
            printf("  lseek 失败 errno=%d(%s)\n", errno, strerror(errno));
        fprintf(fp, "%s", A);                      /* FILE 没有自己的偏移 → 就写到 100 处 */
        fflush(fp);
        fclose(fp);
        verdict("⑤ 外部 lseek 后继续 fprintf", p,
                "→ 写进去了，但落在偏移 100：前面 ~100~ 是空洞（全 NUL 字节）");
        unlink(p);
    }

    printf("\n== 结论：什么时候能混、怎么混 ==\n");
    printf("  ┌ %-8s %s\n", "做法", "判定");
    printf("  ├ %-8s %s\n", "只用一套", "✅ 首选。要么全程 stdio，要么全程 read/write");
    printf("  ├ %-8s %s\n", "fflush 过渡", "✅ 每次从 stdio 切到 write 之前 fflush(fp)");
    printf("  ├ %-8s %s\n", "关缓冲", "✅ setvbuf(fp, NULL, _IONBF, 0)，代价是没了缓冲收益");
    printf("  ├ %-8s %s\n", "fdopen 包装", "✅ 给 fd 套一个 FILE*，之后**全走 stdio**");
    printf("  ├ %-8s %s\n", "换 fd", "❌ 没用，dup 出来的 fd 共享文件偏移");
    printf("  └ %-8s %s\n", "直接混", "❌ 乱序 / 覆盖 / 丢数据，且**难以复现**");
    printf("\n  原书两个演示程序在本目录：`mix23io.c`、`mix23_linebuff.c`（习题 13-4 的官方解答）。\n");
    printf("  ⚠️ 在 CE 上跑它们时，stdout 是 **socket**（不是终端）→ 一律全缓冲，\n");
    printf("     所以只能观察到「重定向」那一半现象（write 先出、printf 后出）。\n");
    printf("     「终端下行缓冲、printf 先出」那一半在 CE 里**观察不到** ——\n");
    printf("     要看那一半得用真终端，或者用 13.2 里「显式 setvbuf(_IOLBF)」的办法间接验证。\n");
    return EXIT_SUCCESS;
}
