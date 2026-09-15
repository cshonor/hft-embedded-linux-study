/* c11_1_three_kinds.c — Ch11 §11.1：三类限制的现场对照
 *
 * TLPI 把 SUSv3 的限制分三类，本节只用一次实验把「类 1 恒定」和
 * 「类 3 运行时可改」摆在一起：
 *
 *   实验：先查四个 _SC_*，再把 RLIMIT_NOFILE 的 soft 减半，再查同样的四项。
 *         _SC_CLK_TCK / _SC_PAGESIZE / _SC_VERSION 一位不变（类 1）；
 *         _SC_OPEN_MAX 跟着 RLIMIT_NOFILE 走（类 3 —— 它甚至不是「上限」，
 *         只是一个当前值）。
 *
 * 类 2（路径相关）需要另一个文件系统才能看出差别，放到 §11.3 / 习题 11-2。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c11_1_three_kinds c11_1_three_kinds.c
 * 取材： sysconf(3) 的 RETURN VALUE 段（man-pages 6.19）
 *        glibc 2.39 sysdeps/posix/sysconf.c:82-83（_SC_CLK_TCK -> __getclktck()）
 *                              :92-93（_SC_OPEN_MAX -> __getdtablesize()）
 *                  sysdeps/posix/getdtsz.c:32（= getrlimit(RLIMIT_NOFILE).rlim_cur）
 *                  sysdeps/unix/sysv/linux/getclktck.c:22-23（SYSTEM_CLK_TCK 兜底 100）
 *        Linux v6.6 include/uapi/asm-generic/resource.h:30-31（RLIMIT_NOFILE = 7）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

static long sc(int name)
{
    errno = 0;
    return sysconf(name);
}

static const struct { int name; const char *lbl; } FOUR[] = {
    { _SC_CLK_TCK,      "_SC_CLK_TCK" },
    { _SC_PAGESIZE,     "_SC_PAGESIZE" },
    { _SC_VERSION,      "_SC_VERSION" },
    { _SC_OPEN_MAX,     "_SC_OPEN_MAX" },
};

static void dump_four(const char *when)
{
    printf("--- %s ---\n", when);
    for (size_t i = 0; i < sizeof(FOUR) / sizeof(FOUR[0]); i++)
        printf("  %-16s = %ld\n", FOUR[i].lbl, sc(FOUR[i].name));
}

int main(void)
{
    printf("### 一次实验：把 RLIMIT_NOFILE 的 soft 减半，再看这四项\n");
    dump_four("改 rlimit 之前");

    struct rlimit r;
    if (getrlimit(RLIMIT_NOFILE, &r) != 0) {
        perror("getrlimit");
        return 1;
    }
    printf("\nRLIMIT_NOFILE: soft=%lu hard=%lu\n",
           (unsigned long)r.rlim_cur, (unsigned long)r.rlim_max);

    struct rlimit low = r;
    low.rlim_cur = (r.rlim_max >= 10) ? r.rlim_max / 2 : r.rlim_max;   /* 只降不升 */
    if (setrlimit(RLIMIT_NOFILE, &low) != 0) {
        printf("setrlimit 失败: %s\n", strerror(errno));
        return 1;
    }
    printf("setrlimit(soft -> %lu) 成功\n\n", (unsigned long)low.rlim_cur);

    dump_four("改 rlimit 之后");

    printf("\n结论：前三项与环境无关（类 1 运行时恒定）；\n");
    printf("      _SC_OPEN_MAX 只是「RLIMIT_NOFILE 的当前值」，改了它就变（类 3）。\n");
    printf("      glibc 的实现是一行：return __getrlimit(RLIMIT_NOFILE,&ru)<0 ? OPEN_MAX : ru.rlim_cur;\n");

    printf("\n### SUSv3 保证的只是「不低于这个下限」（<limits.h> / <unistd.h>）\n");
    printf("  %-22s %-10s %s\n", "保证下限宏", "值", "本机实测（未改 rlimit 时）");
    printf("  %-22s %-10s %ld\n", "(CLK_TCK：无下限宏)", "--", sc(_SC_CLK_TCK));
    printf("  %-22s %-10d %ld\n", "_POSIX_OPEN_MAX", _POSIX_OPEN_MAX,
           (long)r.rlim_max);
    printf("  %-22s %-10d %ld\n", "_POSIX_CHILD_MAX", _POSIX_CHILD_MAX, sc(_SC_CHILD_MAX));
    printf("  %-22s %-10d %ld\n", "_POSIX_NGROUPS_MAX", _POSIX_NGROUPS_MAX, sc(_SC_NGROUPS_MAX));
    printf("  %-22s %-10d %ld\n", "_POSIX_STREAM_MAX", _POSIX_STREAM_MAX, sc(_SC_STREAM_MAX));
    printf("  %-22s %-10d %ld\n", "_POSIX_LOGIN_NAME_MAX", _POSIX_LOGIN_NAME_MAX,
           sc(_SC_LOGIN_NAME_MAX));
    printf("  %-22s %-10d %ld\n", "_POSIX_TTY_NAME_MAX", _POSIX_TTY_NAME_MAX, sc(_SC_TTY_NAME_MAX));
    printf("  %-22s %-10d %ld\n", "_POSIX_NAME_MAX", _POSIX_NAME_MAX, (long)NAME_MAX);
    printf("  %-22s %-10d %ld\n", "_POSIX_PATH_MAX", _POSIX_PATH_MAX, (long)PATH_MAX);
    printf("  %-22s %-10d %ld\n", "_POSIX_PIPE_BUF", _POSIX_PIPE_BUF, (long)PIPE_BUF);

    printf("\n注意四件事：\n");
    printf("  1) 下限宏是「保证」，不是「实际」。_POSIX_NAME_MAX=14 看着很荒唐，\n");
    printf("     但 POSIX 只承诺 14 —— 它允许一个文件系统只支持 14 字符文件名。\n");
    printf("  2) <limits.h> 里 NAME_MAX/PATH_MAX/PIPE_BUF 有定义，ARG_MAX/OPEN_MAX/LINK_MAX\n");
    printf("     却被 glibc 主动 #undef 掉了（见 §11.3 的源码）。用它们之前先 #ifdef。\n");
    printf("  3) _SC_CLK_TCK 不等于内核的 HZ：它是「times() 的节拍频率」，\n");
    printf("     本机是 %ld，而内核 HZ 是内核编译期常量，两者不必相同。\n", sc(_SC_CLK_TCK));
    printf("  4) 第一行连下限宏都没有：全库找不到 _POSIX_CLK_TCK。POSIX.1-2017\n");
    printf("     <limits.h>/<sysconf> 原文只说「The symbol CLK_TCK is obsolescent\n");
    printf("     and removed.」—— 它只保证 _SC_CLK_TCK 查得到，不承诺任何下限。\n");
    return 0;
}
