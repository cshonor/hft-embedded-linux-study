/* c9_2_fsuid.c —— filesystem ID 与那个「看不出成败」的接口
 *
 * 9.5 的主题：Linux 独有的 fsuid / fsgid。
 *
 * 本程序要坐实三件事：
 *   ① fsuid 有两个**独立**读法：setfsuid(-1) 与 /proc/self/status 的 Uid: 第 4 列。
 *   ② setfsuid() 失败时**不报错**：返回值是「旧值」，成功也是「旧值」——光看返回值
 *      无法分辨。必须再读一次才知道有没有生效。
 *   ③ 文件权限检查用的是 fsgid/fsuid + 补充组，不是 egid/euid（内核 groups.c:234
 *      in_group_p() 里第一句比的就是 cred->fsgid）。
 *
 * 权威依据：
 *   - setfsuid(2) RETURN VALUE："On both success and failure, this call returns the
 *     previous filesystem user ID of the caller."
 *   - setfsuid(2) BUGS："No error indications of any kind are returned to the caller,
 *     ... the caller must resort to looking at the return value from a further call
 *     such as setfsuid(-1) (which will always fail) ... At the very least, EPERM
 *     should be returned when the call fails (because the caller lacks the
 *     CAP_SETUID capability)."
 *   - proc_pid_status(5)：Uid / Gid = Real, effective, saved set, and filesystem UIDs。
 *   - kernel/sys.c:865 __sys_setfsuid()：不改动时 `return old_fsuid;`（静默）。
 *
 * 编译: gcc -O0 -Wall -Wextra -o c9_2_fsuid c9_2_fsuid.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/fsuid.h>
#include <sys/types.h>
#include <unistd.h>

/* 取 /proc/self/status 里某一行（如 "Uid:"）的第 col 列（1-based） */
static unsigned status_col(const char *key, int col)
{
    FILE *fp = fopen("/proc/self/status", "r");
    if (fp == NULL)
        return 0;
    size_t klen = strlen(key);
    char buf[512];
    unsigned v[4] = { 0, 0, 0, 0 };
    unsigned ret = 0;
    while (fgets(buf, sizeof buf, fp) != NULL) {
        if (strncmp(buf, key, klen) == 0 && buf[klen] == ':') {
            /* key 已经是 "Uid:" 形式，sscanf 直接从冒号后取 4 个数 */
            if (sscanf(buf + klen, "%u %u %u %u", &v[0], &v[1], &v[2], &v[3]) >= col)
                ret = v[col - 1];
            break;
        }
    }
    fclose(fp);
    return ret;
}

int main(void)
{
    printf("=== ① fsuid 的两个读法 ===\n");
    uid_t via_api = (uid_t) setfsuid((uid_t) -1);
    unsigned via_proc = status_col("Uid:", 4);
    printf("  setfsuid(-1) 读回          : %u\n", (unsigned) via_api);
    printf("  /proc/self/status 第 4 列  : %u\n", via_proc);
    printf("  两者一致? %s\n", ((unsigned) via_api == via_proc) ? "是" : "否");

    printf("\n=== ② setfsuid() 的返回值分辨不出成败 ===\n");
    uid_t before = (uid_t) setfsuid((uid_t) -1);
    errno = 0;
    uid_t ret = (uid_t) setfsuid((uid_t) 10240);   /* 本容器里未映射的 uid */
    printf("  setfsuid(10240) 返回 %u   (errno=%d %s)\n",
           (unsigned) ret, errno, errno ? strerror(errno) : "未置位");
    uid_t after = (uid_t) setfsuid((uid_t) -1);
    printf("  再读一次 fsuid     = %u\n", (unsigned) after);
    printf("  判决: ");
    if (after == (uid_t) 10240)
        printf("生效了\n");
    else if (before == ret)
        printf("**没生效**，但它的返回值和「成功」完全一样 —— 看返回值看不出来\n");
    else
        printf("变化了 %u -> %u\n", (unsigned) before, (unsigned) after);

    printf("\n=== ③ fsgid 同理 ===\n");
    gid_t fa = (gid_t) setfsgid((gid_t) -1);
    printf("  setfsgid(-1)               = %u\n", (unsigned) fa);
    printf("  /proc/self/status 第 4 列  = %u\n", status_col("Gid:", 4));

    printf("\n=== ④ setfsuid(2) BUGS 原文 ===\n");
    printf("  \"No error indications of any kind are returned to the caller,\n");
    printf("   and the fact that both successful and unsuccessful calls return the\n");
    printf("   same value makes it impossible to directly determine whether the\n");
    printf("   call succeeded or failed.\"\n");

    printf("\n=== ⑤ 内核里 setfsuid() 的放行条件（kernel/sys.c:883-885）===\n");
    printf("  uid == ruid || uid == euid || uid == suid || uid == fsuid\n");
    printf("      || ns_capable_setid(user_ns, CAP_SETUID)\n");
    printf("  四条全不满足 -> 不动 cred，也**不设 errno**，直接 return old_fsuid\n");
    return 0;
}
