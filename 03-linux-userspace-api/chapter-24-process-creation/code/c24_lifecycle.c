/* c24_lifecycle.c

   ⚠️ 本仓库自写 —— 原书 §24.1 **只有一张 Figure 24-1**（"Overview of the use of
      fork(), exit(), wait(), and execve()"），既没有 Listing，也没有配套程序。

   本程序把 Figure 24-1 从"一张图"变成"一串可运行的真实输出"：

     ① 单进程：报自己的 pid / ppid
     ② fork()  —— 一次调用，两个返回值（父得子 pid，子得 0）
     ③ _exit(7) —— 子进程带一个状态码退出
     ④ wait(&status) —— 父进程取回状态，并把它**解码**（很多人只会用 WEXITSTATUS
        却不知道 status 里还有什么）
     ⑤ fork() + execve() —— 再起一个子进程，用 exec 把它的**程序映像**换掉；
        对照 ③：这次子进程不是"_exit"，而是"变成了另一个程序"

   ⚠️ ⑤ 为什么 exec 的是「自己」（/proc/self/exe）而不是 /bin/echo：
      CE 的 executor 沙箱里 **PATH 是空的、/bin 和 /usr/bin 下什么都没有**
      （实测见 code/probe24.c：`access("/bin/echo", X_OK) == -1`）。
      而 exec 自己恰好是最干净的演示 —— 同一个可执行文件，换一副 argv 就是
      「另一个程序」；并且能顺便看到 **exec 不换 PID**（对比 fork 会换）。

   ⚠️ 没写 exit() vs _exit() 的区别：那是 §25.1 的主题（exit() 会刷 stdio 缓冲、
      会跑 atexit 处理器，_exit() 不会）。这里两个都用 `_exit()`。

   编译（自包含）：
     gcc -O0 -Wall -Wextra -o c24_lifecycle c24_lifecycle.c
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *msg)
{
    fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    pid_t childPid;
    int status;

    setvbuf(stdout, NULL, _IONBF, 0);   /* 关缓冲，父子输出不互相夹杂 */

    /* ---- 被 ⑤ 里的 exec 拉起来时走这条分支：说明"我已经是另一个映像了" ---- */
    if (argc > 1 && strcmp(argv[1], "--after-exec") == 0) {
        printf("⑤★ exec 之后  pid 还是 %ld —— execve 换映像但**不换 PID**\n",
                (long) getpid());
        printf("     （能走到这一行，本身就证明 execve 成功了：\n");
        printf("      同一个可执行文件，靠 argv[1]=\"%s\" 走了不同分支）\n", argv[1]);
        return 0;
    }

    /* ---------------- ① ---------------- */
    printf("① 单进程      pid=%ld ppid=%ld\n", (long) getpid(), (long) getppid());

    /* ---------------- ② ③ ④ ------------- */
    switch (childPid = fork()) {
    case -1:
        die("fork");

    case 0:                                  /* ③ 子进程 */
        printf("② 子进程      pid=%ld  fork() 返回 0（这是子进程的判据）\n",
                (long) getpid());
        printf("③ 子进程      _exit(7) —— 带 7 退出\n");
        _exit(7);                            /* 注意：不是 exit() */

    default:                                 /* ② 父进程 */
        printf("② 父进程      pid=%ld  fork() 返回 %ld（= 子进程 pid）\n",
                (long) getpid(), (long) childPid);

        if (wait(&status) == -1)             /* ④ */
            die("wait");

        /* status 不是"退出码"，是内核打包的一整块信息 */
        printf("④ 父进程      wait() 拿到 status=%d\n", status);
        printf("   WIFEXITED   = %d\n", WIFEXITED(status));
        printf("   WEXITSTATUS = %d   （子进程 _exit 的那个 7）\n",
                WEXITSTATUS(status));
        printf("   WIFSIGNALED = %d   （0 = 不是被信号杀死的）\n",
                WIFSIGNALED(status));
        printf("   —— 7 << 8 = %d，所以直接看 status 会看到 %d 而不是 7\n",
                7 << 8, 7 << 8);
        break;
    }

    /* ---------------- ⑤ execve 把子进程换成另一个程序 ------------- */
    switch (childPid = fork()) {
    case -1:
        die("fork");

    case 0:
        printf("⑤ 子进程      pid=%ld 即将 exec —— 下面两行的输出来自新映像\n",
                (long) getpid());
        execl("/proc/self/exe", "c24_lifecycle", "--after-exec", (char *) NULL);
        die("execl(/proc/self/exe)");        /* exec 成功则永远到不了这里 */

    default:
        if (wait(NULL) == -1)
            die("wait");
        printf("⑤ 父进程      第二个子进程（pid=%ld）也收完了\n", (long) childPid);
        break;
    }

    printf("── 结束：Figure 24-1 的四个调用都跑过一遍了 ──\n");
    return 0;
}
