/* c11_2_sysconf_table.c — Ch11 §11.2：sysconf() 全表 + 返回 -1 的三种含义
 *
 * 三件事：
 *   ① 把 sysconf 的四类 name 全查一遍（常量类 / 限制类 / 选项类 / 扩展类），
 *      每行带上「调用后 errno」——因为判断 -1 的唯一依据就是它。
 *   ② 把「-1 三义」逐个抓到现场：
 *        a. 真错误              -> errno=EINVAL（name 非法）
 *        b. 限制不确定(indeterminate) -> 返回 -1 且 errno **不变**
 *        c. 选项不支持          -> 也返回 -1，errno 也 **不变**（和 b 不可区分！）
 *   ③ 演示不重置 errno 会怎样误判：先制造 errno=22，再查一个 indeterminate，
 *      就会把一个「没问题的 -1」当成错误。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c11_2_sysconf_table c11_2_sysconf_table.c
 * 取材： sysconf(3) RETURN VALUE 段（man-pages 6.19）逐字：
 *         "If name corresponds to a maximum or minimum limit, and that limit is
 *          indeterminate, -1 is returned and errno is not changed."
 *       glibc 2.39 sysdeps/posix/sysconf.c:64-66（default: __set_errno(EINVAL); return -1;）
 *       glibc 2.39 sysdeps/posix/sysconf.c:102-103（_SC_TZNAME_MAX 直接 return -1）
 *       glibc 2.39 sysdeps/unix/sysv/linux/sysconf.c:56-67（_SC_ARG_MAX 的算法）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* errno==0 且返回 -1 => indeterminate；errno!=0 且返回 -1 => 真错误 */
static void sc(const char *lbl, int name)
{
    errno = 0;
    long v = sysconf(name);
    const char *tag = "";
    if (v == -1)
        tag = (errno == 0) ? "  <- 非错误：indeterminate / 选项不支持"
                           : "  <- 真错误";
    printf("  %-26s = %-12ld errno=%-3d%s\n", lbl, v, errno, tag);
}

static const struct { int name; const char *lbl; } CONSTS[] = {
    { _SC_VERSION,     "_SC_VERSION" },
    { _SC_2_VERSION,   "_SC_2_VERSION" },
    { _SC_CLK_TCK,     "_SC_CLK_TCK" },
    { _SC_PAGESIZE,    "_SC_PAGESIZE" },
    { _SC_PAGE_SIZE,   "_SC_PAGE_SIZE" },
    { _SC_CHAR_BIT,    "_SC_CHAR_BIT" },
    { _SC_INT_MAX,     "_SC_INT_MAX" },
    { _SC_LONG_BIT,    "_SC_LONG_BIT" },
    { _SC_WORD_BIT,    "_SC_WORD_BIT" },
    { _SC_MB_LEN_MAX,  "_SC_MB_LEN_MAX" },
    { _SC_NZERO,       "_SC_NZERO" },
    { _SC_SSIZE_MAX,   "_SC_SSIZE_MAX" },
    { _SC_ATEXIT_MAX,  "_SC_ATEXIT_MAX" },
    { _SC_PASS_MAX,    "_SC_PASS_MAX" },
};

static const struct { int name; const char *lbl; } LIMITS[] = {
    { _SC_ARG_MAX,         "_SC_ARG_MAX" },
    { _SC_CHILD_MAX,       "_SC_CHILD_MAX" },
    { _SC_OPEN_MAX,        "_SC_OPEN_MAX" },
    { _SC_STREAM_MAX,      "_SC_STREAM_MAX" },
    { _SC_NGROUPS_MAX,     "_SC_NGROUPS_MAX" },
    { _SC_LOGIN_NAME_MAX,  "_SC_LOGIN_NAME_MAX" },
    { _SC_HOST_NAME_MAX,   "_SC_HOST_NAME_MAX" },
    { _SC_TTY_NAME_MAX,    "_SC_TTY_NAME_MAX" },
    { _SC_TZNAME_MAX,      "_SC_TZNAME_MAX" },
    { _SC_SYMLOOP_MAX,     "_SC_SYMLOOP_MAX" },
    { _SC_RE_DUP_MAX,      "_SC_RE_DUP_MAX" },
    { _SC_IOV_MAX,         "_SC_IOV_MAX" },
    { _SC_SIGQUEUE_MAX,    "_SC_SIGQUEUE_MAX" },
    { _SC_MQ_OPEN_MAX,     "_SC_MQ_OPEN_MAX" },
    { _SC_MQ_PRIO_MAX,     "_SC_MQ_PRIO_MAX" },
    { _SC_DELAYTIMER_MAX,  "_SC_DELAYTIMER_MAX" },
    { _SC_SEM_NSEMS_MAX,   "_SC_SEM_NSEMS_MAX" },
    { _SC_SEM_VALUE_MAX,   "_SC_SEM_VALUE_MAX" },
};

static const struct { int name; const char *lbl; } OPTS[] = {
    { _SC_JOB_CONTROL,            "_SC_JOB_CONTROL" },
    { _SC_SAVED_IDS,              "_SC_SAVED_IDS" },
    { _SC_THREADS,                "_SC_THREADS" },
    { _SC_REALTIME_SIGNALS,       "_SC_REALTIME_SIGNALS" },
    { _SC_PRIORITY_SCHEDULING,    "_SC_PRIORITY_SCHEDULING" },
    { _SC_TIMERS,                 "_SC_TIMERS" },
    { _SC_SEMAPHORES,             "_SC_SEMAPHORES" },
    { _SC_SHARED_MEMORY_OBJECTS,  "_SC_SHARED_MEMORY_OBJECTS" },
    { _SC_MONOTONIC_CLOCK,        "_SC_MONOTONIC_CLOCK" },
    { _SC_CPUTIME,                "_SC_CPUTIME" },
    { _SC_THREAD_CPUTIME,         "_SC_THREAD_CPUTIME" },
    { _SC_BARRIERS,               "_SC_BARRIERS" },
    { _SC_READER_WRITER_LOCKS,    "_SC_READER_WRITER_LOCKS" },
    { _SC_2_C_DEV,                "_SC_2_C_DEV" },
    { _SC_2_FORT_DEV,             "_SC_2_FORT_DEV" },
    { _SC_2_LOCALEDEF,            "_SC_2_LOCALEDEF" },
    { _SC_2_SW_DEV,               "_SC_2_SW_DEV" },
    { _SC_XOPEN_VERSION,          "_SC_XOPEN_VERSION" },
};

static const struct { int name; const char *lbl; } EXTRAS[] = {
    { _SC_PHYS_PAGES,         "_SC_PHYS_PAGES" },
    { _SC_AVPHYS_PAGES,       "_SC_AVPHYS_PAGES" },
    { _SC_NPROCESSORS_CONF,   "_SC_NPROCESSORS_CONF" },
    { _SC_NPROCESSORS_ONLN,   "_SC_NPROCESSORS_ONLN" },
    { _SC_MINSIGSTKSZ,        "_SC_MINSIGSTKSZ" },
    { _SC_SIGSTKSZ,           "_SC_SIGSTKSZ" },
};

#define DUMP(arr)                                                       \
    do {                                                                \
        for (size_t i = 0; i < sizeof(arr) / sizeof((arr)[0]); i++)     \
            sc((arr)[i].lbl, (arr)[i].name);                            \
    } while (0)

int main(void)
{
    printf("### ① 常量类（与 CPU/内核无关的确定值）\n");
    DUMP(CONSTS);

    printf("\n### ② 限制类\n");
    DUMP(LIMITS);

    printf("\n### ③ 选项类（支持则返回一个 >0 的值，不支持则 -1）\n");
    DUMP(OPTS);

    printf("\n### ④ glibc/Linux 扩展\n");
    DUMP(EXTRAS);

    printf("\n### ⑤ 返回 -1 的三种含义，逐个抓现场\n");

    errno = 0;
    long v = sysconf(9999);
    printf("  a) sysconf(9999)          -> %ld errno=%d (%s)\n", v, errno, strerror(errno));
    printf("     name 非法 = 真错误。glibc 走的是 sysdeps/posix/sysconf.c:64 的 default 分支。\n");

    errno = 0;
    v = sysconf(_SC_TZNAME_MAX);
    printf("  b) sysconf(_SC_TZNAME_MAX) -> %ld errno=%d   <- indeterminate，不是错误\n", v, errno);
    printf("     glibc 里就是一行 `case _SC_TZNAME_MAX: return -1;`（sysconf.c:102-103），\n");
    printf("     压根不碰 errno，所以调用者必须自己在调用前把 errno 清零。\n");

    errno = 0;
    v = sysconf(_SC_2_FORT_DEV);
    printf("  c) sysconf(_SC_2_FORT_DEV) -> %ld errno=%d   <- 选项不支持，也是 -1\n", v, errno);
    printf("     和 b) 在「返回值 + errno」上完全一样 —— 靠返回值无法区分，\n");
    printf("     只能靠你知道这个 name 问的是「限制」还是「选项」。\n");

    printf("\n### ⑥ 不重置 errno 的经典误判\n");
    errno = 0;
    (void) sysconf(9999);                       /* 先把 errno 弄成 22，之后故意不清零 */
    printf("  上一步留下了 errno=%d (%s)\n", errno, strerror(errno));
    long w = sysconf(_SC_TZNAME_MAX);           /* 一个完全正常的 indeterminate */
    printf("  紧接着查 _SC_TZNAME_MAX -> %ld，errno=%d\n", w, errno);
    printf("  如果按「-1 && errno != 0 就是错误」去判，这里会误报错误。\n");
    printf("  正确写法：**每次调用之前** errno = 0，而不是整个函数开头清一次。\n");

    printf("\n### ⑦ 值对不代表 errno 干净\n");
    errno = 0;
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    printf("  sysconf(_SC_NPROCESSORS_ONLN) = %ld，但 errno 被留成 %d (%s)\n",
           n, errno, strerror(errno));
    printf("  （glibc 的 __get_nprocs 读 /sys 时留下的 ENOENT，值本身是对的。）\n");
    return 0;
}
