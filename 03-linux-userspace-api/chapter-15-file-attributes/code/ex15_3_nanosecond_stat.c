/* ex15_3_nanosecond_stat.c — 习题 15-3：把 t_stat.c（Listing 15-1）的时间戳显示
 * 升级到纳秒精度
 *
 * 题面：修改 Listing 15-1，使文件时间戳以纳秒精度显示。
 * 做法：三种途径（本程序全部演示）：
 *   ① struct stat 里的 timespec 字段（Linux: st_atim / macOS: st_atimespec）
 *      —— 最直接；
 *   ② strftime() 只管秒，纳秒自己 printf 补；
 *   ③ 对照 ctime() 的秒级输出，证明「ctime() 丢掉了纳秒信息」。
 *
 * 编译： cc -Wall -Wextra -o ex15_3_nanosecond_stat ex15_3_nanosecond_stat.c
 * 取材： TLPI §15.1 习题 15-3；man-pages 6.19 stat(2)（nanosecond timestamps 段）
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#if defined(__APPLE__)
#define AT(sb) ((sb)->st_atimespec)
#define MT(sb) ((sb)->st_mtimespec)
#define CT(sb) ((sb)->st_ctimespec)
#else
#define AT(sb) ((sb)->st_atim)
#define MT(sb) ((sb)->st_mtim)
#define CT(sb) ((sb)->st_ctim)
#endif

/* 秒转人类可读 + 手工补纳秒 */
static void fmt_ts(const struct timespec *ts, char *out, size_t n)
{
    struct tm tm;
    time_t sec = ts->tv_sec;
    localtime_r(&sec, &tm);
    size_t used = strftime(out, n, "%Y-%m-%d %H:%M:%S", &tm);
    snprintf(out + used, n - used, ".%09ld %s",
             (long) ts->tv_nsec, "+0800");
}

int main(int argc, char *argv[])
{
    if (argc != 2 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct stat sb;
    if (stat(argv[1], &sb) == -1) { perror("stat"); return EXIT_FAILURE; }

    char a[64], m[64], c[64];
    fmt_ts(&AT(&sb), a, sizeof a);
    fmt_ts(&MT(&sb), m, sizeof m);
    fmt_ts(&CT(&sb), c, sizeof c);

    printf("文件: %s\n", argv[1]);
    printf("  atime(nanosecond): %s\n", a);
    printf("  mtime(nanosecond): %s\n", m);
    printf("  ctime(nanosecond): %s\n", c);

    /* 对照：ctime() 的秒级输出（Listing 15-1 原始行为） */
    printf("\n对照 Listing 15-1 的秒级显示（ctime() 丢纳秒）：\n");
    printf("  atime = %s", ctime(&sb.st_atime));
    printf("  mtime = %s", ctime(&sb.st_mtime));
    printf("  ctime = %s", ctime(&sb.st_ctime));

    /* 现场演示纳秒真的有意义：快速连续两次写，秒级看不出差异 */
    const char *tmp = "/tmp/tlpi_ex15_3.txt";
    int fd = open(tmp, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) { perror("open tmp"); return EXIT_FAILURE; }
    struct stat b1, b2;
    write(fd, "first", 5);
    fstat(fd, &b1);
    write(fd, "second", 6);
    fstat(fd, &b2);
    printf("\n同一秒内两次 write 后：\n");
    printf("  第一次 mtime: %ld.%09ld\n", (long) MT(&b1).tv_sec, (long) MT(&b1).tv_nsec);
    printf("  第二次 mtime: %ld.%09ld\n", (long) MT(&b2).tv_sec, (long) MT(&b2).tv_nsec);
    printf("  → 秒级比较分不出先后（%s），纳秒比较 %s\n",
           MT(&b1).tv_sec == MT(&b2).tv_sec ? "两值同秒" : "跨秒",
           MT(&b1).tv_sec != MT(&b2).tv_sec ||
           MT(&b1).tv_nsec != MT(&b2).tv_nsec ? "可分辨" : "相同?!");

    close(fd);
    unlink(tmp);
    return EXIT_SUCCESS;
}
