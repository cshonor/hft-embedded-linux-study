/* TLPI 第 2 章 §2.11 —— 信号：异步通知、屏蔽与挂起、EINTR
 *
 * 编译：gcc -O0 -Wall -Wextra c2_11_signal.c -o c2_11
 * 运行：./c2_11        （约 0.15 秒）
 *
 * 本节要钉死的事实：
 *   ① 信号是「软件中断」：内核在合适的时机把控制流抢过来，跳到 handler。
 *   ② 用 sigaction() 而不是 signal()：语义明确、可移植。
 *   ③ 屏蔽（block）不等于忽略（ignore）：被屏蔽的信号进入 pending，解除屏蔽后才执行。
 *   ④ handler 里只能调「异步信号安全」函数 —— write() 安全，printf() 不安全。
 *   ⑤ 没有 SA_RESTART 时，慢系统调用被信号打断会返回 -1 / EINTR，调用方必须重试。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>

static volatile sig_atomic_t g_count;
static volatile sig_atomic_t g_last;

/* handler 里只用 write()：它是异步信号安全的；printf() 不是 */
static void handler(int sig)
{
    g_count++;
    g_last = sig;
    const char *m = "  [handler] 在信号处理器里跑，只调用 write()\n";
    if (write(STDOUT_FILENO, m, strlen(m)) < 0) { /* 忽略 */ }
}

static void on_alarm(int sig) { (void)sig; }

static void on_chld(int sig)
{
    (void)sig;
    const char *m = "  [handler] 收到 SIGCHLD：有子进程状态变了\n";
    if (write(STDOUT_FILENO, m, strlen(m)) < 0) { /* 忽略 */ }
}

/* 安装 handler 的样板：sigaction + sigemptyset + flags */
static int install(int sig, void (*fn)(int), int flags)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = fn;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = flags;
    return sigaction(sig, &sa, NULL);
}

int main(void)
{
    printf("=== ① sigaction 注册，然后 kill 自己 ===\n");
    if (install(SIGUSR1, handler, SA_RESTART) < 0) { perror("sigaction"); return 1; }
    printf("  已注册 SIGUSR1(%d) -> handler\n", SIGUSR1);
    fflush(NULL);
    if (kill(getpid(), SIGUSR1) < 0) perror("kill");
    printf("  handler 调用次数 = %d，最后一次 sig = %d\n", (int)g_count, (int)g_last);
    printf("  -> 信号是异步的：kill() 立刻返回，handler 在内核选定的时机跑。\n");

    printf("\n=== ② 屏蔽 ≠ 忽略：屏蔽会进 pending，解除后立刻执行 ===\n");
    if (install(SIGUSR2, handler, SA_RESTART) < 0) { perror("sigaction"); return 1; }

    sigset_t block, pend;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR2);
    if (sigprocmask(SIG_BLOCK, &block, NULL) < 0) perror("sigprocmask");
    printf("  已屏蔽 SIGUSR2(%d)，handler 次数停在 %d\n", SIGUSR2, (int)g_count);

    fflush(NULL);
    kill(getpid(), SIGUSR2);
    printf("  发了 SIGUSR2，但被屏蔽 → handler 没跑（次数仍 = %d）\n", (int)g_count);

    if (sigpending(&pend) == 0)
        printf("  sigpending(): SIGUSR2 在挂起集合里？%s\n",
               sigismember(&pend, SIGUSR2) ? "YES" : "no");

    printf("  解除屏蔽 ...\n");
    fflush(NULL);
    sigprocmask(SIG_UNBLOCK, &block, NULL);      /* 一解除，pending 的信号立刻送达 */
    printf("  handler 次数 = %d，最后一次 sig = %d\n", (int)g_count, (int)g_last);
    printf("  -> 信号没丢，只是被「扣押」了。SIG_IGN 才是彻底丢弃。\n");

    printf("\n=== ③ SIGCHLD：子进程状态变化的通知 ===\n");
    install(SIGCHLD, on_chld, SA_RESTART | SA_NOCLDSTOP);
    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) _exit(0);
    int st; waitpid(pid, &st, 0);
    printf("  父进程 waitpid 已回收子进程 %d\n", (int)pid);
    printf("  -> 注意竞态：SIGCHLD 可能在 waitpid 之前/之后到，\n");
    printf("     handler 里不该假设子进程一定还活着。\n");

    printf("\n=== ④ 慢系统调用被信号打断：EINTR ===\n");
    if (install(SIGALRM, on_alarm, 0) < 0) { perror("sigaction SIGALRM"); return 1; }
    printf("  注册 SIGALRM，flags 里**故意不给** SA_RESTART\n");

    int pp[2];
    if (pipe(pp) < 0) { perror("pipe"); return 1; }
    /* 保留写端不写：read() 会一直阻塞（这就是「慢系统调用」） */
    struct itimerval it;
    it.it_value.tv_sec = 0;  it.it_value.tv_usec = 100000;        /* 100ms 后响 */
    it.it_interval.tv_sec = 0; it.it_interval.tv_usec = 100000;
    if (setitimer(ITIMER_REAL, &it, NULL) < 0) perror("setitimer");

    char c;
    errno = 0;
    ssize_t n = read(pp[0], &c, 1);
    printf("  阻塞中的 read() 返回 %zd，errno=%d (%s)\n", n, errno, strerror(errno));
    printf("  -> EINTR(%d) = Interrupted system call：不是错误，是「被打断了，重试」\n", EINTR);
    printf("  -> 正确写法：do { n = read(...); } while (n < 0 && errno == EINTR);\n");
    printf("  -> 或者给 sigaction 加 SA_RESTART，让内核自动重启这个调用\n");
    errno = 0;
    close(pp[0]); close(pp[1]);

    printf("\n=== ⑤ 处理器里什么能调、什么不能调 ===\n");
    printf("  异步信号安全（可以调）：write / read / open / close / _exit /\n");
    printf("                          kill / sigprocmask / waitpid / fcntl\n");
    printf("  不安全（别调）：printf / malloc / free / strerror / syslog\n");
    printf("  原因：它们是**不可重入**的。主流程可能正好在 printf 内部改着\n");
    printf("        stdio 的缓冲区，handler 插进来再调一次 → 数据错乱或死锁。\n");
    printf("  本 demo 的 handler 只用 write()，就是为了守这条规矩。\n");

    printf("\n=== ⑥ 常用信号一览 ===\n");
    printf("  SIGINT(%2d)   Ctrl+C          默认终止\n", SIGINT);
    printf("  SIGQUIT(%2d)  Ctrl+\\          默认终止 + core\n", SIGQUIT);
    printf("  SIGTERM(%2d)  kill 默认       默认终止（可捕获，礼貌退出）\n", SIGTERM);
    printf("  SIGKILL(%2d)  杀无赦          不可捕获/屏蔽/忽略 —— 唯一杀不死进程的兜底\n", SIGKILL);
    printf("  SIGSTOP(%2d)  Ctrl+Z 的硬版   不可捕获，暂停\n", SIGSTOP);
    printf("  SIGCHLD(%2d)  子进程状态变化\n", SIGCHLD);
    printf("  SIGPIPE(%2d)  写一个读端已关的管道 —— 默认终止（网络服务常见坑）\n", SIGPIPE);
    printf("  SIGHUP(%2d)   控制终端断开 —— 传统上用来让 daemon 重读配置\n", SIGHUP);
    return 0;
}
