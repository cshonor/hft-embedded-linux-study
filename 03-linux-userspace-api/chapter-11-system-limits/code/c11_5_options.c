/* c11_5_options.c — Ch11 §11.5：系统选项（System Options）的三态与两条查法
 *
 * 限制是「一个数」，选项是「支持 / 不支持」。POSIX 给选项设计了三态：
 *   _POSIX_FOO 未定义      -> 编译期不知道，必须运行时问 sysconf(_SC_FOO)
 *   _POSIX_FOO == -1       -> 明确不支持
 *   _POSIX_FOO == 0        -> 头文件/函数都在，但支持到什么程度要运行时问
 *   _POSIX_FOO == 其他正值 -> 支持，通常就是描述它的 POSIX 修订版年月（如 200809L）
 *
 * 本程序把这三态用宏判断打出来，再和运行时的 sysconf(_SC_*) 对齐，最后加一段
 * confstr() —— 它查的是**字符串**选项（路径、版本号），是 sysconf 的字面兄弟。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c11_5_options c11_5_options.c
 * 取材： sysconf(3) DESCRIPTION 段（man-pages 6.19）逐字：
 *         "For options, typically, there is a constant _POSIX_FOO that may be
 *          defined in <unistd.h>.  If it is undefined, one should ask at run
 *          time.  If it is defined to -1, then the option is not supported.
 *          If it is defined to 0, then relevant functions and headers exist,
 *          but one has to ask at run time what degree of support is available."
 *       glibc 2.39 sysdeps/unix/sysv/linux/bits/posix_opt.h:23/26/32/103/106/128/131/146/152/155/164/182-183/186/192
 *       glibc 2.39 posix/confstr.c:43（_CS_PATH）/ :252（_CS_GNU_LIBC_VERSION）
 *                                 / :257（_CS_GNU_LIBPTHREAD_VERSION）；无值时 :265/:277 return 0
 */
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void sc(const char *lbl, int name)
{
    errno = 0;
    long v = sysconf(name);
    const char *note = "";
    if (v == -1)
        note = (errno == 0) ? "  <- 不支持（或该 name 属限制类且 indeterminate）" : "  <- 真错误";
    printf("  %-28s = %-10ld%s\n", lbl, v, note);
}

int main(void)
{
    printf("### ① 编译期：_POSIX_* 的三态（本机 glibc 2.39）\n");

    printf("  -- 支持（值为 POSIX 修订年月）--\n");
    printf("  _POSIX_JOB_CONTROL=%ld _POSIX_SAVED_IDS=%ld _POSIX_THREADS=%ld\n",
           (long)_POSIX_JOB_CONTROL, (long)_POSIX_SAVED_IDS, (long)_POSIX_THREADS);
    printf("  _POSIX_REALTIME_SIGNALS=%ld _POSIX_TIMERS=%ld _POSIX_SEMAPHORES=%ld\n",
           (long)_POSIX_REALTIME_SIGNALS, (long)_POSIX_TIMERS, (long)_POSIX_SEMAPHORES);
    printf("  _POSIX_PRIORITY_SCHEDULING=%ld _POSIX_BARRIERS=%ld _POSIX_SPIN_LOCKS=%ld\n",
           (long)_POSIX_PRIORITY_SCHEDULING, (long)_POSIX_BARRIERS, (long)_POSIX_SPIN_LOCKS);

    printf("\n  -- 定义为 0（头文件在，但程度要说运行时）--\n");
    printf("  _POSIX_MONOTONIC_CLOCK=%ld  _POSIX_CPUTIME=%ld  _POSIX_THREAD_CPUTIME=%ld\n",
           (long)_POSIX_MONOTONIC_CLOCK, (long)_POSIX_CPUTIME, (long)_POSIX_THREAD_CPUTIME);

    printf("\n  -- 定义为 -1（明确不支持）--\n");
    printf("  _POSIX_SPORADIC_SERVER=%ld  _POSIX_THREAD_SPORADIC_SERVER=%ld\n",
           (long)_POSIX_SPORADIC_SERVER, (long)_POSIX_THREAD_SPORADIC_SERVER);
    printf("  _POSIX_TRACE=%ld  _POSIX_TRACE_EVENT_FILTER=%ld  _POSIX_TYPED_MEMORY_OBJECTS=%ld\n",
           (long)_POSIX_TRACE, (long)_POSIX_TRACE_EVENT_FILTER, (long)_POSIX_TYPED_MEMORY_OBJECTS);

    printf("\n### ② 运行时：同一批选项用 sysconf(_SC_*) 再问一遍\n");
    sc("_SC_JOB_CONTROL", _SC_JOB_CONTROL);
    sc("_SC_SAVED_IDS", _SC_SAVED_IDS);
    sc("_SC_THREADS", _SC_THREADS);
    sc("_SC_REALTIME_SIGNALS", _SC_REALTIME_SIGNALS);
    sc("_SC_TIMERS", _SC_TIMERS);
    sc("_SC_SEMAPHORES", _SC_SEMAPHORES);
    sc("_SC_MONOTONIC_CLOCK", _SC_MONOTONIC_CLOCK);
    sc("_SC_CPUTIME", _SC_CPUTIME);
    sc("_SC_THREAD_CPUTIME", _SC_THREAD_CPUTIME);
    sc("_SC_2_C_DEV", _SC_2_C_DEV);
    sc("_SC_2_FORT_DEV", _SC_2_FORT_DEV);
    sc("_SC_2_LOCALEDEF", _SC_2_LOCALEDEF);
    sc("_SC_XOPEN_VERSION", _SC_XOPEN_VERSION);

    printf("\n  对照要点：_POSIX_MONOTONIC_CLOCK=0（编译期「说不清」），而\n");
    printf("  sysconf(_SC_MONOTONIC_CLOCK) 给出 200809 —— 编译期含糊的，运行时才有答案。\n");

    printf("\n### ③ confstr()：查「字符串」选项（sysconf 的字面兄弟）\n");
    struct { int name; const char *lbl; } cs[] = {
        { _CS_PATH, "_CS_PATH" },
        { _CS_GNU_LIBC_VERSION, "_CS_GNU_LIBC_VERSION" },
        { _CS_GNU_LIBPTHREAD_VERSION, "_CS_GNU_LIBPTHREAD_VERSION" },
    };
    for (size_t i = 0; i < sizeof(cs) / sizeof(cs[0]); i++) {
        errno = 0;
        size_t need = confstr(cs[i].name, NULL, 0);      /* 先问需要多大 */
        if (need == 0) {
            printf("  %-28s 无值（errno=%d）\n", cs[i].lbl, errno);
            continue;
        }
        char *buf = malloc(need);
        size_t got = confstr(cs[i].name, buf, need);
        printf("  %-28s 需要 %2zu B，写入 %2zu B: \"%s\"\n", cs[i].lbl, need, got, buf);
        free(buf);
    }
    printf("  注意：与 sysconf 一样，confstr 没有值的时候返回 0（不是 -1），\n");
    printf("        且**不保证**设置 errno —— 用 (confstr(n,NULL,0)) 判空即可。\n");

    printf("\n### ④ 三态判断在代码里怎么写才不会踩坑\n");
    printf("  错的：int have = _POSIX_BARRIERS;        // 宏没定义时编不过\n");
    printf("  对的：#ifdef _POSIX_BARRIERS              // 先判有没有\n");
    printf("            #if _POSIX_BARRIERS > 0 ...      // 再判支不支持\n");
    printf("            #else ... runtime ...            // ==0 要问运行时\n");
    printf("            #endif\n");
    printf("        #else  ... runtime ...               // 未定义也要问运行时\n");
    printf("        #endif\n");
    printf("  本机实测：_POSIX_BARRIERS=%ld（正值），sysconf 也给 %ld，两边一致。\n",
           (long)_POSIX_BARRIERS, sysconf(_SC_BARRIERS));
    return 0;
}
