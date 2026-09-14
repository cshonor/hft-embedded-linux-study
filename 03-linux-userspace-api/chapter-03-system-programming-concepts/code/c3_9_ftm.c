/* TLPI 第 03 章 §3.6.1 —— 功能测试宏（feature test macros）到底在控制什么
 *
 * 同一个头文件里，同一个函数「可见 / 不可见」取决于你在 `#include` **之前**
 * 定义了哪些宏。glibc 的 <features.h> 把你的宏翻译成一组内部开关 __USE_*，
 * 各头文件再按这些开关决定要不要声明某个函数。
 *
 * ⚠️ 本文件列出的 __USE_* 名字与下面 ③ 段的守护条件，全部**逐字抄**自
 *    glibc 2.39 的 include/features.h、string/string.h、time/time.h：
 *      · features.h 里真实存在 25 个 __USE_*；**没有** __USE_POSIX2008 这个名字，
 *        POSIX.1-2008 在 features.h 里的体现是 __USE_XOPEN2K8
 *      · memmem() 的守护开关是 __USE_MISC（string.h:385），不是 __USE_GNU
 *      · strerror_r() 在 __USE_GNU 下是另一种签名（string.h:428 / 448）
 *
 * 本 demo 的意义在于：**同一个源码，用不同的编译参数编出来，输出完全不同**。
 * 笔记里给出 5 组参数下的实测输出对照。
 *
 * 编译（5 组，见笔记实测块）：
 *   gcc -O0 -Wall -Wextra -o c3_9 c3_9_ftm.c
 *   gcc -O0 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -o c3_9 c3_9_ftm.c
 *   gcc -O0 -Wall -Wextra -D_XOPEN_SOURCE=700     -o c3_9 c3_9_ftm.c
 *   gcc -O0 -Wall -Wextra -D_GNU_SOURCE           -o c3_9 c3_9_ftm.c
 *   gcc -O0 -Wall -Wextra -std=c99                -o c3_9 c3_9_ftm.c
 */
#include <features.h>
#include <stdio.h>

static void flag(const char *name, int on)
{
    printf("  %-26s %s\n", name, on ? "ON " : "OFF");
}

int main(void)
{
    printf("=== ① 我定义了什么（编译参数 + glibc 的自动补全）===\n");
#ifdef _GNU_SOURCE
    printf("  _GNU_SOURCE        = %ld\n", (long) _GNU_SOURCE);
#else
    printf("  _GNU_SOURCE        未定义\n");
#endif
#ifdef _POSIX_C_SOURCE
    printf("  _POSIX_C_SOURCE    = %ldL\n", (long) _POSIX_C_SOURCE);
#else
    printf("  _POSIX_C_SOURCE    未定义\n");
#endif
#ifdef _XOPEN_SOURCE
    printf("  _XOPEN_SOURCE      = %ld\n", (long) _XOPEN_SOURCE);
#else
    printf("  _XOPEN_SOURCE      未定义\n");
#endif
#ifdef _DEFAULT_SOURCE
    printf("  _DEFAULT_SOURCE    = %ld  （没指定别的 FTM、又不是严格 ANSI 时 glibc 自己给的）\n",
           (long) _DEFAULT_SOURCE);
#else
    printf("  _DEFAULT_SOURCE    未定义\n");
#endif
#ifdef __STRICT_ANSI__
    printf("  __STRICT_ANSI__    = %d  （-std=c99 这类「严格标准」模式才会置上）\n",
           __STRICT_ANSI__);
#else
    printf("  __STRICT_ANSI__    未定义\n");
#endif
    printf("  __STDC_VERSION__   = %ldL\n", (long) __STDC_VERSION__);

    printf("\n=== ② glibc 把它翻译成的内部开关 __USE_* ===\n");
#ifdef __USE_ISOC99
    flag("__USE_ISOC99", 1);
#else
    flag("__USE_ISOC99", 0);
#endif
#ifdef __USE_ISOC11
    flag("__USE_ISOC11", 1);
#else
    flag("__USE_ISOC11", 0);
#endif
#ifdef __USE_POSIX_IMPLICITLY
    flag("__USE_POSIX_IMPLICITLY", 1);      /* 用户没点 POSIX，glibc 替他点了 */
#else
    flag("__USE_POSIX_IMPLICITLY", 0);
#endif
#ifdef __USE_POSIX
    flag("__USE_POSIX", 1);
#else
    flag("__USE_POSIX", 0);
#endif
#ifdef __USE_POSIX199309
    flag("__USE_POSIX199309", 1);           /* 决定 struct timespec / clock_gettime */
#else
    flag("__USE_POSIX199309", 0);
#endif
#ifdef __USE_POSIX199506
    flag("__USE_POSIX199506", 1);           /* 线程相关声明的门槛 */
#else
    flag("__USE_POSIX199506", 0);
#endif
#ifdef __USE_XOPEN
    flag("__USE_XOPEN", 1);
#else
    flag("__USE_XOPEN", 0);
#endif
#ifdef __USE_XOPEN_EXTENDED
    flag("__USE_XOPEN_EXTENDED", 1);
#else
    flag("__USE_XOPEN_EXTENDED", 0);
#endif
#ifdef __USE_UNIX98
    flag("__USE_UNIX98", 1);
#else
    flag("__USE_UNIX98", 0);
#endif
#ifdef __USE_XOPEN2K
    flag("__USE_XOPEN2K", 1);
#else
    flag("__USE_XOPEN2K", 0);
#endif
#ifdef __USE_XOPEN2K8
    flag("__USE_XOPEN2K8", 1);              /* ←「POSIX.1-2008」在 glibc 里的真名 */
#else
    flag("__USE_XOPEN2K8", 0);
#endif
#ifdef __USE_XOPEN2K8XSI
    flag("__USE_XOPEN2K8XSI", 1);
#else
    flag("__USE_XOPEN2K8XSI", 0);
#endif
#ifdef __USE_LARGEFILE
    flag("__USE_LARGEFILE", 1);
#else
    flag("__USE_LARGEFILE", 0);
#endif
#ifdef __USE_LARGEFILE64
    flag("__USE_LARGEFILE64", 1);
#else
    flag("__USE_LARGEFILE64", 0);
#endif
#ifdef __USE_FILE_OFFSET64
    flag("__USE_FILE_OFFSET64", 1);
#else
    flag("__USE_FILE_OFFSET64", 0);
#endif
#ifdef __USE_MISC
    flag("__USE_MISC", 1);                  /* BSD/SVID 兼容物 + memmem */
#else
    flag("__USE_MISC", 0);
#endif
#ifdef __USE_ATFILE
    flag("__USE_ATFILE", 1);                /* openat/fstatat 一族 */
#else
    flag("__USE_ATFILE", 0);
#endif
#ifdef __USE_GNU
    flag("__USE_GNU", 1);                   /* GNU 扩展 */
#else
    flag("__USE_GNU", 0);
#endif

    printf("\n=== ③ 这些开关真的决定声明在不在（守护条件照抄 glibc 头文件）===\n");
#if (defined __USE_XOPEN_EXTENDED || defined __USE_XOPEN2K8 || __GLIBC_USE (LIB_EXT2) || __GLIBC_USE (ISOC2X))
    printf("  strdup()              ：可见     ← string.h:184 的 #if 条件（照抄）\n");
#else
    printf("  strdup()              ：**不可见**   ← string.h:184 的 #if 条件（照抄）\n");
#endif
#ifdef __USE_POSIX199309
    printf("  struct timespec / clock_gettime() / nanosleep()：可见   ← time.h:41, 276\n");
#else
    printf("  struct timespec / clock_gettime() / nanosleep()：**不可见**   ← time.h:41, 276\n");
#endif
#ifdef __USE_MISC
    printf("  memmem()              ：可见     ← string.h:385 守的是 __USE_MISC，不是 __USE_GNU\n");
#else
    printf("  memmem()              ：**不可见**   ← string.h:385 守的是 __USE_MISC，不是 __USE_GNU\n");
#endif
#ifdef __USE_XOPEN2K8
    printf("  strsignal()           ：可见     ← string.h:476\n");
#else
    printf("  strsignal()           ：**不可见**   ← string.h:476\n");
#endif
#ifdef __USE_GNU
    printf("  strerror_r()          ：GNU 版签名（返回 char *，缓冲由 glibc 管）   ← string.h:448\n");
#else
    printf("  strerror_r()          ：POSIX 版签名（返回 int，写进你给的 buf）      ← string.h:428\n");
#endif
    printf("    ↑ 最后一个最值得注意：**同一个函数名，两种 FTM 下签名都不同**。\n"
           "      换旗标不只是「能不能用」，还可能改变函数的 ABI。\n");

    printf("\n=== ④ 四条实践规则 ===\n");
    printf("  1. FTM 必须写在**所有 #include 之前**（<features.h> 被谁先拉进来就定了）\n");
    printf("  2. 要「严格 POSIX + 可移植」→ 定义 _POSIX_C_SOURCE；要 GNU 扩展 → _GNU_SOURCE\n");
    printf("  3. 别混着定义互相矛盾的 FTM；glibc 会按固定优先级归并，结果常出乎意料\n");
    printf("  4. **加旗标不一定只增不减**：-std=c99 会带出 __STRICT_ANSI__，\n"
           "     于是 glibc 不再自动给 _DEFAULT_SOURCE/_POSIX_C_SOURCE，\n"
           "     连 memmem（靠 _DEFAULT_SOURCE）都会跟着消失\n");
    return 0;
}
