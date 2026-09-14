/* TLPI 第 2 章 §2.15 —— 伪终端（PTY）：为什么 ssh/tmux 需要它
 *
 * 编译：gcc -O0 -Wall -Wextra c2_15_pty.c -o c2_15
 * 运行：./c2_15
 *
 * 本节要钉死的事实：
 *   ① 伪终端是「一对」设备：master（一端给程序，如 sshd）+ slave（另一端给
 *      终端程序，如 bash）。数据从一端进、从另一端出。
 *   ② 关键是 slave 端带**终端语义**（行缓冲、Ctrl+C 生成 SIGINT、回显、
 *      ioctl(TIOCGWINSZ) 拿窗口大小）—— 这是普通管道做不到的。
 *   ③ 打开 master 靠 posix_openpt()，它需要一个挂载好的 devpts（/dev/ptmx）。
 *      本容器把它裁掉了，所以只能看到「失败」这一面。
 *   ④ 没有 PTY 时，用 socketpair/pipe 也能做双向字节流，但拿不到终端语义。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(void)
{
    printf("=== ① 先在容器里试一把：能不能拿到 PTY master ===\n");
    errno = 0;
    int m = posix_openpt(O_RDWR | O_NOCTTY);
    printf("  posix_openpt(O_RDWR|O_NOCTTY) = %d", m);
    if (m < 0) printf("  errno=%d (%s)", errno, strerror(errno));
    printf("\n");
    errno = 0;

    int a1 = access("/dev/ptmx", F_OK | R_OK | W_OK);
    printf("  access(\"/dev/ptmx\") = %d", a1);
    if (a1 < 0) printf("  errno=%d (%s)", errno, strerror(errno));
    printf("\n");
    errno = 0;

    int a2 = access("/dev/pts", F_OK);
    printf("  access(\"/dev/pts\")  = %d", a2);
    if (a2 < 0) printf("  errno=%d (%s)", errno, strerror(errno));
    printf("\n");
    errno = 0;

    printf("  -> PTY 不是内核里「一定有」的东西：它需要 devpts 文件系统挂载在\n");
    printf("     /dev/pts，并由 /dev/ptmx 提供 master 的申请入口。\n");
    printf("     容器里这两个都没有 → 直接 ENOENT。这是环境边界，不是代码错。\n");

    printf("\n=== ② PTY 的结构：为什么非得有它 ===\n");
    printf("     sshd / script / tmux          bash / vim / top\n");
    printf("            |                              |\n");
    printf("     写/读 master <==== 内核 PTY ====> 写/读 slave\n");
    printf("                    （内含行规程 line discipline）\n");
    printf("\n");
    printf("  程序侧只拿到 master，终端程序侧拿到 slave。中间那一层「行规程」负责：\n");
    printf("    · 行缓冲：你打的字先攒着，按回车才交给程序\n");
    printf("    · Ctrl+C → 生成 SIGINT 发给 slave 的**前台进程组**\n");
    printf("    · Ctrl+Z → SIGTSTP；Ctrl+\\ → SIGQUIT\n");
    printf("    · 回显、退格处理、ICANON/ECHO 等 termios 开关\n");
    printf("    · ioctl(TIOCGWINSZ) 让 vim/top 知道「窗口多少行多少列」\n");
    printf("  这些语义**只有终端设备有**，普通 pipe/socket 一律没有。\n");
    printf("  → 所以「远程把 bash 接到一个管道上」做不成：bash 会发现 stdin 不是\n");
    printf("     终端（isatty 为假），于是退化成非交互模式、不做补全、不打印提示符。\n");

    printf("\n=== ③ 没有 PTY 时的替代：socketpair 做双向字节流 ===\n");
    printf("  用 socketpair 能复现「一边写、一边读、还能回话」的骨架，\n");
    printf("  但拿不到终端语义。实测一下骨架部分：\n");
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) { perror("socketpair"); return 1; }

    fflush(NULL);
    pid_t pid = fork();
    if (pid == 0) {
        /* ---- slave 侧：只持有 sv[0] 这一端 ---- */
        close(sv[1]);
        char buf[256];
        ssize_t n = read(sv[0], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("  [slave ] 收到: \"%s\"", buf);
            const char *out = "hi from slave\n";
            if (write(sv[0], out, strlen(out)) < 0) { /* 忽略 */ }
        } else {
            printf("  [slave ] read 返回 %zd（对端已关闭）\n", n);
        }
        printf("  [slave ] isatty(slave fd) = %d  <- 0：没有终端语义\n", isatty(sv[0]));
        close(sv[0]);
        fflush(NULL);
        _exit(0);
    }

    /* ---- master 侧：只持有 sv[1] 这一端 ---- */
    close(sv[0]);
    const char *cmd = "echo hello-from-master\n";
    if (write(sv[1], cmd, strlen(cmd)) < 0) perror("write to slave");
    printf("  [master] 写入: \"%s\"", cmd);
    fflush(NULL);                        /* 先把上面这行送出去，顺序好读 */

    char rb[256];
    ssize_t k = read(sv[1], rb, sizeof(rb) - 1);
    if (k > 0) { rb[k] = '\0'; printf("  [master] 收到: \"%s\"", rb); }
    else       printf("  [master] read 返回 %zd\n", k);
    printf("  [master] isatty(master fd) = %d\n", isatty(sv[1]));
    close(sv[1]);
    waitpid(pid, NULL, 0);

    printf("\n  对照：真 PTY 时 master 侧也不会让 slave 看到终端之外的东西，\n");
    printf("  但 slave 侧 isatty() 会是 1，且 termios（tcgetattr/tcsetattr）可用。\n");

    printf("\n=== ④ 真实世界里谁在用 PTY ===\n");
    printf("  %-16s %s\n", "ssh / telnet", "服务端 forkpty()，把 shell 接到 slave");
    printf("  %-16s %s\n", "script(1)", "录下终端会话（master 侧一边抄一份）");
    printf("  %-16s %s\n", "tmux / screen", "自己实现终端多路复用，需要 PTY 对");
    printf("  %-16s %s\n", "xterm / gnome-terminal", "终端模拟器本身：master 持有 PTY，替你画屏");
    printf("  %-16s %s\n", "expect / pexpect", "自动化交互式程序，靠 PTY 骗过程序「你连的是终端」");
    printf("  %-16s %s\n", "docker run -t / kubectl exec -it", "那个 -t 就是「分配一个伪终端」");
    printf("\n  → 见到 -t / -it 就想到 PTY：它给容器里的进程一个「看起来像终端」的 fd。\n");

    printf("\n=== ⑤ 代码层面的四个调用（本 demo 无法全部跑通）===\n");
    printf("  %-30s %s\n", "posix_openpt(O_RDWR)", "拿 master fd（需要 /dev/ptmx）");
    printf("  %-30s %s\n", "grantpt(mfd)", "改 slave 的属主/权限（一般由内核/devpts 处理）");
    printf("  %-30s %s\n", "unlockpt(mfd)", "解锁 slave，允许打开");
    printf("  %-30s %s\n", "ptsname(mfd)", "取 slave 的路径（如 /dev/pts/3）");
    printf("  %-30s %s\n", "forkpty(&m, ...)", "上面四步 + fork + 把 slave 设为子进程的 0/1/2");
    printf("  本环境在第一步就 ENOENT 了，后面全部无法执行 —— 如实记录。\n");
    return 0;
}
