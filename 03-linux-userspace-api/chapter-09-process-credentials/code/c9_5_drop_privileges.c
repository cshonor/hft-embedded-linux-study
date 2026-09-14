/* c9_5_drop_privileges.c —— 临时降权 vs 永久降权
 *
 * 9.4 的主题：saved set-user-ID 的存在意义，就是让 set-user-ID 程序能
 * 「降下来干活、再升回去」。
 *
 *   临时降权：seteuid(t)          euid 变、ruid/suid 不变 -> 还能 seteuid(0) 回切
 *   永久降权：setuid(t)           ruid=euid=suid=t       -> 回不去了
 *             或 setresuid(t,t,t) 同上，而且更明确地表达「连 saved 也要砸掉」
 *
 * 为什么 setuid() 回不去：POSIX 的 setuid() 是 SysV 语义，**把 saved 也一起设**。
 * 在真正的 set-user-ID-root 程序里，一旦用 setuid() 降权，就再也拿不回 root。
 * 内核源码里那条注释把话说得很直白（kernel/sys.c:601-611）：
 *   "Note that SAVED_ID's is deficient in that a setuid root program like sendmail,
 *    for example, cannot set its uid to be a normal user and then switch back,
 *    because if you're root, setuid() sets the saved uid too."
 *
 * ⚠️ 本程序在评测容器里**跑不出降权效果** —— 容器只映射了 1 个 uid（见
 *    /proc/self/uid_map），内核在 make_kuid() 阶段就以 EINVAL 挡掉了。这不是程序
 *    写错，是环境物理上做不到。程序会如实打印这个失败，并把真机上的跑法印出来。
 *
 * 编译: gcc -O0 -Wall -Wextra -o c9_5_drop_privileges c9_5_drop_privileges.c
 * 真机跑法（需要 root）:
 *   sudo useradd -m bob
 *   id bob                       # 拿到 bob 的 uid，假设是 1001
 *   sudo ./c9_5_drop_privileges temp 1001
 *   sudo ./c9_5_drop_privileges perm 1001
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static void print_ids(const char *tag)
{
    uid_t r, e, s;
    if (getresuid(&r, &e, &s) == -1) {
        perror("getresuid");
        return;
    }
    printf("    %-22s r=%u e=%u s=%u\n", tag, (unsigned) r, (unsigned) e, (unsigned) s);
}

static int do_temp(uid_t target)
{
    printf("\n--- 临时降权：seteuid(%u) ---\n", (unsigned) target);
    print_ids("降权前");
    errno = 0;
    if (seteuid(target) == -1) {
        printf("    seteuid(%u) 失败: errno=%d (%s)\n",
               (unsigned) target, errno, strerror(errno));
        return 1;
    }
    print_ids("降权后");
    printf("    → 用 ruid/suid 回切：seteuid(%u)\n", (unsigned) getuid());
    errno = 0;
    if (seteuid(getuid()) == -1) {
        printf("    seteuid 回切失败: errno=%d (%s)\n", errno, strerror(errno));
        return 1;
    }
    print_ids("回切后");
    printf("    可以回切，因为 saved(=suid) 没被砸掉。\n");
    return 0;
}

static int do_perm(uid_t target)
{
    printf("\n--- 永久降权：setuid(%u) ---\n", (unsigned) target);
    print_ids("降权前");
    errno = 0;
    if (setuid(target) == -1) {
        printf("    setuid(%u) 失败: errno=%d (%s)\n",
               (unsigned) target, errno, strerror(errno));
        return 1;
    }
    print_ids("降权后");
    printf("    → 试着用 setuid(0) 回切：\n");
    errno = 0;
    if (setuid(0) == -1)
        printf("    setuid(0) 失败: errno=%d (%s)  ← 回不去了，这就是「永久」\n",
               errno, strerror(errno));
    else {
        printf("    setuid(0) 成功了！\n");
        print_ids("回切后");
        printf("    ⚠️ 这里成功说明降权没有砸掉所有东西，不是安全写法。\n");
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char *mode = (argc > 1) ? argv[1] : "show";
    uid_t target = (argc > 2) ? (uid_t) strtoul(argv[2], NULL, 10) : (uid_t) 1000;

    print_ids("开始");

    if (strcmp(mode, "show") == 0) {
        printf("\n用法: %s <show|temp|perm> [目标uid]\n", argv[0]);
        printf("  show  只打印当前 R/E/S\n");
        printf("  temp  seteuid 降权再回切（需要 saved 还在）\n");
        printf("  perm  setuid  永久降权（连 saved 一起砸）\n");
        printf("\n本环境只能走 show：目标 uid 没有映射，set*id 会 EINVAL。\n");
        return 0;
    }
    if (strcmp(mode, "temp") == 0)
        return do_temp(target);
    if (strcmp(mode, "perm") == 0)
        return do_perm(target);

    printf("未知模式: %s\n", mode);
    return 2;
}
