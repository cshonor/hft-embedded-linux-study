/* ex18_1_txtbsy.c — 习题 18-1：正在执行的程序为什么还能被「覆盖」？
 *
 * 题面（原书 §18 习题 18-1，按转述）：§4.3.2 说「文件正在被执行时不能以写方式
 * 打开（open 返回 -1，errno=ETXTBSY）」。但从 shell 里却可以：
 *     cc -o longrunner longrunner.c
 *     ./longrunner &                 # 后台留着跑
 *     vi longrunner.c                # 改源码
 *     cc -o longrunner longrunner.c  # 这条居然成功！
 * 为什么？（提示：两次编译后用 ls -li 看可执行文件的 i-node。）
 *
 * 答案（②③④三部分钉住）：
 *   ③ 直接以 O_WRONLY 打开**正在执行**的二进制 → Linux 拒（ETXTBSY）；
 *   ④ cc 的「覆盖」根本没写老文件，而是**建新 inode + rename 换目录项**：
 *      rename 成功、inode 号变了，而老 inode 被运行中进程的 fd 吊住
 *      （回到 §18.3：nlink 归零且无 fd 才真删）。所以「覆盖」安全，
 *      正在跑的程序用的仍是老那份数据。
 *
 * ⚠️ ⓪ 是本程序特意保留的「踩坑自证」环节，别删：
 *   最初长跑程序是 `cp /bin/sleep /tmp/prog` 造的，结果子进程**瞬间就退**——
 *   macOS 代码签名（AMFI/taskgated）会 SIGKILL 掉「离开原位置的平台二进制」
 *   （实测 rc=137 = 128+SIGKILL）。那样测出的「open 成功」只证明「打开了一个
 *   没在运行的文件」，结论完全无效。所以长跑程序改为**拷自己编译的二进制**。
 *
 * 编译： cc -Wall -Wextra -o ex18_1_txtbsy ex18_1_txtbsy.c
 * 取材： TLPI §4.3.2 / §18.1 / §18.3 / 习题 18-1；man-pages 6.19 open(2)(ETXTBSY)、rename(2)
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#define PROG     "/tmp/ex18_1_longrunner"       /* 会被"覆盖"的可执行文件 */
#define PROG_NEW "/tmp/ex18_1_longrunner.new"   /* "重新编译"的产物 */
#define SYSCOPY  "/tmp/ex18_1_syscopy"          /* ⓪ 用的系统二进制副本 */

static ino_t ino_of(const char *p)
{
    struct stat sb;
    return stat(p, &sb) == 0 ? sb.st_ino : 0;
}

/* 三态探活：真的在跑 / 已退出 / 查询出错 */
static const char *probe(pid_t c)
{
    int st;
    errno = 0;
    pid_t w = waitpid(c, &st, WNOHANG);
    if (w == 0) return "仍在运行 ✔";
    if (w == c) return "已退出 ✘（子进程没跑住 → 本项测试无效）";
    return "waitpid 出错（ECHILD 等）";
}

/* 跑一小段以观察它是「正常退出」还是「被信号杀死」 */
static void exec_and_report(const char *prog, const char *tag)
{
    pid_t c = fork();
    if (c < 0) { perror("fork"); return; }
    if (c == 0) { execl(prog, prog, (char *) NULL); _exit(127); }
    int st;
    waitpid(c, &st, 0);
    if (WIFSIGNALED(st))
        printf("  %-22s 被信号杀死：SIG %d（rc=%d = 128+%d）\n",
               tag, WTERMSIG(st), 128 + WTERMSIG(st), WTERMSIG(st));
    else
        printf("  %-22s 正常退出 rc=%d\n", tag, WEXITSTATUS(st));
}

/* 子模式：被 exec 起来的长跑进程本体（也方便本程序自证「能跑住」）*/
static int child_mode(void)
{
    fprintf(stderr, "[longrunner] pid=%d 正在运行，sleep(20)\n", (int) getpid());
    sleep(20);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "--child") == 0)
        return child_mode();

    /* ---------- ⓪ 先演示那个坑：拷系统二进制 = 跑不起来 ---------- */
    printf("== ⓪ 陷阱预热：为什么长跑程序不能「拷」系统的 sleep ==\n");
    char cp[256];
    snprintf(cp, sizeof cp, "cp /bin/sleep %s 2>/dev/null && chmod 755 %s",
             SYSCOPY, SYSCOPY);
    if (system(cp) == 0) {
        exec_and_report(SYSCOPY, "cp /bin/sleep 的副本");
        printf("  → 平台签名二进制离开原位置即被 AMFI/taskgated 干掉。\n");
        printf("     若用它当长跑程序，子进程秒退 → 后面所有结论都是假的。\n");
        unlink(SYSCOPY);
    } else {
        printf("  （本机无 /bin/sleep，跳过此项）\n");
    }
    printf("  ✔ 正解：长跑程序拷**自己编译**的二进制（ad-hoc 签名可任意搬）。\n\n");

    /* ---------- 准备长跑程序：拷贝本程序自身 ---------- */
    unlink(PROG);
    snprintf(cp, sizeof cp, "cp %s %s", argv[0], PROG);
    if (system(cp) != 0) { perror("copy self"); return EXIT_FAILURE; }
    ino_t ino_before = ino_of(PROG);

    pid_t child = fork();
    if (child == -1) { perror("fork"); return EXIT_FAILURE; }
    if (child == 0) {
        execl(PROG, PROG, "--child", (char *) NULL);
        _exit(127);
    }
    sleep(1);                                  /* 等孩子进入 exec */

    printf("== ① 待测对象 ==\n");
    printf("  子进程 pid=%d 正在执行 %s（inode=%lu）\n",
           (int) child, PROG, (unsigned long) ino_before);
    printf("  探活：%s（若显示已退出，说明长跑程序没准备好，后面结论无效）\n",
           probe(child));

    printf("\n== ② 直接以写方式打开「正在执行」的二进制 ==\n");
    errno = 0;
    int fd = open(PROG, O_WRONLY);
    printf("  open(PROG, O_WRONLY) = %d errno=%d(%s)\n",
           fd, errno, fd == -1 ? strerror(errno) : "未置位");
    if (fd == -1)
        printf("  → 被拒。Linux 上此即 ETXTBSY(16)「Text file busy」。\n");
    else {
        printf("  → ⚠️ 本机 macOS(APFS) **不 enforce ETXTBSY**：正在执行的二进制\n");
        printf("     竟可被写打开（危险！）。Linux 上是 ETXTBSY(16)。跨平台务必单独验证。\n");
        close(fd);
    }

    printf("\n== ③ cc 的「覆盖」：建新 inode + rename，老 inode 被 fd 吊住 ==\n");
    snprintf(cp, sizeof cp, "cp %s %s", argv[0], PROG_NEW);   /* 冒充"重新编译的产物" */
    if (system(cp) != 0) { perror("copy new"); return EXIT_FAILURE; }
    printf("  「重新编译」产出 %s（inode=%lu）\n",
           PROG_NEW, (unsigned long) ino_of(PROG_NEW));
    errno = 0;
    int r = rename(PROG_NEW, PROG);            /* 就是 cc 落盘那一下 */
    printf("  rename(.new → 老名字) = %d %s\n", r, r == 0 ? "" : strerror(errno));
    printf("  覆盖后 PROG 的 inode = %lu  %s\n",
           (unsigned long) ino_of(PROG),
           ino_of(PROG) == ino_before ? "→ 没变（意外）"
                                      : "→ 已换成新 inode ✔（ls -li 可验证）");
    printf("  老 inode %lu 呢？", (unsigned long) ino_before);
    printf("  再次探活：%s\n", probe(child));
    printf("  ⇒ 所谓「覆盖正在执行的程序」= §18.4 的 rename 原子替换；\n");
    printf("     老 inode 因 nlink 归零但仍有 fd 而存活，直到进程退出（§18.3）。\n");

    printf("\n== ④ 子进程退出后再写打开 ==\n");
    int st;
    kill(child, SIGTERM);                      /* 不等 20 秒 */
    waitpid(child, &st, 0);
    errno = 0;
    fd = open(PROG, O_WRONLY);
    printf("  open(O_WRONLY) = %d errno=%d(%s)\n",
           fd, errno, fd == -1 ? strerror(errno) : "未置位");
    if (fd != -1) {
        printf("  → 成功。与 ② 对比：若 ② 为 ETXTBSY，这就是「忙 / 闲」的判据。\n");
        close(fd);
    }

    unlink(PROG); unlink(PROG_NEW);
    return EXIT_SUCCESS;
}
