/* c12_1_proc_pid.c — Ch12 §12.1.1：/proc/PID/ 下到底有什么，以及两个真实的解析陷阱
 *
 * TLPI §12.1.1 讲「拿进程信息」，本程序做三件事：
 *   ① 把 /proc/self/ 下的主力文件逐个读一遍，看真实形态；
 *   ② 演示 **`/proc/PID/stat` 的 `(comm)` 字段解析陷阱**——
 *      用 prctl(PR_SET_NAME) 把进程名改成含 ')' 的串，再用「朴素 strchr(')')」
 *      和「第一个 '(' 配最后一个 ')'」两种切法对比，看差了多少字符；
 *   ③ 演示 **`/proc/PID/status` 的字段顺序不是直觉顺序**——
 *      第 2 行不是 Pid，是 Umask。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c12_1_proc_pid c12_1_proc_pid.c
 * 取材： man-pages 6.19 proc(5) 的 /proc/pid/stat 与 /proc/pid/status 两节
 *       Linux v6.6 fs/proc/array.c（do_task_stat 的 `%d (%s) %c` 与 status 的
 *                        seq_printf 顺序：Name/Umask/State/Tgid/Ngid/Pid/PPid）
 *                      fs/proc/base.c（proc_pid_operations 注册了哪些文件）
 *                      fs/proc/task_mmu.c（maps 的 show_map_vma）
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>

/* 读一个文件的前 max 行；line_no 从 1 开始，打印时带序号 */
static void head_lines(const char *path, int max)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf("  %-26s 打开失败 errno=%d(%s)\n", path, errno, strerror(errno));
        return;
    }
    char line[512];
    int n = 0;
    while (n < max && fgets(line, sizeof(line), fp) != NULL) {
        size_t l = strlen(line);
        while (l && (line[l - 1] == '\n' || line[l - 1] == '\r'))
            line[--l] = '\0';
        printf("  %-26s [%d] %s\n", n == 0 ? path : "", n + 1, line);
        n++;
    }
    if (n == 0)
        printf("  %-26s (空)\n", path);
    fclose(fp);
}

/* 读整个小文件，把 NUL 显示成 '|' —— 看清 cmdline/environ 的分隔符 */
static void raw_nul(const char *path, int maxenv)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("  %-26s 打开失败 errno=%d(%s)\n", path, errno, strerror(errno));
        return;
    }
    static char buf[8192];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        printf("  %-26s 读出 %zd 字节\n", path, n);
        return;
    }
    buf[n] = '\0';
    printf("  %-26s 共 %zd 字节，NUL 显示为 '|'：", path, n);
    for (ssize_t i = 0; i < n; i++)
        putchar(buf[i] == '\0' ? '|' : buf[i]);
    printf("\n");

    int cnt = 0;
    for (ssize_t i = 0; i < n; i++)
        if (buf[i] == '\0')
            cnt++;
    printf("  %-26s NUL 个数 = %d（= 条目数：每条都以 \\0 结尾）\n", "", cnt);
    if (maxenv > 0) {
        int shown = 0;
        for (ssize_t i = 0; i < n && shown < maxenv; i++) {
            if (i == 0 || buf[i - 1] == '\0') {
                printf("        %s\n", buf + i);
                shown++;
            }
        }
    }
}

static void show_link(const char *path)
{
    char t[PATH_MAX];
    errno = 0;
    ssize_t n = readlink(path, t, sizeof(t) - 1);
    if (n == -1)
        printf("  %-26s readlink 失败 errno=%d(%s)\n", path, errno, strerror(errno));
    else {
        t[n] = '\0';
        printf("  %-26s -> %s\n", path, t);
    }
}

int main(void)
{
    /* ---------- ① 主力文件逐个看 ---------- */
    printf("== ① /proc/self/status 的前 8 行（注意字段顺序）==\n");
    head_lines("/proc/self/status", 8);
    printf("  ⚠️ 第 2 行是 **Umask**，不是 Pid；真 Pid 在第 6 行。\n");
    printf("     顺序由内核 fs/proc/array.c 的 seq_printf 调用次序决定，\n");
    printf("     跨内核版本可能增删（Umask 就是较新内核才加的），别按行号取。\n\n");

    printf("== ② /proc/self/stat 是「单行 + 空格分隔」，第 2 字段是 (comm) ==\n");
    head_lines("/proc/self/stat", 1);
    printf("  ⚠️ comm 被一对括号包住，且 comm 里的空格**不算分隔符**。\n");
    printf("     注意那一片 0 里夹着 18446744073709551615 = RLIM_INFINITY，\n");
    printf("     说明这些字段有的按「无符号、-1 表示无限」解释。\n\n");

    printf("== ③ statm / comm / maps / ns ==\n");
    head_lines("/proc/self/statm", 1);
    head_lines("/proc/self/comm", 1);
    {
        FILE *fp = fopen("/proc/self/maps", "r");
        if (fp != NULL) {
            char line[512];
            int n = 0;
            while (fgets(line, sizeof(line), fp) != NULL) {
                if (n < 3) {
                    size_t l = strlen(line);
                    while (l && line[l - 1] == '\n')
                        line[--l] = '\0';
                    printf("  %-26s [%d] %s\n", n == 0 ? "/proc/self/maps" : "", n + 1, line);
                }
                n++;
            }
            printf("  %-26s 共 %d 行（每行 = 一段虚拟地址区间）\n", "", n);
            fclose(fp);
        }
    }
    printf("  /proc/self/ns/ 里的命名空间（都是符号链接，inode 号即 ns 身份）：\n");
    DIR *d = opendir("/proc/self/ns");
    if (d != NULL) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (e->d_name[0] == '.')
                continue;
            char p[PATH_MAX];
            snprintf(p, sizeof(p), "/proc/self/ns/%s", e->d_name);
            show_link(p);
        }
        closedir(d);
    }
    printf("\n");

    printf("== ④ cmdline / environ：用 NUL 分隔，不是空格也不是换行 ==\n");
    raw_nul("/proc/self/cmdline", 0);
    raw_nul("/proc/self/environ", 6);
    printf("  ⚠️ 所以直接 fgets 只能拿到「第一条」；要全读得按 \\0 切。\n");
    printf("     environ 的条数每次运行都不同（CE 容器实测 7 条）。\n\n");

    printf("== ⑤ cwd / exe / root / fd/ 都是符号链接 ==\n");
    show_link("/proc/self/cwd");
    show_link("/proc/self/exe");
    show_link("/proc/self/root");
    d = opendir("/proc/self/fd");
    if (d != NULL) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (e->d_name[0] == '.')
                continue;
            char p[PATH_MAX];
            snprintf(p, sizeof(p), "/proc/self/fd/%s", e->d_name);
            show_link(p);
        }
        closedir(d);
    }
    printf("  ⚠️ 遍历 fd/ 时，`opendir` 自己的那个 fd 也在里面，\n");
    printf("     而它的目标是 \"/proc/<pid>/fd\" —— 于是你会看到一条指向自己的链接。\n\n");

    /* ---------- ⑥ comm 含 ')' 的解析陷阱 ---------- */
    printf("== ⑥ 陷阱：comm 里含 ')' 时，朴素解析切错位置 ==\n");
    printf("  prctl(PR_SET_NAME, \"a)b\") = %d",
           prctl(PR_SET_NAME, "a)b", 0, 0, 0));
    head_lines("/proc/self/comm", 1);

    FILE *fp = fopen("/proc/self/stat", "r");
    if (fp == NULL)
        return EXIT_FAILURE;
    static char stat_line[2048];
    if (fgets(stat_line, sizeof(stat_line), fp) == NULL) {
        fclose(fp);
        return EXIT_FAILURE;
    }
    fclose(fp);
    size_t l = strlen(stat_line);
    while (l && stat_line[l - 1] == '\n')
        stat_line[--l] = '\0';
    printf("  /proc/self/stat = %s\n", stat_line);

    char *lp = strchr(stat_line, '(');          /* 第一个 '(' */
    char *last = strrchr(stat_line, ')');       /* 最后一个 ')' */
    char *first = strchr(stat_line, ')');       /* 第一个 ')' —— 朴素写法的错处 */
    if (lp != NULL && last != NULL && first != NULL) {
        printf("  正确切法（第一个 '(' 配最后一个 ')'）：%.*s\n",
               (int) (last - lp + 1), lp);
        printf("  朴素切法（第一个 '(' 配第一个 ')'）：(%.*s)\n",
               (int) (first - lp - 1), lp + 1);
        printf("  → 朴素算法丢掉 %d 个字符，后面所有字段的编号**全部错位**\n",
               (int) (last - first));
    }
    printf("  ⚠️ 这就是「别自己按空格/括号切 /proc/PID/stat」的原因。\n");
    printf("     要进程名就用 /proc/PID/comm；要完整字段建议 man 5 proc 逐字段数，\n");
    printf("     或直接用现成库（libprocps / osquery），别手搓。\n");
    return EXIT_SUCCESS;
}
