/* TLPI 第 03 章 §3.1 —— 同一次写操作的「三条路径」+ 失败分支 + SIGPIPE
 *
 *   ① glibc 包装函数      write(1, ...)            唯一推荐用法
 *   ② syscall(2) 通用入口  syscall(SYS_write, ...)  给「glibc 没给包装」的调用兜底
 *   ③ 手写 syscall 指令    寄存器约定：rax=调用号，rdi/rsi/rdx/r10/r8/r9=参数，
 *                          返回值也在 rax；内核的失败约定是 rax 落在 [-4095, -1]
 *
 * 本节最反直觉的两条：
 *   1. syscall(2) 这个「通用入口」本身就做了一层翻译 —— 它把内核的 -errno 变成
 *      -1 并把 errno 设上；手写指令拿到的是裸返回码 -9，errno 根本没人碰。
 *   2. 往「没有读端的管道」里写，默认不是收到一个错误码，而是**整个进程被
 *      SIGPIPE(13) 杀掉**。这是网络服务必须第一行就忽略它的原因。
 *
 * 编译：gcc -O0 -Wall -Wextra -o c3_1 c3_1_syscall_path.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

/* 内核把 [-4095, -1] 这一段保留给错误码。范围上界来自
 * include/linux/err.h 的 MAX_ERRNO = 4095 —— 拿到裸返回码时，
 * 落在这个区间里就意味着失败，-ret 就是 errno 的值。 */
#define KERNEL_ERR_MAX (-4095)

/* 把三种调用方式的返回值放在一起读：同一个错误，三种「报法」 */
static void report(const char *tag, long ret, int err)
{
    if (ret >= 0) {
        printf("  %-34s ret=%-6ld (成功)\n", tag, ret);
    } else if (ret == -1) {
        printf("  %-34s ret=-1      errno=%d (%s)   ← glibc 约定：-1 + errno\n",
               tag, err, strerror(err));
    } else if (ret >= KERNEL_ERR_MAX) {
        printf("  %-34s ret=%-6ld  ← 裸返回码：-ret = %d 才是 errno 该有的值，而 errno 仍是 %d\n",
               tag, ret, (int) -ret, err);
    } else {
        printf("  %-34s ret=%-6ld  ← 超出内核错误码区间 [-4095,-1]\n", tag, ret);
    }
}

/* ③ 手写 syscall 指令：只传 3 个参数，足够覆盖 write 这组实验。
 *    clobber 里必须写 rcx / r11 —— syscall 指令会把返回地址放 rcx、
 *    把 rflags 放 r11，编译器不知道，所以要显式告诉它。 */
static long raw_syscall3(long nr, long a1, long a2, long a3)
{
    long ret;
    __asm__ volatile ("syscall"
                      : "=a"(ret)
                      : "a"(nr), "D"(a1), "S"(a2), "d"(a3)
                      : "rcx", "r11", "memory");
    return ret;
}

/* ⑧ 往「读端已关闭的管道」写一次，看 SIGPIPE 的两种处置 */
static void sigpipe_child(sighandler_t how, const char *label)
{
    int p[2];
    if (pipe(p) != 0) {
        printf("  %-16s → pipe 创建失败\n", label);
        return;
    }
    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) {                       /* ---- 子进程 ---- */
        signal(SIGPIPE, how);
        close(p[0]);                      /* 本进程里把读端关掉 → 管道再无读者 */
        errno = 0;
        ssize_t w = write(p[1], "x", 1);
        printf("  %-16s → 子进程没被杀死：write 返回 %zd，errno=%d (%s)  EPIPE=%d\n",
               label, w, errno, strerror(errno), EPIPE);
        _exit(0);
    }
    close(p[0]);
    close(p[1]);
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        printf("  %-16s → 子进程被信号 %d 终止   （shell 里 $? = 128+%d = %d）\n",
               label, WTERMSIG(status), WTERMSIG(status), 128 + WTERMSIG(status));
    } else if (WIFEXITED(status)) {
        printf("  %-16s → 子进程 exit(%d)\n", label, WEXITSTATUS(status));
    }
}

int main(void)
{
    /* 容器里 stdout 是 socket → 默认全缓冲。混用 printf 与直接 write(1,...) 会乱序，
     * 这里显式关掉缓冲，让每一行的先后顺序真实可读。 */
    setvbuf(stdout, NULL, _IONBF, 0);

    const char *msg = "[3.1] hello from three paths\n";
    const long n = (long) strlen(msg);
    long ret;
    int err;

    printf("=== ① glibc 包装函数 write(2) ===\n");
    errno = 0;
    ret = (long) write(STDOUT_FILENO, msg, (size_t) n);
    err = errno;
    report("write(1, msg, n)", ret, err);

    printf("=== ② syscall(2) 通用入口 ===\n");
    errno = 0;
    ret = syscall(SYS_write, STDOUT_FILENO, msg, (size_t) n);
    err = errno;
    report("syscall(SYS_write, ...)", ret, err);

    printf("=== ③ 手写 syscall 指令 ===\n");
    errno = 0;
    ret = raw_syscall3(SYS_write, STDOUT_FILENO, (long) (uintptr_t) msg, n);
    err = errno;
    report("raw syscall 指令", ret, err);
    printf("\n");

    printf("=== ④ 失败分支 A：写一个「只读方式打开的 fd」===\n");
    /* pipe(2) 返回的 p[0] 只能读、p[1] 只能写。往 p[0] 里写，必然 EBADF(9)。
     * 这个靶子不需要文件系统配合，任何 UN*X 上都成立。 */
    int p[2];
    if (pipe(p) != 0) {
        printf("  pipe 创建失败\n");
        return 1;
    }
    errno = 0;
    ret = (long) write(p[0], msg, (size_t) n);
    err = errno;
    report("write(读端) [glibc]", ret, err);

    errno = 0;
    ret = syscall(SYS_write, p[0], msg, (size_t) n);
    err = errno;
    report("syscall(SYS_write, 读端)", ret, err);

    errno = 0;
    ret = raw_syscall3(SYS_write, p[0], (long) (uintptr_t) msg, n);
    err = errno;
    report("raw syscall 写读端", ret, err);

    /* 换一个「只读打开的普通文件」当靶子，证明不是 pipe 特有 */
    int rfd = open("/proc/self/status", O_RDONLY);
    if (rfd >= 0) {
        errno = 0;
        ret = syscall(SYS_write, rfd, msg, (size_t) n);
        err = errno;
        report("syscall 写只读普通文件", ret, err);
        close(rfd);
    } else {
        printf("  %-34s (跳过：/proc/self/status 打不开)\n", "只读普通文件靶子");
    }
    printf("    ↑ glibc 与 syscall(2) 都返回 -1 并把 errno 设成 9；\n"
           "      手写指令拿到的是 -9 —— 这就是「薄封装」与「裸接口」的分界\n\n");

    printf("=== ⑤ 失败分支 B：无效系统调用号 9999 ===\n");
    errno = 0;
    ret = syscall(9999);
    err = errno;
    report("syscall(9999)", ret, err);
    errno = 0;
    ret = raw_syscall3(9999, 0, 0, 0);
    err = errno;
    report("raw 9999", ret, err);
    printf("    ↑ 号超出 NR_syscalls，内核查表落到 __x64_sys_ni_syscall()，统一回 ENOSYS(38)\n\n");

    printf("=== ⑥ syscall(2) 真正的用武之地：glibc 没给包装的调用 ===\n");
    /* SYS_gettid 在 glibc 里没有等价函数（pthread_self 返回的是 pthread_t，
     * 不是内核 tid），只能靠 syscall(2)。 */
    errno = 0;
    ret = syscall(SYS_gettid);
    err = errno;
    printf("  %-34s ret=%-6ld (系统调用号 %d，无 glibc 包装)\n",
           "syscall(SYS_gettid)", ret, (int) SYS_gettid);
    printf("    ↑ 和上面 ④⑤ 的失败分支对照读：同一个 syscall(2) 入口，成功走 ret，失败走 errno\n\n");

    printf("=== ⑦ 号是怎么来的：arch/x86/entry/syscalls/syscall_64.tbl ===\n");
    printf("  SYS_read          = %d\n", (int) SYS_read);
    printf("  SYS_write         = %d\n", (int) SYS_write);
    printf("  SYS_openat        = %d\n", (int) SYS_openat);
    printf("  SYS_clock_gettime = %d\n", (int) SYS_clock_gettime);
    printf("  可见号的最大值 + 1 ≈ NR_syscalls 的量级 = %d（表里有大量空洞/已废弃号）\n\n",
           (int) SYS_clock_gettime + 1);

    printf("=== ⑧ 往「读端已关闭的管道」里写：默认不是错误码，是死刑 ===\n");
    sigpipe_child(SIG_DFL, "SIG_DFL（默认）");
    sigpipe_child(SIG_IGN, "SIG_IGN（忽略）");
    printf("    ↑ 网络服务为什么启动时就 signal(SIGPIPE, SIG_IGN)：\n"
           "      对端断开后自己再 write，默认行为是**整个进程被信号杀掉**，\n"
           "      而不是拿到一个能处理的错误码。本 demo 第一版就是直接在主进程里\n"
           "      写了一个只读 fd，结果进程被 13 号信号带走、exit code = 141，\n"
           "      后面 ⑤⑥⑦ 三段一行都没跑到 —— 现在把这段关进子进程才看得全。\n"
           "    注：SIG_IGN 会被 fork 继承，所以上面第一个子进程必须显式\n"
           "        signal(SIGPIPE, SIG_DFL) 才能还原「默认处置」。\n");
    return 0;
}
