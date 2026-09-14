/* c9_1_all_creds.c —— 一个进程到底持有几类凭证？
 *
 * 9.1 只解决一件事：把「凭证（credentials）」这个笼统的词拆成**可枚举的 5 类 ID**，
 * 并给出两个**互相独立**的读法：一组 API，以及内核自己吐的 /proc 文件。
 * 两者都看到同一组数字，才算把这一节坐实。
 *
 * 权威依据：
 *   - credentials(7), User and group identifiers 一节：5 类逐条（real / effective /
 *     saved set / filesystem / supplementary）。
 *   - proc_pid_status(5)：Uid / Gid 行是 **四列** —— "Real, effective, saved set,
 *     and filesystem UIDs (GIDs)."；Groups 行是 "Supplementary group list."
 *   - kernel/sys.c:749 getresuid() 实现：三次 from_kuid_munged(cred->user_ns, ...)。
 *
 * 编译: gcc -O0 -Wall -Wextra -o c9_1_all_creds c9_1_all_creds.c
 */
#define _GNU_SOURCE
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fsuid.h>
#include <sys/types.h>
#include <unistd.h>

/* 从 /proc/self/status 里取一行（只用静态缓冲，避免自己引入堆分配） */
static void show_status_line(const char *key)
{
    FILE *fp = fopen("/proc/self/status", "r");
    if (fp == NULL) {
        printf("  (打不开 /proc/self/status)\n");
        return;
    }
    char buf[512];
    size_t klen = 0;
    while (key[klen] != '\0')
        klen++;
    while (fgets(buf, sizeof buf, fp) != NULL) {
        if (strncmp(buf, key, klen) == 0 && buf[klen] == ':') {
            printf("  %s", buf);      /* 整行原样抄，不加工 */
            break;
        }
    }
    fclose(fp);
}

static void show_whole_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    printf("--- %s ---\n", path);
    if (fp == NULL) {
        printf("  (打不开)\n");
        return;
    }
    char buf[256];
    int any = 0;
    while (fgets(buf, sizeof buf, fp) != NULL) {
        printf("    %s", buf);
        any = 1;
    }
    if (!any)
        printf("    (空文件)\n");
    fclose(fp);
}

int main(void)
{
    /* ---------- 1 类 + 2 类 + 3 类：一次全拿 ---------- */
    uid_t ruid, euid, suid;
    gid_t rgid, egid, sgid;
    if (getresuid(&ruid, &euid, &suid) == -1) {
        perror("getresuid");
        return 1;
    }
    if (getresgid(&rgid, &egid, &sgid) == -1) {
        perror("getresgid");
        return 1;
    }

    /* ---------- 4 类：filesystem ID ----------
     * setfsuid() 的返回值是**旧** fsuid，而且成功失败都返回它（setfsuid(2) BUGS）。
     * 所以惯用法是再调一次 setfsuid(-1)：-1 永远不合法，这一调用一定不改任何东西，
     * 但它的返回值就是「当前的 fsuid」。 */
    uid_t fsuid = (uid_t) setfsuid((uid_t) -1);
    gid_t fsgid = (gid_t) setfsgid((gid_t) -1);

    /* ---------- 5 类：supplementary groups ---------- */
    int ng = getgroups(0, NULL);        /* 先问几个，不传缓冲 */
    gid_t *groups = NULL;
    if (ng > 0) {
        groups = malloc(sizeof(gid_t) * (size_t) ng);
        if (groups == NULL) {
            perror("malloc");
            return 1;
        }
        int got = getgroups(ng, groups);
        if (got == -1) {
            perror("getgroups");
            return 1;
        }
        ng = got;
    }

    printf("=== 来源 ①：API ===\n");
    printf("UID  real=%u  effective=%u  saved=%u  filesystem=%u\n",
           (unsigned) ruid, (unsigned) euid, (unsigned) suid, (unsigned) fsuid);
    printf("GID  real=%u  effective=%u  saved=%u  filesystem=%u\n",
           (unsigned) rgid, (unsigned) egid, (unsigned) sgid, (unsigned) fsgid);
    printf("补充组 %d 个:", ng);
    for (int i = 0; i < ng; i++)
        printf(" %u", (unsigned) groups[i]);
    printf("\n");
    free(groups);

    printf("\n=== 来源 ②：内核自己的 /proc/self/status ===\n");
    show_status_line("Uid");
    show_status_line("Gid");
    show_status_line("Groups");
    show_status_line("CapEff");
    show_status_line("NoNewPrivs");

    printf("\n=== 为什么 uid=0 却不是「特权进程」===\n");
    show_whole_file("/proc/self/uid_map");
    show_whole_file("/proc/self/gid_map");

    printf("\n=== sysconf 给出的上限 ===\n");
    printf("  _SC_NGROUPS_MAX    = %ld\n", sysconf(_SC_NGROUPS_MAX));
    printf("  _SC_LOGIN_NAME_MAX = %ld\n", sysconf(_SC_LOGIN_NAME_MAX));
    return 0;
}
