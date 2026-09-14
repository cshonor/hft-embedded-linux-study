/* TLPI 第 2 章 §2.6 —— 程序（Programs）：execve 换了什么、没换什么
 *
 * 编译：gcc -O0 -Wall -Wextra c2_6_exec.c -o c2_6
 * 运行：./c2_6
 *
 * 本节要钉死的事实：
 *   ① exec 是「用新程序整个替换当前进程的地址空间」，不是新建进程。
 *      PID 不变、fd 不变（除非 CLOEXEC）、信号处置大多复位、argv/envp 换新。
 *   ② execve 拿到的是「路径 + argv 数组 + envp 数组」，自己不做 PATH 搜索。
 *   ③ execvp/execvpe 才会去查 PATH —— PATH 搜索是**库函数**行为，不是内核行为。
 *   ④ 成功 exec 永不返回；返回了一定是失败，此时 errno 有效。
 *   ⑤ /proc/self/cmdline 与 /proc/self/environ 是内核保存的原始字节串（NUL 分隔）。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

extern char **environ;

/* 把 NUL 分隔的字节串打印成可读形式 */
static void dump_nul(const char *path, const char *label)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) { printf("  %s: 打不开 (%s)\n", label, strerror(errno)); return; }
    char buf[2048];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n < 0) { printf("  %s: 读失败\n", label); return; }
    buf[n] = '\0';

    printf("  %s（原始字节，NUL 分隔）：\n    ", label);
    for (ssize_t i = 0; i < n; i++) putchar(buf[i] == '\0' ? '|' : buf[i]);
    printf("\n");

    printf("  %s（逐条）：\n", label);
    for (ssize_t i = 0; i < n; ) {
        const char *s = buf + i;
        size_t len = strlen(s);
        if (len) printf("    [%ld] %s\n", (long)(s - buf), s);
        i += (ssize_t)len + 1;
    }
}

int main(int argc, char **argv)
{
    /* ================= 子进程分支：被 execve 出来的新程序 ================= */
    if (argc > 1 && strcmp(argv[1], "--child") == 0) {
        printf("=== 子进程（execve 之后的新程序）===\n");
        printf("  getpid() = %d   ← 还是原来的 PID，但内存里已经是另一个程序\n",
               (int)getpid());

        printf("\n  收到的 argv[argc] 数组（argc=%d）：\n", argc);
        for (int i = 0; i < argc; i++)
            printf("    argv[%d] = \"%s\"\n", i, argv[i]);

        int cnt = 0;
        for (char **e = environ; *e; e++) cnt++;
        printf("\n  收到的 envp（共 %d 条，只列前 4 条）：\n", cnt);
        for (int i = 0; environ[i] && i < 4; i++)
            printf("    environ[%d] = %s\n", i, environ[i]);

        printf("\n  注意：execve 传进来的 envp 是新进程的**全部**环境，\n");
        printf("  父进程的环境变量不会自动继承 —— 要自己拼好整份 envp。\n");

        printf("\n  /proc/self/cmdline 与 /proc/self/environ 对照：\n");
        dump_nul("/proc/self/cmdline", "cmdline");
        printf("\n  （environ 在 /proc/self/environ 里同样的格式）\n");
        return 0;
    }

    /* ================= 父进程 ================= */
    printf("=== ① 用 execve 把子进程整个换成另一个程序 ===\n");
    fflush(NULL);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        static char exe[512];
        ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
        if (n < 0) { perror("readlink /proc/self/exe"); _exit(127); }
        exe[n] = '\0';

        /* execve 的三个参数：路径、argv、envp */
        char *av[] = { exe, "--child", "alpha", "beta", NULL };
        char *ev[] = { "C2_DEMO=execve", "PATH=/nowhere", NULL };
        printf("  [child] execve(\"%s\", av, ev)  —— 成功就不会返回\n", exe);
        fflush(NULL);
        execve(exe, av, ev);
        printf("  [child] 走到这里说明 execve 失败了：errno=%d (%s)\n",
               errno, strerror(errno));
        _exit(127);
    }

    int st = 0;
    if (waitpid(pid, &st, 0) < 0) perror("waitpid");
    else printf("  [parent] 子进程 %d 结束，退出码 %d\n", (int)pid, WEXITSTATUS(st));

    printf("\n=== ② execve 不做 PATH 搜索（execvp 才做）===\n");
    const char *envpath = getenv("PATH");
    printf("  本进程的 PATH = %s\n",
           envpath ? envpath : "(未设置)");
    fflush(NULL);                        /* fork 前必须清缓冲，否则父、子各印一遍 */
    pid_t p2 = fork();
    if (p2 == 0) {
        errno = 0;
        execve("echo", (char *[]) { "echo", "hi", NULL }, environ);
        printf("  execve(\"echo\")  失败 errno=%d (%s)\n", errno, strerror(errno));
        errno = 0;
        execvp("echo", (char *[]) { "echo", "hi", NULL });
        printf("  execvp(\"echo\") 失败 errno=%d (%s)\n", errno, strerror(errno));
        fflush(NULL);                 /* 子进程要打印就必须自己 flush：_exit 不清 stdio */
        _exit(0);
    }
    waitpid(p2, NULL, 0);
    printf("  → execve 只认「路径」，PATH 搜索是 execvp 在用户态自己做的：\n");
    printf("    它按 PATH 逐段拼路径，对每段调 access/stat 试探，最后才 execve。\n");

    printf("\n=== ③ exec 失败时的两个经典 errno ===\n");
    fflush(NULL);
    pid_t p3 = fork();
    if (p3 == 0) {
        errno = 0;
        execve("/definitely/not/here", (char *[]) { "x", NULL }, environ);
        printf("  execve(\"/definitely/not/here\") -> errno=%d (%s)\n",
               errno, strerror(errno));
        errno = 0;
        execve("/tmp", (char *[]) { "x", NULL }, environ);   /* 目录不是可执行文件 */
        printf("  execve(\"/tmp\")                -> errno=%d (%s)\n",
               errno, strerror(errno));
        fflush(NULL);
        _exit(0);
    }
    waitpid(p3, NULL, 0);
    printf("  ENOENT(%d)=路径不存在   EACCES(%d)=有文件但无执行权\n", ENOENT, EACCES);
    printf("  ENOEXEC(%d)=格式不对（不是可执行文件或解释器缺失）\n", ENOEXEC);

    printf("\n=== ④ exec 换了什么 / 没换什么 ===\n");
    printf("  换了：地址空间（代码/数据/堆/栈）、argv、envp、\n");
    printf("        信号处置（被捕获的复位成默认）、线程（只剩调用线程）\n");
    printf("  没换：PID、PPID、进程组/会话、真实 UID/GID、\n");
    printf("        打开的文件描述符（除标了 FD_CLOEXEC 的）、当前工作目录、\n");
    printf("        资源限制（rlimit）、定时器\n");
    return 0;
}
