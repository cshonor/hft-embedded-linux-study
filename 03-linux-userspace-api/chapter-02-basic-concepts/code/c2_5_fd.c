/* TLPI 第 2 章 §2.5 —— fd 是进程私有的下标：最小可用分配 + O_CLOEXEC
 *
 * 编译：gcc -O0 -Wall -Wextra c2_5_fd.c -o c2_5
 * 运行：./c2_5
 *
 * 本节要钉死的事实：
 *   ① fd 只是 files_struct->fdtable[] 的下标，进程私有。
 *   ② 分配规则是「最小可用」—— close(1) 后下一次 open 必拿到 1，
 *      这正是「重定向」惯用法（close + open）能工作的原因。
 *   ③ O_CLOEXEC 把「打开」和「exec 时关闭」原子绑定，消除多线程竞态窗口。
 *   ④ close(fd) 只是把 fdtable 摘掉 + 引用计数减一，归零才释放 struct file。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>

#define C2DIR "/tmp/c2_5"

/* 子进程分支：列出自己继承到的 fd（验证 O_CLOEXEC 真的把它关了） */
static void list_my_fds(const char *tag)
{
    DIR *d = opendir("/proc/self/fd");
    if (!d) { perror("opendir /proc/self/fd"); return; }
    int self = dirfd(d);                   /* 读目录本身占的那个 fd，要排除掉 */
    struct dirent *e;
    printf("  %s 继承到的 fd：", tag);
    int first = 1;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        if (atoi(e->d_name) == self) continue;
        char p[320], tgt[320];
        snprintf(p, sizeof(p), "/proc/self/fd/%s", e->d_name);
        ssize_t n = readlink(p, tgt, sizeof(tgt) - 1);
        if (n < 0) continue;
        tgt[n] = '\0';
        if (!first) printf("  ");
        printf("%s=%s", e->d_name, tgt);
        first = 0;
    }
    printf("\n");
    closedir(d);
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--listfds") == 0) {
        printf("=== 子进程（exec 之后）===\n");
        list_my_fds("child");
        return 0;
    }

    unlink("/tmp/c2_5_plain"); unlink("/tmp/c2_5_exec");
    mkdir(C2DIR, 0755);

    printf("=== ① fd 是「最小可用整数」===\n");
    int fds[5];
    for (int i = 0; i < 5; i++) {
        char p[128];
        snprintf(p, sizeof(p), "%s/f%d.txt", C2DIR, i);
        fds[i] = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fds[i] < 0) { perror("open"); return 1; }
        printf("  第 %d 次 open -> fd=%d\n", i + 1, fds[i]);
    }
    printf("  -> 0/1/2 已被 stdin/stdout/stderr 占用，所以从 3 开始连号\n");

    printf("\n=== ② 关掉再开：证明「最小可用」不是「递增」===\n");
    printf("  close(fd=%d) 然后重新 open：\n", fds[1]);
    close(fds[1]);
    char p2[128];
    snprintf(p2, sizeof(p2), "%s/reused.txt", C2DIR);
    int reused = open(p2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    printf("    新 open 拿到 fd=%d\n", reused);
    printf("    %s\n", reused == fds[1] ? "YES —— 正好填刚空出来的那个洞"
                                         : "NO  —— 与预期不符");
    close(reused);
    for (int i = 0; i < 5; i++) if (i != 1) close(fds[i]);
    errno = 0;

    printf("\n=== ③ 重定向惯用法：close(1) + open == 换掉 stdout ===\n");
    int save = dup(STDOUT_FILENO);         /* 备份 stdout */
    fflush(NULL);                          /* 先把缓冲清干净，别让它落到新文件里 */
    close(STDOUT_FILENO);                  /* 腾出 fd 1 */
    int redir = open("/tmp/c2_5_redirect.txt",
                     O_WRONLY | O_CREAT | O_TRUNC, 0644);
    /* 此刻 stdio 仍认为 stdout 就是 fd 1 —— 而 fd 1 已经换了目标 */
    printf("这行是 printf 写的，但它跑进了 %s（fd=%d）\n",
           "/tmp/c2_5_redirect.txt", redir);
    fflush(stdout);
    close(redir);
    dup2(save, STDOUT_FILENO);             /* 恢复 stdout */
    close(save);
    printf("  open 拿到的 fd = %d\n", redir);
    printf("  重定向是否成功：%s\n", redir == STDOUT_FILENO ? "YES（fd==1）" : "NO");

    /* 把被重定向的那行读回来给读者看 */
    int rf = open("/tmp/c2_5_redirect.txt", O_RDONLY);
    if (rf >= 0) {
        char buf[256];
        ssize_t n = read(rf, buf, sizeof(buf) - 1);
        if (n > 0) {
            if (buf[n - 1] == '\n') buf[--n] = '\0';
            buf[n] = '\0';
            printf("  文件内容 = \"%.*s\"\n", (int)n, buf);
        }
        close(rf);
    }
    printf("\n  恢复 stdout 后再 printf 仍然正常：OK\n");

    printf("\n=== ④ O_CLOEXEC：把「打开」和「exec 时关闭」原子绑定 ===\n");
    int pfd = open("/tmp/c2_5_plain", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int efd = open("/tmp/c2_5_exec",
                   O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    printf("  open(无标志)      -> fd=%d  FD_CLOEXEC=%d\n",
           pfd, fcntl(pfd, F_GETFD) & FD_CLOEXEC);
    printf("  open(O_CLOEXEC)   -> fd=%d  FD_CLOEXEC=%d\n",
           efd, fcntl(efd, F_GETFD) & FD_CLOEXEC);
    printf("  也可以事后补上：fcntl(fd, F_SETFD, FD_CLOEXEC)\n");
    printf("  （多线程下「open 后再 setfd」中间有竞态窗口，所以要用 O_CLOEXEC）\n");

    printf("\n  真实验证：fork + exec 自己，看子进程继承到哪些 fd\n");
    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) {
        static char exe[512];
        ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
        if (n < 0) _exit(127);
        exe[n] = '\0';
        execve(exe, (char *[]) { exe, "--listfds", NULL },
               (char *[]) { NULL });
        _exit(127);
    }
    int st; waitpid(pid, &st, 0);
    printf("  （上面 child 列表里应看到普通 fd 还在，O_CLOEXEC 那个已消失）\n");
    close(pfd); close(efd);
    unlink("/tmp/c2_5_plain"); unlink("/tmp/c2_5_exec");

    printf("\n=== ⑤ fd 上限：RLIMIT_NOFILE ===\n");
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
        printf("  getrlimit(RLIMIT_NOFILE): soft=%lld hard=%lld\n",
               (long long)rl.rlim_cur, (long long)rl.rlim_max);
    printf("  sysconf(_SC_OPEN_MAX) = %ld\n", sysconf(_SC_OPEN_MAX));

    int got = 0, devnull[512];
    for (; got < 512; got++) {
        int f = open("/dev/null", O_RDONLY);
        if (f < 0) break;
        devnull[got] = f;
    }
    printf("  死命 open 到崩：成功 %d 个后失败 errno=%d (%s)\n",
           got, errno, strerror(errno));
    printf("  EMFILE(%d)=进程 fd 用尽  ENFILE(%d)=全系统用尽\n", EMFILE, ENFILE);
    for (int i = 0; i < got; i++) close(devnull[i]);
    errno = 0;

    printf("\n=== ⑥ /proc/self/fd 是 fdtable 的用户态视图 ===\n");
    DIR *d = opendir("/proc/self/fd");
    if (d) {
        int self = dirfd(d);
        struct dirent *e;
        while ((e = readdir(d))) {
            if (e->d_name[0] == '.') continue;
            if (atoi(e->d_name) == self) continue;
            char p[320], tgt[320];
            snprintf(p, sizeof(p), "/proc/self/fd/%s", e->d_name);
            ssize_t n = readlink(p, tgt, sizeof(tgt) - 1);
            if (n < 0) continue;
            tgt[n] = '\0';
            printf("  fd %-3s -> %s\n", e->d_name, tgt);
        }
        closedir(d);
    }
    return 0;
}
