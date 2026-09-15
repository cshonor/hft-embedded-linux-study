/* c14_7_mount_points.c —— TLPI §14.7 Single Directory Hierarchy and Mount Points
 *
 *  §14.7 的两个核心概念——「单一目录树」与「挂载点遮盖」——在用户态
 *  唯一的权威入口是 /proc/self/mountinfo。它不是一张平表，而是**一棵树**：
 *
 *    字段 1  = mount ID
 *    字段 2  = parent ID      → 这两个字段拼出挂载树的父子结构
 *    字段 3  = major:minor    → 设备号
 *    字段 4  = root           → 被挂载子树的**根**在源设备上的路径
 *    字段 5  = mount point    → 挂在当前目录树的哪个位置
 *    字段 6  = per-mount options（单挂载点选项）
 *    之后    = 0 个或多个 optional fields（propagation 信息）
 *    " - "   = 分隔符
 *    之后    = fstype / source / super options
 *
 *  本 demo 用四个实验把这棵树讲清：
 *    ① 取一条逐字段拆解
 *    ② 列出所有挂载点（mount point ← fstype source）
 *    ③ **同一设备挂到多个挂载点** —— bind mount 最硬的指纹
 *    ④ 两个路径 st_ino 相同 —— 证明是同一棵子树的两次投射
 *      （外加一个「单文件被 bind 覆盖」的真实例子：/etc/passwd）
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_7 c14_7_mount_points.c
 */
#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 4096
#define MAXREC  256

static void sec(const char *t)
{
    printf("\n== %s ==\n", t);
}

struct rec {
    char id[16], parent[16], dev[32], root[512], mp[512], opts[256];
    char fstype[64], source[256], sopts[512], optf[256];
};

static int split_line(char *line, struct rec *r)
{
    char *tok[64];
    int ntok = 0;
    char *p = line;
    int dash = -1;

    memset(r, 0, sizeof(*r));
    /* 按空格切 */
    while (*p != '\0' && ntok < 64) {
        while (*p == ' ')
            p++;
        if (*p == '\0')
            break;
        tok[ntok++] = p;
        while (*p != '\0' && *p != ' ')
            p++;
        if (*p != '\0')
            *p++ = '\0';
    }
    for (int i = 0; i < ntok; i++)
        if (!strcmp(tok[i], "-")) { dash = i; break; }
    if (ntok < 6 || dash < 6 || dash + 3 >= ntok) {
        /* dash+3 > ntok 时 source 可能为空，仍接受 */
        if (ntok < 6 || dash < 6)
            return -1;
    }
    snprintf(r->id, sizeof(r->id), "%s", tok[0]);
    snprintf(r->parent, sizeof(r->parent), "%s", tok[1]);
    snprintf(r->dev, sizeof(r->dev), "%s", tok[2]);
    snprintf(r->root, sizeof(r->root), "%s", tok[3]);
    snprintf(r->mp, sizeof(r->mp), "%s", tok[4]);
    snprintf(r->opts, sizeof(r->opts), "%s", tok[5]);
    /* 6..dash-1 是 optional fields，用逗号串起来 */
    r->optf[0] = '\0';
    for (int i = 6; i < dash; i++) {
        if (i > 6 && strlen(r->optf) + 1 < sizeof(r->optf))
            strncat(r->optf, ",", sizeof(r->optf) - strlen(r->optf) - 1);
        if (strlen(r->optf) + strlen(tok[i]) + 1 < sizeof(r->optf))
            strncat(r->optf, tok[i], sizeof(r->optf) - strlen(r->optf) - 1);
    }
    if (dash + 1 < ntok)
        snprintf(r->fstype, sizeof(r->fstype), "%s", tok[dash + 1]);
    if (dash + 2 < ntok)
        snprintf(r->source, sizeof(r->source), "%s", tok[dash + 2]);
    if (dash + 3 < ntok)
        snprintf(r->sopts, sizeof(r->sopts), "%s", tok[dash + 3]);
    return 0;
}

static int load(struct rec *recs)
{
    FILE *fp = fopen("/proc/self/mountinfo", "r");
    char line[MAXLINE];
    int n = 0;

    if (fp == NULL) {
        printf("  /proc/self/mountinfo 打不开 errno=%d (%s)\n", errno, strerror(errno));
        return 0;
    }
    while (fgets(line, sizeof(line), fp) != NULL && n < MAXREC) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';
        if (split_line(line, &recs[n]) == 0)
            n++;
    }
    fclose(fp);
    return n;
}

int main(void)
{
    static struct rec recs[MAXREC];
    int n = load(recs);

    if (n == 0) {
        printf("  没读到挂载信息\n");
        return 0;
    }

    /* ---------- ① 逐字段拆解 ---------- */
    sec("① 一条 mountinfo 的逐字段拆解");
    for (int i = 0; i < n; i++) {
        if (!strcmp(recs[i].fstype, "ext4")) {
            struct rec *r = &recs[i];
            printf("  原始字段：\n");
            printf("    mount ID     = %s\n", r->id);
            printf("    parent ID    = %s\n", r->parent);
            printf("    major:minor  = %s\n", r->dev);
            printf("    root         = %s\n", r->root);
            printf("    mount point  = %s\n", r->mp);
            printf("    mnt options  = %s\n", r->opts);
            printf("    opt fields   = %s\n", r->optf[0] ? r->optf : "（无）");
            printf("    fstype       = %s\n", r->fstype);
            printf("    source       = %s\n", r->source);
            printf("    super opts   = %s\n", r->sopts);
            break;
        }
    }

    /* ---------- ② 全部挂载点 ---------- */
    sec("② 全部挂载点（mount_point ← fstype source）");
    for (int i = 0; i < n; i++)
        printf("  %-40s %-10s %s\n", recs[i].mp, recs[i].fstype,
               recs[i].source[0] ? recs[i].source : "(无)");
    printf("  共 %d 个挂载点\n", n);

    /* ---------- ③ 同一设备、多个挂载点 ---------- */
    sec("③ 同一设备挂到多个挂载点 = bind mount 的指纹");
    {
        int used[MAXREC];
        memset(used, 0, sizeof(used));
        for (int i = 0; i < n; i++) {
            int cnt = 0;
            char list[1024];

            if (used[i])
                continue;
            list[0] = '\0';
            for (int j = 0; j < n; j++) {
                if (!strcmp(recs[i].dev, recs[j].dev)) {
                    used[j] = 1;
                    cnt++;
                    if (strlen(list) + strlen(recs[j].mp) + 2 < sizeof(list)) {
                        if (list[0] != '\0')
                            strncat(list, " ", sizeof(list) - strlen(list) - 1);
                        strncat(list, recs[j].mp, sizeof(list) - strlen(list) - 1);
                    }
                }
            }
            if (cnt > 1) {
                printf("  %-9s 出现 %2d 次：%s\n", recs[i].dev, cnt, list);
                if (strlen(list) > 200)
                    printf("           （列表已截断）\n");
            }
        }
    }

    /* ---------- ④ 两条路径一个 inode ---------- */
    sec("④ 两个挂载点的 st_ino 相同 → 同一棵子树被投射了两次");
    {
        const char *pairs[][2] = {
            {"/lib", "/usr/lib"},
            {"/lib64", "/usr/lib64"},
            {"/lib32", "/usr/lib32"},
        };
        for (size_t k = 0; k < sizeof(pairs) / sizeof(pairs[0]); k++) {
            struct stat a, b;
            if (stat(pairs[k][0], &a) == 0 && stat(pairs[k][1], &b) == 0) {
                printf("  %-9s dev=%u:%-5u ino=%llu\n", pairs[k][0],
                       major(a.st_dev), minor(a.st_dev), (unsigned long long) a.st_ino);
                printf("  %-9s dev=%u:%-5u ino=%llu   %s\n", pairs[k][1],
                       major(b.st_dev), minor(b.st_dev), (unsigned long long) b.st_ino,
                       (a.st_ino == b.st_ino && a.st_dev == b.st_dev)
                           ? "<<< 同一个 inode" : "（不同）");
            } else {
                printf("  %-9s 或 %-9s stat 失败\n", pairs[k][0], pairs[k][1]);
            }
        }
    }

    /* ---------- ⑤ 挂载遮盖 ---------- */
    sec("⑤ 挂载遮盖：被挂载点下面原来是什么，用户永远看不到");
    {
        struct stat st;
        for (int i = 0; i < n; i++) {
            if (strstr(recs[i].root, "//deleted") != NULL) {
                printf("  %-24s root=%-32s fstype=%s source=%s\n",
                       recs[i].mp, recs[i].root, recs[i].fstype, recs[i].source);
            }
        }
        if (stat("/etc/passwd", &st) == 0)
            printf("  stat(/etc/passwd) 看到的是 dev=%u:%u ino=%llu —— 即上面那条挂载的内容\n",
                   major(st.st_dev), minor(st.st_dev), (unsigned long long) st.st_ino);
        printf("  ↑ root 里的 \"//deleted\" 表示源在宿主上**已经被删除**，只剩打开着的引用；\n");
        printf("    这是「用挂载把单个文件换掉」的手法（容器给 /etc/passwd 注入口令的做法）。\n");
    }

    /* ---------- ⑥ stat().st_dev 与 mountinfo 的 major:minor 是同一个数 ---------- */
    sec("⑥ 核对：stat(\"/app\").st_dev 与 mountinfo 里 /app 那条的 major:minor");
    {
        struct stat st;
        int hit = 0;

        for (int i = 0; i < n; i++) {
            if (strcmp(recs[i].mp, "/app") != 0)
                continue;
            hit = 1;
            printf("  mountinfo: /app 这一条的 major:minor = %s   （root=%s, fstype=%s）\n",
                   recs[i].dev, recs[i].root, recs[i].fstype);
        }
        if (!hit)
            printf("  mountinfo 里没有 /app 这一条\n");

        if (stat("/app", &st) == 0) {
            printf("  stat():    stat(\"/app\").st_dev 解出来 = %u:%u\n",
                   major(st.st_dev), minor(st.st_dev));
            printf("  ↑ 两个字段说的是**同一个 sb->s_dev**，不是两种编号：\n"
                   "    mountinfo 打印的是原始的 MAJOR:MINOR（fs/proc_namespace.c），\n"
                   "    stat() 交给你的是**编码过**的值（fs/stat.c 用 new_encode_dev），\n"
                   "    glibc 的 major()/minor() 就是那个编码的逆运算（__gnu_dev_major）。\n"
                   "    所以同一台机器上它们必然相等 —— 对不上就说明**不是同一个容器**。\n");
        } else {
            printf("  stat(/app) 失败 errno=%d (%s)\n", errno, strerror(errno));
        }
    }

    printf("\n=== c14_7 done ===\n");
    return 0;
}
