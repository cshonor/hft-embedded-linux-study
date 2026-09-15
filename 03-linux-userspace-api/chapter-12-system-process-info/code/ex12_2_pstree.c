/* ex12_2_pstree.c — Ch12 习题 12-2（page 231）的自写版：画进程树
 *
 * 原书习题 12-2 要求：画出系统上所有进程的父子层级关系、一直回溯到 init，
 * 每个进程显示 PID 与正在执行的命令，输出形态类似 pstree(1)（不必那么精致）。
 * 父进程靠读所有 /proc/PID/status 的 `PPid:` 行得到，并且要处理
 * "某个进程的父进程（连同它的 /proc/PID 目录）在扫描过程中消失" 这种情况。
 *
 * 本题的三个技术点：
 *   ① 全量扫描必须容错：/proc/PID 目录随时会消失，`fopen` 失败是常态
 *      而不是异常（原书在 12-1 的解答里就写了这句注释）；
 *   ② 扫完之后要**对账**：「扫到的目录数」=「成功读到的」+「打开失败的」+
 *      「读 status 中途失败的」+「没有 PPid 行的」。只报总数不算验证；
 *   ③ **父进程不见了怎么办**：内核会把孤儿的父指针改成 1（reparent 到 init）
 *      或某个 subreaper，所以正常情况下树上不该有孤立节点；但扫描是**并发**
 *      的 —— 子进程在你扫到它之后、扫到它父进程之前就退出，是会发生的。
 *      本程序把「PPid 指向不存在的 PID」的节点单独计数，并挂到 root 下，
 *      而不是悄悄丢掉（**丢掉会让整棵子树消失**，这是 pstree 类程序的经典 bug）。
 *
 * 编译： gcc -O0 -Wall -Wextra -o ex12_2_pstree ex12_2_pstree.c
 * 取材： man-pages 6.19 proc(5)（/proc/PID/status 的 Name / Pid / PPid 字段）
 *       Linux v6.6 fs/proc/array.c（proc_pid_status 的字段顺序）
 *                      kernel/exit.c（forget_original_parent：孤儿 reparent 到 init
 *                        或最近的 subreaper）
 *       TLPI 习题 12-2 题干（原书 p.231；man7 只分发源码、不分发习题正文，
 *        本题干文字转引自第三方镜像，见笔记本节目录里的说明）
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXPROC 4096
#define NAMELEN 64

struct pinfo {
    pid_t pid;
    pid_t ppid;
    char  name[NAMELEN];
};

static struct pinfo tab[MAXPROC];
static int n = 0;                  /* 成功收集到的进程 */

/* 扫描计数（用于对账，见文件头 ②） */
static int scanned = 0;            /* /proc 下的数字目录数 */
static int open_failed = 0;        /* fopen("/proc/PID/status") 失败 */
static int unreadable = 0;         /* 读了几行但没凑齐 Name/Pid/PPid */
static int vanished = 0;           /* 扫到一半进程消失 */

/*
 * 从 /proc/<pid>/status 里抠出 Name / Pid / PPid 三行。
 * 返回 0 成功；-1 表示这个 /proc/PID 已经读不到了（进程退出）。
 *
 * 注意：**不能按行号取** —— status 的字段顺序随内核版本增删（Umask 就是
 * 较新内核才补进第 2 行的），只能按 "Key:" 前缀查。
 */
static int read_status(pid_t pid, struct pinfo *out)
{
    char path[PATH_MAX];
    snprintf(path, sizeof path, "/proc/%d/status", (int) pid);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        open_failed++;
        return -1;
    }

    char line[512];
    int got_name = 0, got_pid = 0, got_ppid = 0;

    while (fgets(line, sizeof line, fp) != NULL) {
        if (!got_name && strncmp(line, "Name:", 5) == 0) {
            char *p = line + 5;
            while (*p == ' ' || *p == '\t')
                p++;
            size_t k = strcspn(p, "\n");
            if (k >= NAMELEN)
                k = NAMELEN - 1;
            memcpy(out->name, p, k);
            out->name[k] = '\0';
            got_name = 1;
        } else if (!got_pid && strncmp(line, "Pid:", 4) == 0) {
            out->pid = (pid_t) strtol(line + 4, NULL, 10);
            got_pid = 1;
        } else if (!got_ppid && strncmp(line, "PPid:", 5) == 0) {
            out->ppid = (pid_t) strtol(line + 5, NULL, 10);
            got_ppid = 1;
        }
        if (got_name && got_pid && got_ppid)
            break;
    }

    fclose(fp);

    if (!got_name || !got_pid || !got_ppid) {
        unreadable++;
        return -1;
    }
    return 0;
}

/* 收集全系统的 {pid, ppid, name} 三元组 */
static void collect(void)
{
    DIR *dp = opendir("/proc");
    if (dp == NULL) {
        perror("opendir /proc");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        errno = 0;
        struct dirent *de = readdir(dp);
        if (de == NULL) {
            if (errno != 0)
                perror("readdir");
            break;
        }
        if (!isdigit((unsigned char) de->d_name[0]))
            continue;
        scanned++;

        pid_t pid = (pid_t) strtol(de->d_name, NULL, 10);
        if (pid <= 0)
            continue;

        struct pinfo pi;
        memset(&pi, 0, sizeof pi);
        if (read_status(pid, &pi) != 0) {
            /* 打不开或读不全 —— 进程大概率刚退出。**必须容忍**。 */
            vanished++;
            continue;
        }
        if (n < MAXPROC) {
            tab[n++] = pi;
        } else {
            fprintf(stderr, "warning: 超过 MAXPROC=%d，后续进程被丢弃\n", MAXPROC);
            break;
        }
    }
    closedir(dp);
}

/* 在表里找 pid 的下标；找不到返回 -1 */
static int find(pid_t pid)
{
    for (int i = 0; i < n; i++)
        if (tab[i].pid == pid)
            return i;
    return -1;
}

/* 数某个 pid 有多少个孩子 */
static int count_children(pid_t pid)
{
    int c = 0;
    for (int i = 0; i < n; i++)
        if (tab[i].ppid == pid && tab[i].pid != pid)
            c++;
    return c;
}

/*
 * 画树。prefix 是当前行前缀，is_last 表示「本节点是父节点的最后一个孩子」。
 *
 * 输出形态（纯 ASCII，便于 grep；pstree(1) 用的是制表符画线，语义相同）：
 *     dumb-init (1)
 *     `- output.s (2)
 */
static void show(int idx, const char *prefix, int is_last, int depth)
{
    if (depth > 64) {                        /* 防御：万一 PPid 成环 */
        printf("%s`- [深度超限，疑似 PPid 成环] (%d)\n",
               prefix, (int) tab[idx].pid);
        return;
    }

    printf("%s%s%s (%d)\n",
           prefix,
           depth == 0 ? "" : (is_last ? "`- " : "|- "),
           tab[idx].name,
           (int) tab[idx].pid);

    char sub[512];
    int remain = count_children(tab[idx].pid);

    for (int i = 0; i < n; i++) {
        if (tab[i].ppid != tab[idx].pid || tab[i].pid == tab[idx].pid)
            continue;
        remain--;
        snprintf(sub, sizeof sub, "%s%s",
                 prefix,
                 depth == 0 ? "" : (is_last ? "   " : "|  "));
        show(i, sub, remain == 0, depth + 1);
    }
}

/* 打印整片森林：先找根（PPid 为 0 或指向不存在的 PID），再逐棵画 */
static void render(const char *title)
{
    int roots = 0, orphan = 0;

    printf("%s\n", title);
    for (int i = 0; i < n; i++) {
        pid_t pp = tab[i].ppid;
        if (pp != 0 && find(pp) == -1)
            orphan++;
    }

    for (int i = 0; i < n; i++) {
        pid_t pp = tab[i].ppid;
        if (pp != 0 && find(pp) != -1)
            continue;                        /* 不是根 */
        roots++;
        show(i, "", 1, 0);
    }

    printf("  → 根节点 %d 个；其中「PPid 指向不存在的 PID」的孤立节点 %d 个\n",
           roots, orphan);
    printf("     （孤儿的父指针本应由内核 reparent 到 init/subreaper；扫到孤儿\n");
    printf("      说明扫描期间父进程先退出了 —— 这正是本题要求处理的情况）\n\n");
}

int main(void)
{
    printf("== ① 全量扫描 /proc/PID/status ==\n");
    collect();

    printf("  数字目录（扫到）      = %d\n", scanned);
    printf("  成功收集进程          = %d\n", n);
    printf("  fopen status 失败     = %d\n", open_failed);
    printf("  读不全 Name/Pid/PPid  = %d\n", unreadable);
    printf("  判定为「扫到一半消失」 = %d\n", vanished);
    printf("  ⚠️ 对账恒等式：%d(扫到) = %d(成功) + %d(打不开) + %d(读不全)\n",
           scanned, n, open_failed, unreadable);
    printf("     等式不成立就说明扫描逻辑漏了分支 —— 只报总数不算验证。\n\n");

    printf("== ② 收集到的原始三元组（无序，readdir 的顺序就是目录项顺序）==\n");
    printf("      PID   PPid  comm\n");
    printf("  -------  -----  --------------\n");
    for (int i = 0; i < n; i++)
        printf("  %7d  %5d  %s\n", (int) tab[i].pid, (int) tab[i].ppid, tab[i].name);
    printf("\n");

    printf("== ③ 画树 ==\n");
    render("  真实的进程树：");

    /*
     * 本容器里只有 2 个进程，真实树只有两层，看不出渲染算法的差别。
     * 下面用**合成数据**再画一次：一个三叉、四层的假进程表，
     * 专门用来看「|- 」/「`- 」/「|  」三种前缀分别出现在哪里。
     */
    printf("== ④ 渲染算法演示（合成数据，不是真的进程表）==\n");
    {
        struct pinfo real[MAXPROC];
        int realn = n;
        memcpy(real, tab, sizeof tab);

        struct pinfo demo[] = {
            {    1,    0, "init"        },
            {   10,    1, "sshd"        },
            {   11,    1, "systemd-journal" },
            {   12,    1, "cron"        },
            { 1024,   10, "bash"        },
            { 1025,   10, "sshd"        },
            { 1026,   11, "logrotate"   },
            { 2000, 1024, "vim"         },
            { 2001, 1024, "sleep"       },
            { 2002, 1025, "scp"         },
            { 2003, 2002, "tar"         },
        };
        n = (int) (sizeof demo / sizeof demo[0]);
        memcpy(tab, demo, sizeof demo);

        render("  合成进程表（init=1 为根，深度 4）：");

        memcpy(tab, real, sizeof tab);
        n = realn;
    }

    printf("== ⑤ 与习题 12-1 的分工 ==\n");
    printf("  12-1 读 Name: 与 Uid:，按**用户**过滤；\n");
    printf("  12-2 读 Name: 与 PPid:，按**父子关系**组织；\n");
    printf("  12-3 读 /proc/PID/fd/* 符号链接，按**打开的文件**过滤。\n");
    printf("  三题都要遍历 /proc/PID，都要容忍进程随时消失，只是「索引维度」不同。\n");
    printf("  共同底线：**单个 /proc/PID 读失败绝不能中止整个扫描**。\n");

    return EXIT_SUCCESS;
}
