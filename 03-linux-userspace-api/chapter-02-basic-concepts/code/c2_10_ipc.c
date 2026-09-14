/* TLPI 第 2 章 §2.10 —— IPC 与同步：pipe / socketpair / FIFO 三种最小通道
 *
 * 编译：gcc -O0 -Wall -Wextra c2_10_ipc.c -o c2_10
 * 运行：./c2_10
 *
 * 本节要钉死的事实：
 *   ① pipe() 不经过文件系统，返回两个 fd：fd[0] 读端、fd[1] 写端，单向字节流。
 *   ② 读端全关后再写 → SIGPIPE（默认终止进程）；写端全关后读 → read 返回 0（EOF）。
 *   ③ FIFO 是「有名字的管道」，出现在文件树里，能被不相关的进程 open。
 *   ④ socketpair 天生双向，比 pipe 省一半 fd。
 *   ⑤ PIPE_BUF 是「保证原子」的单次写上限，超过就可能与其他写者交错。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

int main(void)
{
    printf("=== ① pipe：最简单向通道 ===\n");
    int pfd[2];
    if (pipe(pfd) < 0) { perror("pipe"); return 1; }
    printf("  pipe() -> 读端 fd=%d  写端 fd=%d\n", pfd[0], pfd[1]);
    printf("  （两个 fd 都是普通 fd，能用在 select/poll/epoll 里）\n");

    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) {
        close(pfd[0]);                              /* 子进程只写 */
        const char *msg = "hello through pipe";
        ssize_t n = write(pfd[1], msg, strlen(msg));
        printf("  [child]  write(%d, \"%s\", %zu) = %zd 字节\n",
               pfd[1], msg, strlen(msg), n);
        close(pfd[1]);
        fflush(NULL);                               /* 子进程要打印就得自己 flush */
        _exit(0);
    }
    close(pfd[1]);                                  /* 父进程只读 */
    char buf[128];
    ssize_t n = read(pfd[0], buf, sizeof(buf) - 1);
    if (n < 0) { perror("read"); }
    else {
        buf[n] = '\0';
        printf("  [parent] read(%d, ...) = %zd 字节，内容 \"%s\"\n", pfd[0], n, buf);
    }
    int st; waitpid(pid, &st, 0);
    printf("  → 连 write 都不用调就用完了：字节流 + 内核里的环形缓冲区。\n");

    printf("\n=== ② pipe 的两个数字：容量与原子写上限 ===\n");
    int sz = fcntl(pfd[0], F_GETPIPE_SZ);
    printf("  fcntl(fd, F_GETPIPE_SZ) = %d 字节   （管道缓冲区容量）\n", sz);
    printf("  PIPE_BUF                = %d 字节   （保证原子性的单次写上限）\n", PIPE_BUF);
    printf("  → 单次 write ≤ PIPE_BUF 时，多个写者不会互相插入；\n");
    printf("     超过 PIPE_BUF 就可能被切开、与其他写者的数据交错。\n");
    printf("  → 容量可以改（写 /proc/sys/fs/pipe-max-size 以内）：\n");
    printf("     实测 fcntl(fd, F_SETPIPE_SZ, %d) = %d\n",
           16384, fcntl(pfd[0], F_SETPIPE_SZ, 16384));
    close(pfd[0]);

    printf("\n=== ③ 写端全关之后再读，得到 EOF 而不是阻塞 ===\n");
    int q[2];
    pipe(q);
    close(q[1]);                                    /* 立刻把写端关掉 */
    n = read(q[0], buf, sizeof(buf));
    printf("  写端已关闭，read() 返回 %zd  → 0 表示 EOF\n", n);
    printf("  → 这就是「管道对端退出了」的可靠信号，比轮询 PID 靠谱。\n");
    close(q[0]);

    printf("\n=== ④ socketpair：天生双向，只要一对 fd ===\n");
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) { perror("socketpair"); return 1; }
    printf("  socketpair(AF_UNIX, SOCK_STREAM) -> fd %d 与 %d\n", sv[0], sv[1]);
    fflush(NULL);
    pid_t p2 = fork();
    if (p2 == 0) {
        close(sv[1]);
        char r[64];
        ssize_t k = read(sv[0], r, sizeof(r) - 1);
        if (k > 0) { r[k] = '\0'; printf("  [child]  收到: \"%s\"\n", r); }
        const char *reply = "got it, thanks";
        write(sv[0], reply, strlen(reply));
        close(sv[0]);
        fflush(NULL);
        _exit(0);
    }
    close(sv[1]);
    const char *ask = "ping from parent";
    write(sv[0], ask, strlen(ask));
    char r2[64];
    ssize_t k = read(sv[0], r2, sizeof(r2) - 1);
    if (k > 0) { r2[k] = '\0'; printf("  [parent] 收到: \"%s\"\n", r2); }
    close(sv[0]);
    waitpid(p2, NULL, 0);
    printf("  → 一对 fd 就能来回说话，不用像 pipe 那样开两条。\n");

    printf("\n=== ⑤ FIFO：有名字的管道，能出现在文件树里 ===\n");
    const char *fpath = "/tmp/c2_10.fifo";
    unlink(fpath);
    if (mkfifo(fpath, 0644) < 0) { perror("mkfifo"); return 1; }
    struct stat fst;
    stat(fpath, &fst);
    printf("  mkfifo(\"%s\", 0644) 成功，st_mode=%04o  S_ISFIFO=%d\n",
           fpath, fst.st_mode & 07777, S_ISFIFO(fst.st_mode));
    printf("  → 它现在是一个「文件」，无关进程都能 open 它来会合。\n");

    fflush(NULL);
    pid_t p3 = fork();
    if (p3 == 0) {
        int w = open(fpath, O_WRONLY);               /* 会阻塞到有读者 */
        if (w < 0) { _exit(1); }
        const char *m = "via FIFO";
        write(w, m, strlen(m));
        close(w);
        _exit(0);
    }
    int rfd = open(fpath, O_RDONLY);                 /* 会阻塞到有写者 */
    if (rfd >= 0) {
        char r3[64];
        ssize_t k2 = read(rfd, r3, sizeof(r3) - 1);
        if (k2 > 0) { r3[k2] = '\0'; printf("  父进程从 FIFO 读到: \"%s\"\n", r3); }
        else printf("  读失败/空 (%zd)\n", k2);
        close(rfd);
    } else perror("open FIFO");
    waitpid(p3, NULL, 0);
    unlink(fpath);

    printf("\n=== ⑥ 四种通道对照 ===\n");
    printf("  %-14s %-10s %-18s %-16s %s\n", "mechanism", "direction",
           "named?", "cross-process?", "typical use");
    printf("  %-14s %-10s %-18s %-16s %s\n", "--------------", "----------",
           "------------------", "----------------", "-------------------");
    printf("  %-14s %-10s %-18s %-16s %s\n", "pipe", "one-way", "no",
           "via fork", "parent <-> child");
    printf("  %-14s %-10s %-18s %-16s %s\n", "FIFO", "one-way", "yes (path in fs)",
           "any process", "shell named pipe");
    printf("  %-14s %-10s %-18s %-16s %s\n", "socketpair", "two-way", "no",
           "via fork", "parent <-> child, duplex");
    printf("  %-14s %-10s %-18s %-16s %s\n", "UNIX socket", "two-way",
           "yes (path/abstract)", "any process", "local client/server");
    printf("\n  → 四者共用同一个底层结构：内核里的一对缓冲区 + 两个 fd。\n");
    printf("     「一切皆文件」在这里的兑现就是：它们都能 read/write/select。\n");
    return 0;
}
