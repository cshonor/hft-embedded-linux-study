/* TLPI 第 03 章 §3.5.2 —— 原书 lib/error_functions.[hc]（Listing 3-2 / 3-3）
 * 的六个函数各自解决什么，以及它为什么要自带一张 errno 名字表（ename.c.inc）
 *
 * 六个函数的语义差别（照原书）：
 *   errExit(msg)            打印 "prog: msg: strerror(errno)" 到 stderr，exit(EXIT_FAILURE)
 *   err_exit(msg)           同上，但用 _exit()（不 flush stdio、不跑 atexit）
 *   errExitEN(errnum, msg)  用**显式**给定的 errno 号，而不是当前 errno
 *   fatal(msg)              打印 "prog: msg"（**不带** errno），exit
 *   usageErr(fmt, ...)      打印 "Usage: ..." 到 stderr，exit(EXIT_FAILURE)
 *   cmdLineErr(fmt, ...)    打印 "Command-line usage error: ..." 到 stderr，exit
 *
 * 难点：这些函数**都会把进程结束掉**，所以不能「依次调用看输出」。
 * 本 demo 用 fork + 管道把子进程的 stdout/stderr 都抓回来，于是每个函数的真实
 * 输出和真实退出码都能并排打印。
 *
 * 这里还顺带踩了两个真实的坑（都做成了独立段落）：
 *   坑 1  父进程从管道里「只 read 一次就 close」→ 子进程后续的 write 收到
 *         SIGPIPE(13) 直接被杀死，抓回来的输出全是残句。正确姿势是读到 EOF。
 *         demo 的第一版就是这么挂的：七个函数里六个变成「被信号 13 终止」，
 *         只有 fatal 侥幸活下来，整张表失去意义。
 *   坑 2  err_exit 用 _exit()，不 flush stdio → 崩溃路径上写进 stdout 缓冲的
 *         日志会**直接蒸发**。
 *
 * 编译：gcc -O0 -Wall -Wextra -o c3_8 c3_8_error_functions.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static const char *progname = "c3_8";

/* ---------- 原书 error_functions.h 的精简复刻 ---------- */

static void fatal(const char *fmt, ...)
{
    va_list ap;
    fflush(stdout);
    fprintf(stderr, "%s: ", progname);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

static void errExit(const char *fmt, ...)
{
    va_list ap;
    int err = errno;                 /* 先存下来：后面 fprintf 自己可能改 errno */
    fflush(stdout);
    fprintf(stderr, "%s: ", progname);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(err));
    exit(EXIT_FAILURE);
}

/* 原书的 errExitEN：显式指定 errno 号，用于「错误来自函数返回值而非 errno」的场合 */
static void errExitEN(int errnum, const char *fmt, ...)
{
    va_list ap;
    fflush(stdout);
    fprintf(stderr, "%s: ", progname);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errnum));
    exit(EXIT_FAILURE);
}

static void err_exit(const char *fmt, ...)
{
    va_list ap;
    int err = errno;
    fprintf(stderr, "%s: ", progname);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(err));
    _exit(EXIT_FAILURE);             /* 与 errExit 的唯一区别：不跑 atexit、不 flush */
}

static void usageErr(const char *fmt, ...)
{
    va_list ap;
    fflush(stdout);
    fprintf(stderr, "Usage: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

static void cmdLineErr(const char *fmt, ...)
{
    va_list ap;
    fflush(stdout);
    fprintf(stderr, "Command-line usage error: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

/* ---------- 把「会自杀的函数」关进子进程，抓回它的输出与退出码 ---------- */

typedef void (*errfn)(void);

static void c_msg_enoent(void) { errno = ENOENT; errExit("open"); }
static void c_msg_eacces(void) { errno = EACCES; errExit("open %s", "/etc/shadow"); }
static void c_en(void)         { errExitEN(ENOSYS, "pthread_style_call"); }
static void c_fatal(void)      { fatal("unexpected state: counter went negative"); }
static void c_usage(void)      { usageErr("%s [-a] [-b value] file...", progname); }
static void c_cmdline(void)    { cmdLineErr("option '-b' requires an argument"); }
static void c_err_exit(void)   { errno = ENOSPC; err_exit("write"); }

/* 这两个用来对比 exit() 与 _exit() 对 stdio 缓冲的不同态度：
 * 都先往 stdout 写一段标记，但子进程的 stdout 已被重定向到**管道** → 全缓冲，
 * 不 flush 就看不到。
 * 注意这里**不能**用 setvbuf 去改缓冲模式：流已经被用过，此时再 setvbuf 是 UB，
 * glibc 直接当没听见。本 demo 的正确做法是反过来 —— 全局**不**给 stdout 设
 * _IONBF（见 main 顶部注释），让它在「非 tty」的默认判定下就是全缓冲。 */
static void c_flush_via_errExit(void)
{
    fputs("MARKER-from-stdout-buffer\n", stdout);
    errno = ENOENT;
    errExit("flush-demo");           /* 里面那句 fflush(stdout) 会把它冲出来 */
}

static void c_flush_via_err_exit(void)
{
    fputs("MARKER-from-stdout-buffer\n", stdout);
    errno = ENOENT;
    err_exit("flush-demo");          /* 没有 fflush，_exit 直接丢缓冲 */
}

/* 子进程里跑 fn（它不会返回），父进程把它的 stdout+stderr 一起捞回来。
 * ⚠️ 关键：必须一直 read 到返回 0（EOF）才能 close —— EOF 意味着子进程
 *    那一端的 fd 已全部关闭。中途 close 会让还在写的子进程收到 SIGPIPE。 */
static void capture(const char *tag, void (*fn)(void))
{
    int fds[2];
    if (pipe(fds) != 0) {
        printf("  [%s] pipe 失败\n", tag);
        return;
    }
    fflush(NULL);                    /* fork 前清缓冲，否则子进程会重复输出一遍 */

    pid_t pid = fork();
    if (pid == 0) {                  /* ---- 子进程 ---- */
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        dup2(fds[1], STDERR_FILENO);
        close(fds[1]);
        fn();                        /* 这里不会返回 */
        _exit(127);
    }
    /* ---- 父进程 ---- */
    close(fds[1]);
    char buf[512];
    size_t got = 0;
    for (;;) {
        char chunk[256];
        ssize_t n = read(fds[0], chunk, sizeof chunk);
        if (n <= 0) {                /* n == 0 就是 EOF：可以安全关闭了 */
            break;
        }
        size_t room = sizeof buf - 1 - got;
        size_t take = (size_t) n < room ? (size_t) n : room;
        memcpy(buf + got, chunk, take);
        got += take;
    }
    close(fds[0]);
    buf[got] = '\0';
    for (size_t i = 0; i < got; i++) {   /* 多行也排成一条记录，方便并排看 */
        if (buf[i] == '\n') {
            buf[i] = ' ';
        }
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        printf("  %-12s exit(%d)  输出: %s\n", tag, WEXITSTATUS(status), buf);
    } else if (WIFSIGNALED(status)) {
        printf("  %-12s 被信号 %d 终止  输出: %s\n", tag, WTERMSIG(status), buf);
    }
}

/* ---------- 附：读端到底什么时候能关 ---------- */

static void pipe_close_order(void)
{
    printf("=== 附：抓子进程输出时，「管道读端什么时候能关」===\n");
    /* 子进程故意写 25×8192 = 204800 字节，远超管道容量（64 KiB）→ 必然写到阻塞，
     * 于是「提前 close」这一方的错误必定暴露，不依赖时序运气。 */
    for (int mode = 0; mode < 2; mode++) {
        int fds[2];
        if (pipe(fds) != 0) {
            return;
        }
        fflush(NULL);
        pid_t pid = fork();
        if (pid == 0) {
            close(fds[0]);
            dup2(fds[1], STDERR_FILENO);
            close(fds[1]);
            char blk[8192];
            memset(blk, 'x', sizeof blk);
            for (int i = 0; i < 25; i++) {
                if (write(STDERR_FILENO, blk, sizeof blk) < 0) {
                    break;           /* 只有在 SIGPIPE 被忽略时才会走到这里 */
                }
            }
            _exit(1);
        }
        close(fds[1]);
        char blk[8192];
        size_t got = 0;
        if (mode == 0) {
            for (;;) {                       /* 正确：读到 EOF */
                ssize_t n = read(fds[0], blk, sizeof blk);
                if (n <= 0) {
                    break;
                }
                got += (size_t) n;
            }
        } else {
            ssize_t n = read(fds[0], blk, sizeof blk);   /* 错误：只读一次就关 */
            got = n > 0 ? (size_t) n : 0;
        }
        close(fds[0]);
        int status = 0;
        waitpid(pid, &status, 0);
        const char *name = mode == 0 ? "读到 EOF 再关" : "读一次就关";
        if (WIFEXITED(status)) {
            printf("  %-14s → 子进程 exit(%d)，抓到 %zu 字节\n", name, WEXITSTATUS(status), got);
        } else if (WIFSIGNALED(status)) {
            printf("  %-14s → 子进程被信号 %d 终止，只抓到 %zu 字节\n", name,
                   WTERMSIG(status), got);
        }
    }
    printf("    教训：只要读端一关，还在写的子进程立刻收到 SIGPIPE(13)，默认处置是\n"
           "          终止进程 —— 于是你抓回来的是半句话，而且看不出任何报错。\n"
           "          正确姿势：read() 返回 0（EOF）才说明子进程那一端已经关干净，\n"
           "          此时再 close 是安全的。\n"
           "          同一个道理在服务端更致命：对端断开后自己再 write → SIGPIPE\n"
           "          把服务进程写死，所以启动时该 signal(SIGPIPE, SIG_IGN)。\n\n");
}

/* ---------- ename.c.inc 的意义：strerror 覆盖不到时怎么办 ---------- */

static void strerror_coverage(void)
{
    int unknown = 0, total = 0;
    printf("=== 扫 errno 1..200，看 strerror 的覆盖率 ===\n");
    for (int e = 1; e <= 200; e++) {
        const char *s = strerror(e);
        total++;
        if (strncmp(s, "Unknown error", 13) == 0) {
            unknown++;
        } else if (e <= 12 || e == 38 || e == 95 || e == 110 || e == 111) {
            printf("  errno %3d → %s\n", e, s);
        }
    }
    printf("  ...（1..200 中认得 = %d/%d，返回 \"Unknown error\" 的 = %d）\n\n",
           total - unknown, total, unknown);
    printf("  原书为什么要自带 ename.c.inc：\n");
    printf("    1. 有些平台的 strerror() 只覆盖极少数 errno，其余返回 \"Unknown error N\"，\n"
           "       而原书要让示例在任何 UN*X 上都打印出像样的名字，所以自带一张表；\n"
           "    2. 那张表是按 errno 值直接建索引的静态数组，取名字比 strerror 更快\n"
           "       （没有 locale 查询、没有函数调用）—— HFT 的错误日志路径里这点很实际；\n"
           "    3. 它用 #ifdef 把本平台不存在的 errno 整块剪掉，所以同一份表能跨平台编译。\n\n");
}

int main(void)
{
    /* 这里**故意不**调用 setvbuf(stdout, NULL, _IONBF, 0)。
     * 原因有两层：
     *   1. 本容器里 stdout 是 socket，不是 tty → 默认全缓冲，正好用来演示
     *      「缓冲没冲刷就 _exit() 会丢日志」；
     *   2. 每次 fork 之前都会 fflush(NULL)，所以缓冲里不会有残留被复制成两份。
     * 在别处写 demo 时若为了行序好看而设 _IONBF，这个「丢日志」现象就消失了。 */
    strerror_coverage();

    printf("=== 六个函数的真实输出与退出码（各关在一个子进程里）===\n");
    capture("errExit", c_msg_enoent);
    capture("errExit+fmt", c_msg_eacces);
    capture("errExitEN", c_en);
    capture("fatal", c_fatal);
    capture("usageErr", c_usage);
    capture("cmdLineErr", c_cmdline);
    capture("err_exit", c_err_exit);

    printf("\n=== err_exit 的 _exit() 到底少做了什么（同一个标记，两种结局）===\n");
    capture("errExit", c_flush_via_errExit);
    capture("err_exit", c_flush_via_err_exit);
    printf("    ↑ 两行都把 \"MARKER-from-stdout-buffer\" 写进了 stdout 缓冲；\n"
           "      errExit 里有 fflush(stdout) → 标记出现；\n"
           "      err_exit 走 _exit() → 缓冲里的字节随进程一起消失。\n"
           "      实战含义：崩溃路径上想留下最后一句话，就别用 _exit()，\n"
           "      或者自己先 fflush(NULL)。\n\n");

    pipe_close_order();

    printf("=== 读表要点 ===\n");
    printf("  · errExit / err_exit 多打一段 \": 错误描述\"，fatal / usageErr / cmdLineErr 不打\n");
    printf("  · 全部用 EXIT_FAILURE(=1) 退出，不是 0 —— 脚本才能用 $? 判失败\n");
    printf("  · 全部写 **stderr** 而不是 stdout，所以重定向 stdout 也不会丢错误\n");
    printf("  · err_exit 的 _exit() 跳过 atexit 与 stdio flush —— 想「立刻死、别做收尾」时用它，\n"
           "    但代价是缓冲里的日志一起蒸发\n");
    printf("  · errExitEN 的存在理由：pthread_* 这类 API 的失败码是**返回值**，errno 根本没动，\n"
           "    拿 errExit 只会报出一个无关的错误原因\n");
    return 0;
}
