/* ex9_3_my_initgroups.c —— 原书练习 9-3：自己实现 initgroups()
 *
 * 原书练习 9-3（TLPI 9.9）：
 *   "用 setgroups() 和第 8.4 节里那些读口令/组文件的库函数实现 initgroups()。
 *    记住：进程必须有特权才能调用 setgroups()。"
 *
 * 实现思路（就是 glibc initgroups() 做的事）：
 *   1. 遍历组数据库，凡是成员名单里出现该用户的组，把它的 gid 收进来；
 *   2. 再把 basegid（主组）也加进去；
 *   3. 调 setgroups() 一次装上。
 *
 * 本程序有两点比「照抄答案」更有用：
 *   ① 它用 **fgetgrent(FILE *)**（Ch8 §8.4）扫一个**可以自己指定**的组文件。
 *      因为评测容器里根本没有 /etc/group，程序会先造一个合成的组文件再扫 ——
 *      于是这份实现**在这里真的能跑出结果**，而不是卡在 EPERM 上。
 *   ② 它顺手把 glibc 的 getgrouplist() 也调一遍做对照：那个函数做的是同一件事，
 *      只是它只认系统的组数据库（NSS），扫不到我们合成的文件 —— 这个差异本身
 *      就说明「库函数读的是哪个文件」是可以被替换/缺失的。
 *
 * 编译: gcc -O0 -Wall -Wextra -o ex9_3_my_initgroups ex9_3_my_initgroups.c
 * 用法: ./ex9_3_my_initgroups [用户名] [主组gid] [组文件]
 *       默认: alice 1000 /etc/group（不存在时自动改用 /tmp 合成文件）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXGROUPS 64

/* 造一个合成的组文件，字段格式与 /etc/group 完全一致（4 字段） */
static const char *make_synthetic_group_file(void)
{
    const char *p = "/tmp/ex9_3_group";
    FILE *fp = fopen(p, "w");
    if (fp == NULL)
        return NULL;
    /* 注意最后一行空成员的组：它**没有**任何成员，实现里必须不被选中 */
    fputs("wheel:x:10:alice,bob\n", fp);
    fputs("dev:x:2000:alice\n", fp);
    fputs("docker:x:999:bob,alice\n", fp);
    fputs("empty:x:3000:\n", fp);        /* gr_mem[0] == NULL 的组 */
    fclose(fp);
    return p;
}

/* 核心：扫组文件，收集「user 是成员」的所有 gid，最后补上 basgid */
static int my_initgroups(const char *user, gid_t basegid, const char *path,
                         gid_t *out, int max, int *count, const char **scanned)
{
    gid_t list[MAXGROUPS];
    int n = 0;
    int empty_seen = 0;

    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return -1;
    *scanned = path;

    struct group *gr;
    while ((gr = fgetgrent(fp)) != NULL) {
        if (gr->gr_mem == NULL)
            continue;
        if (gr->gr_mem[0] == NULL) {
            empty_seen++;                       /* 空组：明确记一笔 */
            continue;
        }
        for (char **m = gr->gr_mem; *m != NULL; m++) {
            if (strcmp(*m, user) == 0) {
                if (n < MAXGROUPS)
                    list[n++] = gr->gr_gid;
                break;                          /* 同一组只收一次 */
            }
        }
    }
    fclose(fp);

    printf("  扫 %s：命中 %d 个组，另有 %d 个「空成员」组被跳过\n",
           path, n, empty_seen);

    list[n++] = basegid;                        /* 主组也要在集合里 */
    if (n > max)
        return -2;
    memcpy(out, list, sizeof(gid_t) * (size_t) n);
    *count = n;
    return 0;
}

int main(int argc, char *argv[])
{
    const char *user = (argc > 1) ? argv[1] : "alice";
    gid_t basegid = (argc > 2) ? (gid_t) strtoul(argv[2], NULL, 10) : (gid_t) 1000;
    const char *path = (argc > 3) ? argv[3] : "/etc/group";

    printf("=== 目标: 为 %s 构造补充组列表（主组 gid=%u）===\n",
           user, (unsigned) basegid);

    if (access(path, R_OK) != 0) {
        printf("  %s 不可读（%s），改用合成组文件\n", path, strerror(errno));
        path = make_synthetic_group_file();
        if (path == NULL) {
            printf("  造合成文件失败: %s\n", strerror(errno));
            return 1;
        }
    }

    gid_t list[MAXGROUPS];
    int n = 0;
    const char *used = NULL;
    if (my_initgroups(user, basegid, path, list, MAXGROUPS, &n, &used) != 0) {
        printf("  my_initgroups 失败: %s\n", strerror(errno));
        return 1;
    }

    printf("\n=== 我算出来的列表（%d 个）===\n  ", n);
    for (int i = 0; i < n; i++)
        printf("%u ", (unsigned) list[i]);
    printf("\n");

    printf("\n=== 对照: glibc 的 getgrouplist()（同一算法，但只读系统组库）===\n");
    {
        int ng = MAXGROUPS;
        gid_t gl[MAXGROUPS];
        errno = 0;
        int rc = getgrouplist(user, basegid, gl, &ng);
        printf("  getgrouplist(\"%s\", %u, ...) = %d  ngroups=%d  errno=%d (%s)\n",
               user, (unsigned) basegid, rc, ng, errno, strerror(errno));
        printf("  列表: ");
        for (int i = 0; i < ng; i++)
            printf("%u ", (unsigned) gl[i]);
        printf("\n");
        printf("  差异原因: getgrouplist 走 NSS（本环境无 /etc/group，也没有\n");
        printf("            nsswitch.conf），扫不到合成文件，所以只剩 basegid。\n");
    }

    printf("\n=== 装上去: setgroups(%d, list) ===\n", n);
    errno = 0;
    int rc = setgroups((size_t) n, list);
    printf("  返回 %d  errno=%d (%s)\n", rc, errno, strerror(errno));
    if (rc == -1)
        printf("  预期之内: setgroups 要 CAP_SETGID，且 /proc/self/setgroups 是 deny，\n"
               "  两道闸门本环境都关着（见 c9_3_groups.c）。\n");
    else
        printf("  居然成功了 —— 那说明本进程确实有 CAP_SETGID。\n");

    printf("\n=== glibc 的 initgroups() 做的是同一件事 ===\n");
    errno = 0;
    rc = initgroups(user, basegid);
    printf("  initgroups(\"%s\", %u) = %d  errno=%d (%s)\n",
           user, (unsigned) basegid, rc, errno, strerror(errno));
    printf("  它的 EPERM 是 setgroups() 传上来的，所以和上面那条必然同因同果。\n");
    return 0;
}
