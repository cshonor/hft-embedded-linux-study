/* c11_4_indeterminate.c — Ch11 §11.4：indeterminate 限制与「查不到怎么办」
 *
 * §11.4 讲的是这样一类限制：标准允许系统**不告诉你确切值**。此时
 * sysconf/pathconf 返回 -1 且 errno 不变。问题在于：
 *
 *   ① 哪些 name 会 indeterminate？（本机实测清单）
 *   ② 一个安全的封装长什么样？
 *   ③ 查不到时程序该退到哪一级？（降级阶梯：sysconf -> pathconf -> 编译期宏 -> 保守常量）
 *
 * 编译： gcc -O0 -Wall -Wextra -o c11_4_indeterminate c11_4_indeterminate.c
 * 取材： sysconf(3) / fpathconf(3) RETURN VALUE 段（man-pages 6.19）；
 *       glibc 2.39 sysdeps/posix/sysconf.c:102-103（_SC_TZNAME_MAX 恒 -1）
 *       glibc 2.39 sysdeps/posix/pathconf.c:44-62（LINK_MAX/MAX_CANON/MAX_INPUT 可 -1）
 *       POSIX.1-2017 <limits.h>：_POSIX_* 是「保证下限」，实现可以更大
 */
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---------- ① 扫一遍，只打印真的 indeterminate 的 ---------- */
static void sweep(void)
{
    static const struct { int name; const char *lbl; } N[] = {
        { _SC_ARG_MAX, "_SC_ARG_MAX" }, { _SC_CHILD_MAX, "_SC_CHILD_MAX" },
        { _SC_OPEN_MAX, "_SC_OPEN_MAX" }, { _SC_STREAM_MAX, "_SC_STREAM_MAX" },
        { _SC_NGROUPS_MAX, "_SC_NGROUPS_MAX" }, { _SC_HOST_NAME_MAX, "_SC_HOST_NAME_MAX" },
        { _SC_LOGIN_NAME_MAX, "_SC_LOGIN_NAME_MAX" }, { _SC_TTY_NAME_MAX, "_SC_TTY_NAME_MAX" },
        { _SC_TZNAME_MAX, "_SC_TZNAME_MAX" }, { _SC_SYMLOOP_MAX, "_SC_SYMLOOP_MAX" },
        { _SC_RE_DUP_MAX, "_SC_RE_DUP_MAX" }, { _SC_IOV_MAX, "_SC_IOV_MAX" },
        { _SC_MQ_OPEN_MAX, "_SC_MQ_OPEN_MAX" }, { _SC_MQ_PRIO_MAX, "_SC_MQ_PRIO_MAX" },
        { _SC_SEM_NSEMS_MAX, "_SC_SEM_NSEMS_MAX" }, { _SC_SEM_VALUE_MAX, "_SC_SEM_VALUE_MAX" },
        { _SC_DELAYTIMER_MAX, "_SC_DELAYTIMER_MAX" }, { _SC_SIGQUEUE_MAX, "_SC_SIGQUEUE_MAX" },
    };
    int n = 0;
    printf("### ① 本机的 indeterminate 清单（-1 且 errno==0）\n");
    for (size_t i = 0; i < sizeof(N) / sizeof(N[0]); i++) {
        errno = 0;
        long v = sysconf(N[i].name);
        if (v == -1 && errno == 0) {
            printf("  %-22s = -1  (indeterminate)\n", N[i].lbl);
            n++;
        }
    }
    printf("  共 %d 项 indeterminate。它们不是「不支持」，而是「这个实现不回答」。\n", n);

    static const struct { int name; const char *lbl; } P[] = {
        { _PC_NAME_MAX, "_PC_NAME_MAX" }, { _PC_LINK_MAX, "_PC_LINK_MAX" },
        { _PC_PATH_MAX, "_PC_PATH_MAX" }, { _PC_PIPE_BUF, "_PC_PIPE_BUF" },
        { _PC_MAX_CANON, "_PC_MAX_CANON" }, { _PC_MAX_INPUT, "_PC_MAX_INPUT" },
        { _PC_ASYNC_IO, "_PC_ASYNC_IO" }, { _PC_SYNC_IO, "_PC_SYNC_IO" },
        { _PC_PRIO_IO, "_PC_PRIO_IO" }, { _PC_FILESIZEBITS, "_PC_FILESIZEBITS" },
        { _PC_2_SYMLINKS, "_PC_2_SYMLINKS" },
    };
    printf("\n  pathconf(\"/\") 侧：\n");
    for (size_t i = 0; i < sizeof(P) / sizeof(P[0]); i++) {
        errno = 0;
        long v = pathconf("/", P[i].name);
        if (v == -1 && errno == 0)
            printf("  %-22s = -1  (indeterminate)\n", P[i].lbl);
    }
    printf("  （_PC_ASYNC_IO/_PC_SYNC_IO/_PC_PRIO_IO 在 glibc 里只在「问了普通文件」时才有答案，\n");
    printf("    问目录一律返回 -1。见 sysdeps/posix/pathconf.c:128-148 的 S_ISREG 判断。）\n");
}

/* ---------- ② 安全封装：把「限制值」和「状态」分开 ---------- */
enum qstat { Q_OK, Q_INDETERMINATE, Q_ERROR };

static long query_sysconf(int name, enum qstat *st)
{
    errno = 0;
    long v = sysconf(name);
    if (v != -1) { *st = Q_OK; return v; }
    if (errno == 0) { *st = Q_INDETERMINATE; return -1; }
    *st = Q_ERROR;
    return -1;
}

static const char *qname(enum qstat s)
{
    switch (s) {
    case Q_OK:            return "OK";
    case Q_INDETERMINATE: return "INDETERMINATE";
    default:              return "ERROR";
    }
}

/* ---------- ③ 降级阶梯：查不到时退到哪一级 ---------- */

/* 目标：拿一个「安全的文件名长度上限」。三级退让：
 *   一级  sysconf(_SC_NAME_MAX)? 不存在 —— NAME_MAX 是路径相关的，只能 pathconf
 *   二级  pathconf(path, _PC_NAME_MAX)
 *   三级  编译期 NAME_MAX（<limits.h>，本机 255）—— 但它也可能没定义
 *   四级  _POSIX_NAME_MAX（保证下限，永远有）—— 但可能小到没法用（14）
 */
static long name_max_ladder(const char *path, const char **how)
{
    if (path) {
        errno = 0;
        long v = pathconf(path, _PC_NAME_MAX);
        if (v != -1) { *how = "pathconf(_PC_NAME_MAX)"; return v; }
    }
#ifdef NAME_MAX
    *how = "<limits.h> NAME_MAX";
    return NAME_MAX;
#else
    *how = "_POSIX_NAME_MAX（保证下限）";
    return _POSIX_NAME_MAX;
#endif
}

int main(void)
{
    sweep();

    printf("\n### ② 安全封装的三种结果\n");
    static const struct { int name; const char *lbl; } T[] = {
        { _SC_OPEN_MAX, "_SC_OPEN_MAX" },
        { _SC_TZNAME_MAX, "_SC_TZNAME_MAX" },
        { 9999, "9999（非法 name）" },
    };
    for (size_t i = 0; i < sizeof(T) / sizeof(T[0]); i++) {
        enum qstat st;
        long v = query_sysconf(T[i].name, &st);
        printf("  %-22s -> status=%-14s value=%ld\n", T[i].lbl, qname(st), v);
    }

    printf("\n### ③ 降级阶梯：要一个文件名长度上限\n");
    const char *how = "?";
    long nm = name_max_ladder("/tmp", &how);
    printf("  pathconf(\"/tmp\") 成功 -> %ld  （来源：%s）\n", nm, how);
    how = "?";
    nm = name_max_ladder("/no/such/dir", &how);
    printf("  pathconf(\"/no/such/dir\") 失败 -> %ld （来源：%s）\n", nm, how);
    printf("  阶梯的含义：能用运行时查询就用；查不到就退到编译期宏；宏也没有就退到\n");
    printf("  POSIX 保证下限（本机 _POSIX_NAME_MAX=%d），并按这个值把缓冲区放大一点。\n",
           _POSIX_NAME_MAX);

    printf("\n### ④ 两个容易踩的「假 indeterminate」\n");
    errno = 0;
    long x = pathconf("/", _PC_MAX_CANON);
    printf("  pathconf(\"/\", _PC_MAX_CANON) = %ld, errno=%d  —— 问的不是终端，语义上无意义\n",
           x, errno);
    errno = 0;
    long y = pathconf("/", _PC_VDISABLE);
    printf("  pathconf(\"/\", _PC_VDISABLE)  = %ld, errno=%d  —— glibc 直接返回 _POSIX_VDISABLE，\n",
           y, errno);
    printf("     连「这是不是终端」都不检查（sysdeps/posix/pathconf.c:115-119）。\n");
    printf("  这两类「看起来有值、其实没意义」的结果，比 -1 更危险。\n");
    return 0;
}
