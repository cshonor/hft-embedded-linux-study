/* ex11_1_sysconf_wide.c — 原书习题 11-1 解答
 *
 * 【任务】（对应原书 Ch11 Exercises 11-1）把 Listing 11-1 拿到**另一个 UNIX 实现**
 *   上跑一遍、对比结果。
 *   ⚠️ 原书习题文字的公开原文未能核验（man7 只分发源码、不放习题文字），
 *      所以这里只写任务要求，不做逐字引用。
 *
 * 【本题的困难】本仓库只有 Linux（Compiler Explorer 的 Ubuntu 24.04 容器），
 *   拿不到第二个 UNIX 实现。所以这里做两件能真正落地的事：
 *
 *   ① 把 Listing 11-1 的 6 项**原样先打一遍**（方便和原书输出逐行对照）；
 *   ② 给出一张「宽表」：每项的值 + glibc 2.39 里的取值路径 + POSIX 只保证的
 *      下限。第三列才是本题的真正答案 —— 它解释了「在别的 UNIX 上，哪些
 *      项允许不一样、允许差到多少」。
 *
 * 编译： gcc -O0 -Wall -Wextra -o ex11_1_sysconf_wide ex11_1_sysconf_wide.c
 * 取材： 原书 Listing 11-1 = syslim/t_sysconf.c（page 216），本目录 t_sysconf.c 为其逐字镜像
 *       glibc 2.39 sysdeps/posix/sysconf.c:68-93 / :265-267 / :1113 / :1123
 *       glibc 2.39 sysdeps/unix/sysv/linux/sysconf.c:56-67 / :69-73
 *       glibc 2.39 sysdeps/unix/sysv/linux/x86/sysconf.c:33-36（只多管 CPU cache）
 *       man-pages 6.19 sysconf(3)：每一项的 _POSIX_* 下限
 */
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void orig(const char *msg, int name)
{
    errno = 0;
    long lim = sysconf(name);
    if (lim != -1)
        printf("%s %ld\n", msg, lim);
    else if (errno == 0)
        printf("%s (indeterminate)\n", msg);
    else
        printf("%s ERROR: %s\n", msg, strerror(errno));
}

struct row { int name; const char *lbl; long floor; const char *src; };

int main(void)
{
    printf("=== ① 原样重跑 Listing 11-1（page 216）的 6 项 ===\n");
    orig("_SC_ARG_MAX:        ", _SC_ARG_MAX);
    orig("_SC_LOGIN_NAME_MAX: ", _SC_LOGIN_NAME_MAX);
    orig("_SC_OPEN_MAX:       ", _SC_OPEN_MAX);
    orig("_SC_NGROUPS_MAX:    ", _SC_NGROUPS_MAX);
    orig("_SC_PAGESIZE:       ", _SC_PAGESIZE);
    orig("_SC_RTSIG_MAX:      ", _SC_RTSIG_MAX);

    printf("\n=== ② 宽表：值 / POSIX 保证下限 / glibc 里的取值路径 ===\n");
    const struct row T[] = {
        { _SC_ARG_MAX, "_SC_ARG_MAX", _POSIX_ARG_MAX,
          "sysdeps/unix/sysv/linux/sysconf.c:56-67  min(max(131072, RLIMIT_STACK/4), 6MiB)" },
        { _SC_LOGIN_NAME_MAX, "_SC_LOGIN_NAME_MAX", _POSIX_LOGIN_NAME_MAX,
          "sysdeps/unix/sysv/linux/bits/local_lim.h:90  LOGIN_NAME_MAX 256" },
        { _SC_OPEN_MAX, "_SC_OPEN_MAX", _POSIX_OPEN_MAX,
          "sysdeps/posix/sysconf.c:92 -> getdtsz.c:32  getrlimit(RLIMIT_NOFILE).rlim_cur" },
        { _SC_NGROUPS_MAX, "_SC_NGROUPS_MAX", _POSIX_NGROUPS_MAX,
          "linux/sysconf.c:69-73  读 /proc/sys/kernel/ngroups_max" },
        { _SC_PAGESIZE, "_SC_PAGESIZE", 1,
          "sysdeps/posix/sysconf.c:220  _SC_PAGESIZE -> __getpagesize()" },
        { _SC_RTSIG_MAX, "_SC_RTSIG_MAX", _POSIX_RTSIG_MAX,
          "sysdeps/posix/sysconf.c:265-267 -> RTSIG_MAX（<linux/limits.h>:19 的 32）" },
    };
    printf("  %-22s %-12s %-10s %s\n", "name", "实测", "下限", "POSIX 只保证 / 差值意味着什么");
    for (size_t i = 0; i < sizeof(T) / sizeof(T[0]); i++) {
        errno = 0;
        long v = sysconf(T[i].name);
        char vi[16];
        if (v == -1) snprintf(vi, sizeof vi, "(-1)");
        else snprintf(vi, sizeof vi, "%ld", v);
        printf("  %-22s %-12s %-10ld %s\n", T[i].lbl, vi, T[i].floor,
               (T[i].floor > 0 && v > 0 && v != T[i].floor) ? "** 比下限大 **" : "");
    }
    printf("\n  每项的完整来源：\n");
    for (size_t i = 0; i < sizeof(T) / sizeof(T[0]); i++)
        printf("    %-22s %s\n", T[i].lbl, T[i].src);

    printf("\n=== ③ 这就是「在别的 UNIX 上会不一样」的答案 ===\n");
    printf("  POSIX 对每一项只给「不低于」的下限，所以另一台 UNIX 上合法的最小值是：\n");
    printf("    _SC_ARG_MAX        >= %d      （本机 %ld）\n", _POSIX_ARG_MAX, sysconf(_SC_ARG_MAX));
    printf("    _SC_LOGIN_NAME_MAX >= %d       （本机 %ld）\n", _POSIX_LOGIN_NAME_MAX,
           sysconf(_SC_LOGIN_NAME_MAX));
    printf("    _SC_OPEN_MAX       >= %d      （本机 %ld）\n", _POSIX_OPEN_MAX, sysconf(_SC_OPEN_MAX));
    printf("    _SC_NGROUPS_MAX    >= %d       （本机 %ld）\n", _POSIX_NGROUPS_MAX,
           sysconf(_SC_NGROUPS_MAX));
    printf("    _SC_PAGESIZE       >= %d       （本机 %ld）\n", 1, sysconf(_SC_PAGESIZE));
    printf("    _SC_RTSIG_MAX      >= %d       （本机 %ld）\n", _POSIX_RTSIG_MAX,
           sysconf(_SC_RTSIG_MAX));
    printf("  也就是说：_SC_OPEN_MAX 在另一台机器上可能是 20，_SC_NGROUPS_MAX 可能是 8，\n");
    printf("  _SC_ARG_MAX 可能是 4096 —— 全都合法。写 `char buf[1024]` 赌「路径够短」或者\n");
    printf("  `#define MAX_FD 1024` 就是踩这个坑。\n");

    printf("\n=== ④ 本机这几个值的稳定性（同一进程内查两次）===\n");
    long a1 = sysconf(_SC_OPEN_MAX), a2 = sysconf(_SC_OPEN_MAX);
    long b1 = sysconf(_SC_CLK_TCK), b2 = sysconf(_SC_CLK_TCK);
    printf("  _SC_OPEN_MAX 两次: %ld / %ld -> %s\n", a1, a2, a1 == a2 ? "一致" : "不一致");
    printf("  _SC_CLK_TCK  两次: %ld / %ld -> %s\n", b1, b2, b1 == b2 ? "一致" : "不一致");
    printf("  sysconf(3) 原文：「The values obtained from these functions are system\n");
    printf("  configuration constants.  They do not change during the lifetime of a process.」\n");
    printf("  ——但 _SC_OPEN_MAX 会跟着 setrlimit 变，所以「一生不变」这句对限制类并不成立，\n");
    printf("     见 §11.1 的 c11_1_three_kinds.c。\n");
    return 0;
}
