/* ex12_3_fd_snapshot.c — Ch12 习题 12-3（page 231）的落点：/proc/PID/fd/ 的解析
 *
 * 原书习题 12-3 要求：写一个程序，列出所有「打开了某个指定路径名」的进程 ——
 * 靠遍历全部 /proc/PID/fd/N 符号链接、对每个链接做 readlink() 来判定，并且
 * 外层遍历 /proc/PID、内层遍历 /proc/PID/fd，需要嵌套的 readdir(3)。
 *
 * 本题真正的难点**不是**外层循环，而是内层这些符号链接到底长什么样：
 *   - 磁盘/伪文件 → 绝对路径（如 /proc/version）
 *   - socket      → "socket:[inode]"
 *   - pipe        → "pipe:[inode]"
 *   - eventfd     → "anon_inode:[eventfd]"
 * 所以「是不是某条路径」绝不能用 st_mode 判（**anon_inode 与磁盘普通文件同为
 * S_IFREG**），判据是 target 的第一个字符是不是 '/'。
 *
 * 本仓库的实现收在「自己看自己」这一层：把 /proc/self/fd 的解析规则与四个坑
 * 全部钉住。原因是沙箱里只有 2 个进程，扫全部进程的 fd 得到的是同一个答案，
 * 观察不到新东西；而下面这四个坑在任何进程上都存在：
 *   ① 枚举 /proc/self/fd 这个动作**自己**会占一个 fd，并且它出现在列表里
 *      （readlink 的结果指向 /proc/<pid>/fd）—— 看到别以为泄漏了；
 *   ② target 的前缀才是真正的类型判据（见上）；
 *   ③ 两次快照可能不一样（别的线程同时 open/close）→ 列表只是某一瞬间的像；
 *   ④ 要查偏移量/打开旗标，fd 目录不够，得看 /proc/self/fdinfo/N。
 *
 * 把「外层 /proc/PID 循环」补上就是原书要求的完整程序：外层用
 * ex12_2_pstree.c 里那套带竞态计数的扫描，内层换成这里的 fd 解析。
 *
 * 编译： gcc -O0 -Wall -Wextra -o ex12_3_fd_snapshot ex12_3_fd_snapshot.c
 * 取材： man-pages 6.19 proc(5)（/proc/PID/fd 是符号链接目录；fdinfo 的内容；
 *         符号链接内容是真实路径，或以 socket:[inode] / pipe:[inode] /
 *         anon_inode:<name> 形式出现）
 *       Linux v6.6 fs/proc/fd.c（proc_fd_link / proc_fdinfo / tid_fd_revalidate）
 *                      fs/proc/base.c（/proc/PID/fd 目录的建立）
 *       TLPI §12.1 结尾「/proc/PID/fd 可当 fd 清点工具」+ 习题 12-3
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* 判类型：**先看 readlink 的前缀**，因为 st_mode 分不出匿名文件与磁盘文件 */
static const char *kind_of(const char *target, const struct stat *st)
{
    if (strncmp(target, "socket:", 7) == 0)
        return "socket";
    if (strncmp(target, "pipe:", 5) == 0)
        return "pipe";
    if (strncmp(target, "anon_inode:", 11) == 0)
        return "anon_inode";
    if (S_ISSOCK(st->st_mode))
        return "socket(stat)";
    if (S_ISFIFO(st->st_mode))
        return "fifo(stat)";
    if (S_ISDIR(st->st_mode))
        return "directory";
    if (S_ISCHR(st->st_mode))
        return "char-dev";
    if (S_ISREG(st->st_mode))
        return "regular";           /* ⚠️ 磁盘文件**和**内核匿名文件都落这里 */
    return "other";
}

/* 打一次快照，返回条目数（不含 . 和 ..）；verbose=1 时逐条列出 */
static int snapshot(int verbose)
{
    DIR *d = opendir("/proc/self/fd");
    if (d == NULL) {
        printf("  opendir(/proc/self/fd) 失败 errno=%d(%s)\n", errno, strerror(errno));
        return -1;
    }
    int n = 0;
    struct dirent *e;

    if (verbose) {
        printf("  %-4s %-11s %-52s %s\n", "fd", "类型", "readlink 目标", "st_mode");
        printf("  %-4s %-11s %-52s %s\n", "---", "----------",
               "----------------------------------------------------", "---------");
    }
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] < '0' || e->d_name[0] > '9')
            continue;
        n++;

        if (!verbose)
            continue;

        char path[PATH_MAX], target[512];
        snprintf(path, sizeof path, "/proc/self/fd/%s", e->d_name);
        ssize_t k = readlink(path, target, sizeof target - 1);
        if (k < 0) {
            int re = errno;
            snprintf(target, sizeof target, "(readlink 失败 errno=%d)", re);
        } else {
            target[k] = '\0';
        }

        struct stat st;
        memset(&st, 0, sizeof st);
        int ok = (fstat(atoi(e->d_name), &st) == 0);
        printf("  %-4s %-11s %-52s %s\n", e->d_name,
               ok ? kind_of(target, &st) : "fstat 失败",
               target,
               ok ? "见下" : "-");
    }
    closedir(d);
    if (verbose)
        printf("  → 条目数 = %d\n", n);
    return n;
}

int main(void)
{
    /* 先全部初始化为 -1，避免「分配失败还用未初始化 fd 去 close」 */
    int fd_file = -1, p[2] = { -1, -1 }, sv[2] = { -1, -1 }, ev = -1;

    /* ---------- ① 裸快照 ---------- */
    printf("== ① 进程启动时的 fd 快照 ==\n");
    int before = snapshot(1);
    printf("  ⚠️ 这里有一条 fd 的 readlink 结果是 \"/proc/%d/fd\" —— 那就是\n", (int) getpid());
    printf("     opendir(\"/proc/self/fd\") 自己占的那个 fd。**它不是泄漏**：\n");
    printf("     你只要在枚举 fd 目录，它就一定在里面。\n");
    printf("     要「干净」的清单，就在遍历时按 fd 号把自己那一条跳过。\n\n");

    /* ---------- ② 主动开六条 fd ---------- */
    printf("== ② 主动开 6 条 fd（1+2+2+1），看它们怎么出现 ==\n");
    fd_file = open("/proc/version", O_RDONLY);
    printf("  open(\"/proc/version\", O_RDONLY) = %d\n", fd_file);

    if (pipe(p) == 0)
        printf("  pipe() → fd %d(读端) + fd %d(写端)\n", p[0], p[1]);
    else
        printf("  pipe() 失败 errno=%d(%s)\n", errno, strerror(errno));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0)
        printf("  socketpair(AF_UNIX, SOCK_STREAM) → fd %d + fd %d\n", sv[0], sv[1]);
    else
        printf("  socketpair() 失败 errno=%d(%s)\n", errno, strerror(errno));

    ev = eventfd(0, EFD_CLOEXEC);
    printf("  eventfd(0, EFD_CLOEXEC) = %d\n", ev);

    printf("\n  再快照一次：\n");
    int after = snapshot(1);
    printf("  ⚠️ 四类 target 的写法不一样，这就是判据：\n");
    printf("       磁盘/伪文件 → 绝对路径（如 /proc/version）\n");
    printf("       socket      → \"socket:[inode]\"        —— 不是路径\n");
    printf("       pipe        → \"pipe:[inode]\"          —— 同上\n");
    printf("       eventfd     → \"anon_inode:[eventfd]\"  —— 同上\n");
    printf("     fstat() 对后三者分别给 S_ISSOCK / S_ISFIFO / S_ISREG；\n");
    printf("     **anon_inode 与磁盘普通文件同样都是 S_IFREG**，光看 st_mode 分不出来。\n");
    printf("     所以「target 是不是真路径」只需看 `target[0] == '/'`，一个字符就够；\n");
    printf("     用 stat 去判类型是绕远路而且不准。\n\n");

    /* ---------- ③ 两次快照的差 ---------- */
    printf("== ③ 快照只是某一瞬间的像 ==\n");
    printf("  第一次 = %d 个，第二次 = %d 个，差值 = %d（应为 6 = 1+2+2+1）\n",
           before, after, after - before);
    printf("  ⚠️ 多线程程序里两次遍历之间别的线程随时可能 open/close，数量就会变。\n");
    printf("     「用 fd 目录做泄漏检测」只能是「采样 + 看趋势」，**不能**写成\n");
    printf("     「两次数量相等才算正常」——那是必然误报的判据。\n\n");

    /* ---------- ④ fd 目录 vs fdinfo ---------- */
    printf("== ④ 要偏移量和旗标，得看 /proc/self/fdinfo/N ==\n");
    if (fd_file != -1) {
        char path[64];
        snprintf(path, sizeof path, "/proc/self/fdinfo/%d", fd_file);
        FILE *fp = fopen(path, "r");
        if (fp == NULL) {
            printf("  fopen(%s) 失败 errno=%d(%s)\n", path, errno, strerror(errno));
        } else {
            printf("  %s 的内容：\n", path);
            char line[256];
            while (fgets(line, sizeof line, fp))
                printf("    %s", line);
            fclose(fp);
        }
    }
    printf("  ⚠️ pos 是当前文件偏移量、flags 是 open(2) 旗标（八进制）、mnt_id 是挂载号。\n");
    printf("     读 fdinfo 不会推进该 fd 自己的 pos —— 它是从内核结构另开一条路读的。\n\n");

    /* ---------- ⑤ 与 Ch11 的 fd 上限衔接 ---------- */
    printf("== ⑤ 与 Ch11 的 fd 上限衔接 ==\n");
    printf("  sysconf(_SC_OPEN_MAX) = %ld\n", sysconf(_SC_OPEN_MAX));
    printf("  ⚠️ 它等于 getrlimit(RLIMIT_NOFILE).rlim_cur，**不是恒定值**\n");
    printf("     （Ch11 已实测：soft 一改它立刻跟着变，与 man page 的说法不符）。\n");
    printf("     所以「还差多少才用完」的阈值必须运行时算，不能写死 1024。\n\n");

    /* ---------- ⑥ 关闭后回落 ---------- */
    printf("== ⑥ 关掉再快照一次，验证数量回落 ==\n");
    if (fd_file != -1)
        close(fd_file);
    if (p[0] != -1) {
        close(p[0]);
        close(p[1]);
    }
    if (sv[0] != -1) {
        close(sv[0]);
        close(sv[1]);
    }
    if (ev != -1)
        close(ev);
    int last = snapshot(0);
    printf("  关闭后条目数 = %d（第一次是 %d，应相同）\n", last, before);
    printf("  ⚠️ 关闭时**只按返回的 fd 号关**：不要假设 pipe()/socketpair() 给的\n");
    printf("     两个 fd 号一定连续（p[0]+1 == p[1]）。标准没这么保证，\n");
    printf("     在别的线程并发 open 时它确实会不连续。\n");
    return EXIT_SUCCESS;
}
