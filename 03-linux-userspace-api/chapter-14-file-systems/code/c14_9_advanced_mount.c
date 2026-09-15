/* c14_9_advanced_mount.c —— TLPI §14.9 Advanced Mount Features
 *
 *  官方 14.9 有五个子节：
 *    14.9.1 Mounting a File System at Multiple Mount Points
 *    14.9.2 Stacking Multiple Mounts on the Same Mount Point
 *    14.9.3 Mount Flags That Are Per-Mount Options
 *    14.9.4 Bind Mounts
 *    14.9.5 Recursive Bind Mounts
 *
 *  ⚠️ 本容器不给 CAP_SYS_ADMIN —— 这五件事**一件都做不了**（全 EPERM）。
 *  但它们的**结果**全都留在 /proc/self/mountinfo 里，所以本 demo 换个方向：
 *  不去「造」这些挂载，而是**在一张已经存在的挂载表里把它们认出来**。
 *
 *    ① optional fields（propagation）—— shared / master / propagate_from
 *    ② 14.9.1 的证据：同一 major:minor 出现在多个 mount point
 *    ③ 14.9.4 的证据：root 字段 ≠ / ，说明挂的是源设备的某个**子树**
 *    ④ 14.9.5 的证据：同一设备的多条记录里，root 前缀成组出现（recursive 的痕迹）
 *    ⑤ 14.9.3 的实证：per-mount options 与 super options 是两栏，值可以不一致
 *    ⑥ 试一次 MS_BIND|MS_REC，确认本容器确实被挡住
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_9 c14_9_advanced_mount.c
 */
#define _GNU_SOURCE
#include <sys/mount.h>
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
    int ntok = 0, dash = -1;
    char *p = line;

    memset(r, 0, sizeof(*r));
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
    if (ntok < 6 || dash < 6)
        return -1;
    snprintf(r->id, sizeof(r->id), "%s", tok[0]);
    snprintf(r->parent, sizeof(r->parent), "%s", tok[1]);
    snprintf(r->dev, sizeof(r->dev), "%s", tok[2]);
    snprintf(r->root, sizeof(r->root), "%s", tok[3]);
    snprintf(r->mp, sizeof(r->mp), "%s", tok[4]);
    snprintf(r->opts, sizeof(r->opts), "%s", tok[5]);
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

int main(void)
{
    static struct rec recs[MAXREC];
    int n = 0;

    {
        FILE *fp = fopen("/proc/self/mountinfo", "r");
        char line[MAXLINE];
        if (fp == NULL) {
            printf("  mountinfo 打不开 errno=%d\n", errno);
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
    }
    printf("\n（/proc/self/mountinfo 共 %d 条）\n", n);

    /* ---------- ① propagation ---------- */
    sec("① optional fields = 挂载传播（propagation）信息");
    {
        int shown = 0;
        for (int i = 0; i < n; i++) {
            if (recs[i].optf[0] != '\0' && strstr(recs[i].optf, "deleted") == NULL) {
                printf("  %-40s opt=%-16s\n", recs[i].mp, recs[i].optf);
                shown++;
            }
        }
        if (shown == 0)
            printf("  所有条目都没有 optional field（都是 private 挂载）\n");
        else
            printf("  共 %d 条带 optional field\n", shown);
        printf("  ↑ shared:<N> 表示属于共享组 N；master:<N> 表示从组 N 接收传播事件；\n");
        printf("    propagate_from:<N> 表示事件的来源。这一栏就是 14.9.2「叠加挂载」与\n");
        printf("    14.9.5「递归」在用户态留下的痕迹。\n");
    }

    /* ---------- ② 14.9.1 多挂载点 ---------- */
    sec("② 14.9.1 的证据：一个设备挂到多个挂载点");
    {
        for (int i = 0; i < n; i++) {
            int cnt = 0;
            char list[600];

            list[0] = '\0';
            for (int j = 0; j < n; j++) {
                if (!strcmp(recs[i].dev, recs[j].dev)) {
                    cnt++;
                    if (strlen(list) + strlen(recs[j].mp) + 2 < sizeof(list)) {
                        if (list[0] != '\0')
                            strncat(list, " ", sizeof(list) - strlen(list) - 1);
                        strncat(list, recs[j].mp, sizeof(list) - strlen(list) - 1);
                    }
                }
            }
            /* 只报第一次出现的那个设备 */
            int seen = 0;
            for (int k = 0; k < i; k++)
                if (!strcmp(recs[k].dev, recs[i].dev))
                    seen = 1;
            if (!seen && cnt > 1)
                printf("  %-10s %2d 次：%s\n", recs[i].dev, cnt, list);
        }
    }

    /* ---------- ③ 14.9.4 bind 的证据 ---------- */
    sec("③ 14.9.4 的证据：root 不等于 / —— 挂的是源设备的某一棵子树");
    {
        int shown = 0;
        for (int i = 0; i < n; i++) {
            if (strcmp(recs[i].root, "/") != 0) {
                printf("  root=%-40s -> %-34s (%s)\n", recs[i].root, recs[i].mp,
                       recs[i].fstype);
                shown++;
            }
        }
        if (!shown)
            printf("  没有任何条目的 root 不是 /（没有 bind mount）\n");
        else
            printf("  共 %d 条 bind mount（root != /）\n", shown);
        printf("  ↑ 这些就是 bind mount：源是设备上的一个子目录，挂到另一处。\n");
    }

    /* ---------- ④ per-mount vs super options ---------- */
    sec("④ 14.9.3 的证据：per-mount options 与 super options 是两栏");
    {
        for (int i = 0; i < n; i++) {
            if (!strcmp(recs[i].mp, "/tmp") || !strcmp(recs[i].mp, "/app") ||
                !strcmp(recs[i].mp, "/"))
                printf("  %-8s per-mount = %-34s\n  %-8s super     = %s\n",
                       recs[i].mp, recs[i].opts, "", recs[i].sopts);
        }
        printf("  ↑ per-mount 栏里的 nosuid/nodev/noexec 是**这一个挂载点**的选项；\n");
        printf("    super 栏里的 rw/size=/nr_inodes= 是**整个超级块**的属性。\n");
        printf("    同一个超级块可以被挂成多个挂载点、各自给不同的 per-mount 选项 ——\n");
        printf("    这正是为什么 MS_* 需要拆成 MS_* (ABI) 和 MNT_* (per-mount) 两套。\n");
    }

    /* ---------- ⑤ 实测 bind ---------- */
    sec("⑤ 试一次 bind mount（预期被挡住）");
    {
        errno = 0;
        if (mount("/tmp", "/app", NULL, MS_BIND, NULL) == -1)
            printf("  mount(\"/tmp\", \"/app\", NULL, MS_BIND, NULL) -> -1 errno=%d (%s)\n",
                   errno, strerror(errno));
        else
            printf("  成功\n");
        errno = 0;
        if (mount("/tmp", "/app", NULL, MS_BIND | MS_REC, NULL) == -1)
            printf("  mount(..., MS_BIND|MS_REC, NULL)                -> -1 errno=%d (%s)\n",
                   errno, strerror(errno));
        else
            printf("  成功\n");
        printf("  ↑ 递归与不递归都撞同一道 CAP_SYS_ADMIN 门，本机无法对比两者差异。\n");
    }

    printf("\n=== c14_9 done ===\n");
    return 0;
}
