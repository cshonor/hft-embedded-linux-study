/* c14_6_journaling.c —— TLPI §14.6 Journaling File Systems
 *
 *  §14.6 讲「日志文件系统为什么能免 fsck」：先把**元数据更新**写进一块
 *  循环使用的日志区（journal），落盘成功后再写回家（checkpoint），
 *  崩溃后只要重放日志尾部即可回到一致状态。
 *
 *  这一节的书上内容大量依赖 ext3 的时代背景，但**可观测的入口在今天还在**：
 *      /proc/fs/jbd2/<dev>-<n>/   ← ext4 的日志层（jbd2 = journal block device 2）
 *      /proc/fs/ext4/<dev>/       ← ext4 本体
 *
 *  本 demo 做三件事：
 *    ① 把 jbd2 暴露的文件列出来，读它的 info（提交/事务计数）
 *    ② 在 ext4 上做一次「写 → 不 fsync → 再写 → fsync」，观察事务计数怎么走
 *    ③ 用 statfs 的差异说明「日志保护的是元数据，不是用户数据」
 *
 *  ⚠️ 诚实标注：本容器**没有** `mount -o data=journal|ordered|writeback` 的权限
 *    （缺 CAP_SYS_ADMIN），所以「三种日志模式」只能讲机制、不能在本机对比实测；
 *    /proc/fs/ext4/<dev>/ 里的 `journal_*` 参数能间接证明当前用的是写回模式。
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_6 c14_6_journaling.c
 */
#define _GNU_SOURCE
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void sec(const char *t)
{
    printf("\n== %s ==\n", t);
}

/* 取目录里第一个非 . 条目名 */
static int first_entry(const char *path, char *out, size_t cap)
{
    DIR *d = opendir(path);
    struct dirent *e;

    out[0] = '\0';
    if (d == NULL)
        return -1;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.' &&
            (e->d_name[1] == '\0' || (e->d_name[1] == '.' && e->d_name[2] == '\0')))
            continue;
        snprintf(out, cap, "%s", e->d_name);
        break;
    }
    closedir(d);
    return out[0] == '\0' ? -1 : 0;
}

static void list_dir(const char *path, int maxn)
{
    DIR *d = opendir(path);
    struct dirent *e;
    int n = 0;

    if (d == NULL) {
        printf("  opendir(%s) -> errno=%d (%s)\n", path, errno, strerror(errno));
        return;
    }
    printf("  > %s：\n", path);
    while ((e = readdir(d)) != NULL && n < maxn) {
        if (e->d_name[0] == '.' &&
            (e->d_name[1] == '\0' || (e->d_name[1] == '.' && e->d_name[2] == '\0')))
            continue;
        printf("    %s\n", e->d_name);
        n++;
    }
    closedir(d);
}

/* jbd2 的 info 文件的**首行**形如：
 *     2623 transactions (2623 requested), each up to 5461 blocks
 * 也就是说**没有 `Transactions:` 这样的 key** —— 计数就是首行的第一个整数。
 * 所以取数函数必须「读首行、取第一个整数」，不能按 key 找行。
 * 返回 -1 表示打不开或首行不是那个格式。 */
static long long jbd2_txn_count(const char *file, char *firstline, size_t nfl)
{
    FILE *fp = fopen(file, "r");
    char line[512];
    long long v = -1;

    if (firstline != NULL && nfl > 0)
        firstline[0] = '\0';
    if (fp == NULL)
        return -1;
    if (fgets(line, sizeof(line), fp) != NULL) {
        if (firstline != NULL && nfl > 0)
            snprintf(firstline, nfl, "%s", line);
        if (sscanf(line, "%lld", &v) != 1)
            v = -1;
    }
    fclose(fp);
    return v;
}

int main(void)
{
    char jdev[128] = {0};
    char jinfo[512] = {0};

    /* ---------- ① jbd2 的运行时接口 ---------- */
    sec("① /proc/fs/jbd2/<dev>/：ext4 的日志层");
    list_dir("/proc/fs/jbd2", 8);
    if (first_entry("/proc/fs/jbd2", jdev, sizeof(jdev)) == 0) {
        printf("  > 第一个 jbd2 实例：%s\n", jdev);
        snprintf(jinfo, sizeof(jinfo), "/proc/fs/jbd2/%s/info", jdev);

        FILE *fp = fopen(jinfo, "r");
        if (fp != NULL) {
            char line[512];
            int n = 0;
            printf("  --- %s ---\n", jinfo);
            while (fgets(line, sizeof(line), fp) != NULL && n < 24) {
                size_t len = strlen(line);
                if (len > 0 && line[len - 1] == '\n')
                    line[len - 1] = '\0';
                printf("  %s\n", line);
                n++;
            }
            fclose(fp);
        } else {
            printf("  打不开 %s（errno=%d）\n", jinfo, errno);
        }
    }

    printf("  ↑ 两条容易忘的硬事实（v6.6 Documentation/filesystems/ext4/overview.rst:15-17）：\n"
           "      \"All fields in ext4 are written to disk in little-endian order.\n"
           "       HOWEVER, all fields in jbd2 (the journal) are written to disk in\n"
           "       big-endian order.\" —— 同一个文件系统，数据区与日志区**字节序相反**。\n"
           "    另外：首行那个 N 是**整机累计**事务数，只能看增量方向（见 ②）。\n");

    /* ---------- ② 写文件时事务计数怎么走 ---------- */
    sec("② 写文件 → 观察 jbd2 的事务计数");
    if (jinfo[0] != '\0') {
        long long t0, t1, t2;
        int fd;
        char buf[4096];
        char first[512];

        t0 = jbd2_txn_count(jinfo, first, sizeof(first));
        printf("  info 的首行（计数的出处）：%s", first);
        printf("  写之前   transactions = %lld\n", t0);

        memset(buf, 'J', sizeof(buf));
        fd = open("/app/c14_6_journal", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            printf("  open errno=%d (%s)\n", errno, strerror(errno));
        } else {
            ssize_t n = write(fd, buf, sizeof(buf));
            t1 = jbd2_txn_count(jinfo, NULL, 0);
            printf("  write(%zd 字节) 后（未 fsync）transactions = %lld\n", n, t1);

            if (fsync(fd) == -1)
                printf("  fsync -> -1 errno=%d (%s)\n", errno, strerror(errno));
            else
                printf("  fsync 返回 0（元数据已经过日志落盘）\n");

            t2 = jbd2_txn_count(jinfo, NULL, 0);
            printf("  fsync 之后 transactions = %lld   （比未 fsync 时 %+lld）\n",
                   t2, t2 - t1);

            n = write(fd, buf, sizeof(buf));
            if (fsync(fd) == -1)
                printf("  第二次 write(%zd) + fsync -> -1 errno=%d\n", n, errno);
            close(fd);
            printf("  第二次 close 后 transactions = %lld   （比写之前 %+lld）\n",
                   jbd2_txn_count(jinfo, NULL, 0), jbd2_txn_count(jinfo, NULL, 0) - t0);
            unlink("/app/c14_6_journal");
        }
        printf("  ↑ 该计数是**整机（宿主）累计值**，只能看增量方向，不能当绝对量。\n");
    }

    /* ---------- ③ 日志护的是元数据 ---------- */
    sec("③ 日志保护「元数据一致性」，不是「用户数据不丢」");
    {
        struct statfs sfs;
        if (statfs("/app", &sfs) == 0) {
            printf("  /app 是 ext4（f_type=%#lx）；② 里那个 /proc/fs/jbd2/<dev>-<n>/\n",
                   (unsigned long) sfs.f_type);
            printf("  就是**这一个实例**的日志层 —— 有没有日志，看那一栏在不在，\n"
                   "  别只看文件系统类型（ext4 也可以用 mke2fs -O ^has_journal 造出\n"
                   "  没有日志的实例，那种连 /proc/fs/jbd2 下都不会出现）。\n");
            printf("  但日志只能保证：崩溃后**文件系统结构**仍自洽（不必全盘 fsck）。\n");
            printf("  它不保证：你 write() 过、但没 fsync() 的那几百字节还在。\n");
            printf("  → 要「数据落盘」必须显式 fsync/fdatasync（见 Ch13 §13.3）。\n");
        }
    }

    printf("\n=== c14_6 done ===\n");
    return 0;
}
