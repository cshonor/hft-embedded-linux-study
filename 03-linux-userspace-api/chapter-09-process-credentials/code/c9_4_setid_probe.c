/* c9_4_setid_probe.c —— 把「按规则应该怎样」和「实测返回什么」并排放
 *
 * 9.2 / 9.3 / 9.4 / 9.7 的规则可以用文字背，但**返回码**必须实测。
 * 本程序对 8 个 set*id 接口各试三类参数：
 *   (a) 设成「自己」         -> 应当成功（ps: 这是特例，见下）
 *   (b) -1 形式（不改动）    -> 应当成功，且什么都不变
 *   (c) 未映射的 uid/gid     -> 应当 EINVAL（若内核版本/命名空间如此）
 *
 * 为什么 (a) 会成功：内核**不是**先看「有没有 CAP_SETUID」，而是先看目标值
 * 是否落在 {ruid, euid, suid, fsuid} 里（kernel/sys.c:883 等）。设成自己必然在集合内，
 * 所以不需要任何 capability。这解释了「uid=0 但 CapEff=0」的进程为什么还能调
 * setuid(0) 成功 —— 它其实什么都没做。
 *
 * 权威依据：
 *   - setuid(2) ERRORS EINVAL: "The user ID specified in uid is not valid in this
 *     user namespace."（描述），内核对应 kernel/sys.c:620 make_kuid + :621 uid_valid。
 *   - setresuid(2) DESCRIPTION：非特权进程只能在各 ID 之间互相设；
 *     "If one of the arguments equals -1, the corresponding value is not changed."
 *   - setreuid(2) DESCRIPTION：-1 表示不改；suid 的更新规则。
 *   - kernel/sys.c:612 __sys_setuid()、:669 __sys_setresuid()、:531 __sys_setreuid()。
 *
 * 编译: gcc -O0 -Wall -Wextra -o c9_4_setid_probe c9_4_setid_probe.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define UNMAPPED 10240u

#define TRY(expr, rule)                                                       \
    do {                                                                      \
        errno = 0;                                                            \
        int rc_ = (int) (expr);                                               \
        int e_ = errno;                                                       \
        printf("  %-32s | %-30s | %4d | %2d %-16s\n",                         \
               #expr, rule, rc_, e_, e_ ? strerror(e_) : "-");                \
    } while (0)

static int same_as_before(void)
{
    uid_t a, b, c;
    if (getresuid(&a, &b, &c) == -1)
        return -1;
    return (a == 0 && b == 0 && c == 0) ? 1 : 0;
}

int main(void)
{
    uid_t ru, eu, su;
    gid_t rg, eg, sg;
    getresuid(&ru, &eu, &su);
    getresgid(&rg, &eg, &sg);

    printf("环境: uid=%u/%u/%u  gid=%u/%u/%u\n",
           (unsigned) ru, (unsigned) eu, (unsigned) su,
           (unsigned) rg, (unsigned) eg, (unsigned) sg);

    printf("\n=== uid 家族 ===\n");
    printf("  %-32s | %-30s | %4s | %s\n", "调用", "按规则应当", "ret", "errno");
    printf("  ---------------------------------+--------------------------------+------+------------------\n");
    TRY(setuid(getuid()),            "自己在集合内 -> 0");
    TRY(setreuid((uid_t) -1, (uid_t) -1), "-1 = 不改 -> 0");
    TRY(setresuid((uid_t) -1, (uid_t) -1, (uid_t) -1), "-1 = 不改 -> 0");
    TRY(setresuid(getuid(), getuid(), getuid()), "自己在集合内 -> 0");
    TRY(setuid((uid_t) UNMAPPED),    "未映射 -> EINVAL(22)");
    TRY(seteuid((uid_t) UNMAPPED),   "未映射 -> EINVAL(22)");
    TRY(setreuid((uid_t) UNMAPPED, (uid_t) UNMAPPED), "未映射 -> EINVAL(22)");
    TRY(setresuid((uid_t) UNMAPPED, (uid_t) UNMAPPED, (uid_t) UNMAPPED),
        "未映射 -> EINVAL(22)");

    printf("\n=== gid 家族 ===\n");
    printf("  %-32s | %-30s | %4s | %s\n", "调用", "按规则应当", "ret", "errno");
    printf("  ---------------------------------+--------------------------------+------+------------------\n");
    TRY(setgid(getgid()),            "自己在集合内 -> 0");
    TRY(setregid((gid_t) -1, (gid_t) -1), "-1 = 不改 -> 0");
    TRY(setresgid((gid_t) -1, (gid_t) -1, (gid_t) -1), "-1 = 不改 -> 0");
    TRY(setgid((gid_t) UNMAPPED),    "未映射 -> EINVAL(22)");
    TRY(setegid((gid_t) UNMAPPED),   "未映射 -> EINVAL(22)");
    TRY(setregid((gid_t) UNMAPPED, (gid_t) UNMAPPED), "未映射 -> EINVAL(22)");
    TRY(setresgid((gid_t) UNMAPPED, (gid_t) UNMAPPED, (gid_t) UNMAPPED),
        "未映射 -> EINVAL(22)");

    printf("\n=== 试完一圈，身份变了没有？ ===\n");
    uid_t a, b, c;
    gid_t x, y, z;
    getresuid(&a, &b, &c);
    getresgid(&x, &y, &z);
    printf("  现在 uid=%u/%u/%u   gid=%u/%u/%u\n",
           (unsigned) a, (unsigned) b, (unsigned) c,
           (unsigned) x, (unsigned) y, (unsigned) z);
    printf("  与开始时相同? %s\n", same_as_before() == 1 ? "是（所有调用要么是 no-op，要么被 EINVAL 挡下）" : "否");

    printf("\n=== 为什么「未映射」是 EINVAL 而不是 EPERM ===\n");
    printf("  内核先做 make_kuid(ns, uid) -> 拿不到合法 kuid 时**连通配检查都不做**，\n");
    printf("  直接 -EINVAL（kernel/sys.c:620-622）。EPERM 才是「uid 合法但不许改」。\n");
    printf("  这条区分只在有 user namespace 的环境里才看得到：\n");
    printf("  普通系统上所有 uid 都有映射，setuid(10240) 得到的是 EPERM。\n");
    return 0;
}
