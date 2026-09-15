/* ex12_1_proc_walk.c — Ch12 习题 12-1（page 231）的自写版
 *
 * 原书习题：写一个程序，遍历 /proc/PID 找出属于指定用户的进程，打印 PID 与命令。
 * 原书解答放在 `procfs_user_exe.c`（本目录也有一份逐字镜像），但它依赖
 * `lib/ugid_functions.h` 的 userIdFromName()。本文件把那个函数**自己写一遍**，
 * 于是它零依赖、单文件就能编译 —— 顺便把「用户名字符串怎么变成 UID」这件事
 * 摊开看清楚。
 *
 * 比解答版多做的两件事：
 *   ① 用 `status` 拿 Name/Uid/PPid 之后，**再**用 `stat` 拿一次 comm，并演示
 *      「第一个 '(' 配最后一个 ')'」这个解析规则 —— 进程名里带 ')' 时朴素写法
 *      会把后面 52 个字段全部读错位；
 *   ② 统计扫描 / 跳过 / 命中的条数，让「/proc 是活的，随时有进程消失」这个
 *      竞态变成可观察的数字，而不是一句注释。
 *
 * 编译： gcc -O0 -Wall -Wextra -o ex12_1_proc_walk ex12_1_proc_walk.c
 * 用法： ./ex12_1_proc_walk [username|uid]       不带参数 = 列出全部进程
 * 取材： man-pages 6.19 proc(5)（/proc/PID/status 的 Name/Uid/PPid 字段；
 *         /proc/PID/stat 的 "(comm)" 字段与 "comm 最长 15 字符、可能含空格/括号"）
 *       Linux v6.6 fs/proc/array.c（proc_pid_status / proc_pid_stat 的字段顺序）
 *                      include/linux/sched.h（TASK_COMM_LEN = 16）
 *       lib/ugid_functions.c:30-49（userIdFromName 的语义：数字串短路 + getpwnam）
 *       TLPI 习题 12-1 解答 = sysinfo/procfs_user_exe.c（原书未印全文）
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LINE 1000

/* ---- 原书 lib/ugid_functions.c:30-49 的 userIdFromName，语义逐条照搬 ---- */
static uid_t uid_from_name(const char *name)
{
    struct passwd *pwd;
    uid_t u;
    char *endptr;

    if (name == NULL || *name == '\0')
        return (uid_t) -1;              /* NULL 或空串 → 错误 */

    u = strtol(name, &endptr, 10);      /* 先当数字试 */
    if (*endptr == '\0')
        return u;                       /* 纯数字串：**不查 passwd 直接返回** */

    pwd = getpwnam(name);               /* 否则查 passwd */
    if (pwd == NULL)
        return (uid_t) -1;

    return pwd->pw_uid;
}

/* ---- 从 /proc/PID/status 里取三个字段；全部拿到就提前返回 ---- */
struct pinfo {
    char name[64];
    uid_t uid;
    int ppid;
    int got_name, got_uid, got_ppid;
};

static void read_status(const char *pid, struct pinfo *pi)
{
    char path[PATH_MAX];
    snprintf(path, sizeof path, "/proc/%s/status", pid);

    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return;                 /* 竞态：进程可能刚消失 —— 原书注释也是这么说的 */

    char line[MAX_LINE];
    while (fgets(line, sizeof line, fp)) {
        if (!pi->got_name && strncmp(line, "Name:", 5) == 0) {
            char *p = line + 5;
            while (*p && isspace((unsigned char) *p))
                p++;
            snprintf(pi->name, sizeof pi->name, "%s", p);
            char *nl = strchr(pi->name, '\n');
            if (nl)
                *nl = '\0';
            pi->got_name = 1;
        } else if (!pi->got_uid && strncmp(line, "Uid:", 4) == 0) {
            pi->uid = (uid_t) strtol(line + 4, NULL, 10);   /* 第 1 个 = real UID */
            pi->got_uid = 1;
        } else if (!pi->got_ppid && strncmp(line, "PPid:", 5) == 0) {
            pi->ppid = (int) strtol(line + 5, NULL, 10);
            pi->got_ppid = 1;
        }
        if (pi->got_name && pi->got_uid && pi->got_ppid)
            break;
    }
    fclose(fp);
}

/* ---- 从 /proc/PID/stat 里取 comm：第一个 '(' 配**最后一个** ')' ----
   返回实际切出来的长度；同时把「朴素写法」的结果也返回出来做对比。 */
static int read_stat_comm(const char *pid, char *out, size_t outsz, char *naive, size_t nsz)
{
    char path[PATH_MAX], line[MAX_LINE];

    snprintf(path, sizeof path, "/proc/%s/stat", pid);
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return -1;
    if (fgets(line, sizeof line, fp) == NULL) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    char *lp = strchr(line, '(');            /* 第一个 '(' */
    char *rp = strrchr(line, ')');           /* 最后一个 ')' */
    if (lp == NULL || rp == NULL || rp <= lp)
        return -1;

    size_t n = (size_t) (rp - lp - 1);
    if (n >= outsz)
        n = outsz - 1;
    memcpy(out, lp + 1, n);
    out[n] = '\0';

    /* 反面教材：第一个 ')' 就收手 */
    char *first_rp = strchr(line, ')');
    if (first_rp != NULL && first_rp > lp) {
        size_t k = (size_t) (first_rp - lp - 1);
        if (k >= nsz)
            k = nsz - 1;
        memcpy(naive, lp + 1, k);
        naive[k] = '\0';
    } else {
        naive[0] = '\0';
    }
    return (int) n;
}

/* ---- 现场把本进程的 comm 改成 "a)b"，让陷阱可复现 ---- */
static void set_comm_evil(void)
{
    if (prctl(PR_SET_NAME, "a)b", 0, 0, 0) == 0)
        printf("（演示用：已把本进程 comm 改成 \"a)b\"，请在第 ② 段输出里找自己）\n");
}

int main(int argc, char *argv[])
{
    /* ---------- ① 用户名 → UID ---------- */
    printf("== ① 用户名怎么变成 UID ==\n");
    const char *who = (argc > 1) ? argv[1] : NULL;
    uid_t want = (uid_t) -2;          /* -2 = 「不过滤」 */

    if (who == NULL) {
        printf("  未给参数 → 列出**全部**进程\n");
    } else {
        want = uid_from_name(who);
        printf("  uid_from_name(\"%s\") = ", who);
        if (want == (uid_t) -1)
            printf("-1（查不到）\n");
        else
            printf("%u\n", (unsigned) want);
        printf("  解析规则：\n");
        printf("    * 纯数字串（strtol 后 *endptr=='\\0'）→ 直接当 UID 用，**不查 passwd**\n");
        printf("    * 其它字符串 → getpwnam()，查不到返回 (uid_t)-1\n");
        printf("    * NULL / 空串 → 直接 (uid_t)-1\n");
        if (want == (uid_t) -1) {
            printf("  → 无法解析，退出\n");
            return EXIT_SUCCESS;
        }
    }
    printf("\n");

    /* 让 comm 里带上 ')'，好让第 ② 段的自指行复现解析陷阱 */
    struct stat st_test;
    if (stat("/proc/self/status", &st_test) == 0)
        set_comm_evil();
    printf("\n");

    /* ---------- ② 遍历 /proc ---------- */
    printf("== ② 遍历 /proc/PID ==\n");
    DIR *dirp = opendir("/proc");
    if (dirp == NULL) {
        printf("  opendir(/proc) 失败 errno=%d(%s)\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    printf("  %6s %6s %8s  %-16s %s\n", "PID", "PPid", "UID", "comm(stat)", "Name(status)");
    printf("  %6s %6s %8s  %-16s %s\n", "------", "------", "--------",
           "----------------", "-------");

    struct dirent *dp;
    int scanned = 0, skipped = 0, matched = 0, vanished = 0;
    int self_pid = (int) getpid();

    for (;;) {
        errno = 0;                      /* 区分「读完」与「出错」：原书同款 */
        dp = readdir(dirp);
        if (dp == NULL) {
            if (errno != 0)
                printf("  readdir 出错 errno=%d(%s)\n", errno, strerror(errno));
            break;
        }
        if (dp->d_type != DT_DIR || !isdigit((unsigned char) dp->d_name[0]))
            continue;                   /* 原书判断：非目录、或首字符不是数字 → 跳 */
        scanned++;

        struct pinfo pi;
        memset(&pi, 0, sizeof pi);
        read_status(dp->d_name, &pi);
        if (!pi.got_name || !pi.got_uid) {
            vanished++;                 /* 进程在扫描期间消失了 */
            continue;
        }
        if (who != NULL && pi.uid != want) {
            skipped++;
            continue;
        }
        matched++;

        char comm[256] = "", naive[256] = "";
        int cl = read_stat_comm(dp->d_name, comm, sizeof comm, naive, sizeof naive);
        if (cl < 0)
            snprintf(comm, sizeof comm, "(stat 读失败)");

        printf("  %6s %6d %8u  %-16s %s%s\n", dp->d_name, pi.ppid,
               (unsigned) pi.uid, comm, pi.name,
               (atoi(dp->d_name) == self_pid) ? "   <-- 本进程" : "");
    }
    closedir(dirp);
    printf("\n  scanned(数字目录) = %d  matched = %d  uid 不匹配跳过 = %d  扫描中消失 = %d\n",
           scanned, matched, skipped, vanished);
    printf("  ⚠️ 三个计数加起来不等于 scanned 时就是竞态发生了 —— 这正是原书在\n");
    printf("     fopen 失败处写 'Ignore errors: fopen() might fail if process has\n");
    printf("     just terminated' 的原因。\n\n");

    /* ---------- ③ comm 解析陷阱 ---------- */
    printf("== ③ /proc/PID/stat 的 comm 解析陷阱 ==\n");
    {
        char comm[256] = "", naive[256] = "";
        char pidbuf[32];
        snprintf(pidbuf, sizeof pidbuf, "%d", self_pid);
        int cl = read_stat_comm(pidbuf, comm, sizeof comm, naive, sizeof naive);
        printf("  本进程 comm（status 里读到的）= 见第 ② 段标了 <-- 本进程 的那行\n");
        printf("  stat 里切出来的 comm 长度 = %d\n", cl);
        if (cl >= 0) {
            printf("  正确切法（第一个 '(' 配**最后一个** ')'）= [%s]\n", comm);
            printf("  朴素切法（第一个 '(' 配第一个 ')'）      = [%s]\n", naive);
            printf("  → 朴素算法丢掉 %d 个字符，后面 50 个字段的编号**全部错位**\n",
                   (int) strlen(comm) - (int) strlen(naive));
        }
        printf("  ⚠️ 规则来自 proc(5)：comm 可以含空格甚至括号，所以字段分隔符\n");
        printf("     **不是**按空格切，而是靠第一个 '(' 和最后一个 ')' 定界。\n");
        printf("     解析 /proc/PID/stat 时先定位这两个字符，再对 ')' 之后的部分\n");
        printf("     按空格切第 3、4、5… 个字段，才不会串位。\n\n");
    }

    /* ---------- ④ comm 与 cmdline 不是一回事 ---------- */
    printf("== ④ comm（最长 15 字符）与 cmdline（完整 argv）不是一回事 ==\n");
    {
        char path[64], b[512];
        snprintf(path, sizeof path, "/proc/%d/cmdline", self_pid);
        FILE *fp = fopen(path, "rb");
        int n = 0;
        if (fp) {
            n = (int) fread(b, 1, sizeof b - 1, fp);
            fclose(fp);
        }
        if (n > 0) {
            b[n] = '\0';
            printf("  /proc/self/cmdline 原始 %d 字节（NUL 用 '|' 显示）：", n);
            for (int i = 0; i < n; i++)
                putchar(b[i] ? b[i] : '|');
            putchar('\n');
        }
        printf("  comm 上限是 TASK_COMM_LEN-1 = 15 个字符（include/linux/sched.h），\n");
        printf("  而 cmdline 是完整 argv。所以要按「谁在跑」过滤，用 comm；\n");
        printf("  要按「怎么启动的」过滤，用 cmdline。两者混用是常见误判来源。\n");
    }
    return EXIT_SUCCESS;
}
