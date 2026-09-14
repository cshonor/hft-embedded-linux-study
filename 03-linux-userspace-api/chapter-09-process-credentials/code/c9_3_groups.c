/* c9_3_groups.c —— 补充组：怎么读、为什么写不了
 *
 * 9.6 的主题：Supplementary Group IDs。
 *
 * 本程序要坐实四件事：
 *   ① 读补充组的**惯用法**：先 getgroups(0, NULL) 问个数，再分配、再取。
 *   ② getgroups() 的两个 EINVAL 分支（负 size / 缓冲不够）在源码里是两处不同判断。
 *   ③ 写补充组（setgroups）要过**两道独立闸门**：CAP_SETGID 与 ns 是否允许；
 *      本环境两道都关着，而且能分别看到证据。
 *   ④ initgroups() 只是 setgroups() 的封装，所以它失败的方式和 setgroups() 一样。
 *
 * 权威依据：
 *   - kernel/groups.c:163 getgroups()：`if (gidsetsize < 0) return -EINVAL;`
 *     以及 :174 `if (i > gidsetsize) { i = -EINVAL; }`（注意**不是 ERANGE**）。
 *   - kernel/groups.c:187 may_setgroups()：
 *       `return ns_capable_setid(user_ns, CAP_SETGID) && userns_may_setgroups(user_ns);`
 *   - kernel/groups.c:200 setgroups() 里（:219）`groups_sort(group_info);` —— 内核会排序。
 *   - kernel/groups.c:229 in_group_p()：先比 **fsgid**，再二分查补充组。
 *   - getgroups(2) ERRORS / EPERM（两个来源）；initgroups(3) ERRORS -> setgroups(2)。
 *   - /proc/pid/setgroups = "deny" 的含义见 user_namespaces(7)。
 *
 * 编译: gcc -O0 -Wall -Wextra -o c9_3_groups c9_3_groups.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void show_setgroups_file(void)
{
    FILE *fp = fopen("/proc/self/setgroups", "r");
    printf("--- /proc/self/setgroups ---\n");
    if (fp == NULL) {
        printf("  (打不开)\n");
        return;
    }
    char buf[64];
    while (fgets(buf, sizeof buf, fp) != NULL)
        printf("  %s", buf);
    fclose(fp);
}

int main(void)
{
    printf("=== ① 读补充组的惯用法 ===\n");
    errno = 0;
    int n = getgroups(0, NULL);          /* size=0：不动缓冲，只回个数 */
    printf("  getgroups(0, NULL) = %d   errno=%d\n", n, errno);
    if (n < 0) {
        perror("getgroups");
        return 1;
    }
    if (n == 0) {
        printf("  （本进程没有任何补充组）\n");
    } else {
        gid_t *g = malloc(sizeof(gid_t) * (size_t) n);
        if (g == NULL) {
            perror("malloc");
            return 1;
        }
        errno = 0;
        int got = getgroups(n, g);
        printf("  getgroups(%d, buf) = %d   errno=%d\n", n, got, errno);
        printf("  列表:");
        for (int i = 0; i < got; i++)
            printf(" %u", (unsigned) g[i]);
        printf("\n");
        free(g);
    }

    printf("\n=== ② getgroups() 的两个 EINVAL 分支 ===\n");
    /* 这里的 -1 是**故意**的。有意思的是：如果直接写 getgroups(-1, NULL)，
     * gcc（-Wall）会报 -Wstringop-overflow，因为 unistd.h 里给 getgroups 标了
     * "__attribute__((access(write_only, 2, 1)))" —— 编译器知道「size 为负」
     * 必然越界，所以这个笔误是能被静态抓出来的。用一个 volatile 变量转一手，
     * 编译器就无法在编译期知道具体值，告警随之消失；内核那边照样返回 EINVAL。 */
    volatile int neg_size = -1;
    errno = 0;
    int r = getgroups(neg_size, NULL);   /* 负 size —— 源码里第一处判断 */
    printf("  getgroups(-1, NULL)  = %d  errno=%d (%s)\n", r, errno, strerror(errno));
    printf("  「缓冲不够」那一支（size 比组数少且非 0）在源码里是另一处判断，\n");
    printf("  本环境只有 %d 个补充组，构造不出（需要 >= 2 个组）。\n", n > 0 ? n : 0);

    printf("\n=== ③ 写补充组要过两道闸门 ===\n");
    show_setgroups_file();
    errno = 0;
    r = setgroups(0, NULL);
    printf("  setgroups(0, NULL)   = %d  errno=%d (%s)\n", r, errno, strerror(errno));
    printf("  闸门 1  CAP_SETGID  : 看主程序的 CapEff —— 本环境是 0（无任何 capability）\n");
    printf("  闸门 2  ns 是否允许 : /proc/self/setgroups = deny（见上）\n");
    printf("  两道独立，任何一道关着都返回 EPERM；靠 errno 分辨不出是哪一道。\n");

    printf("\n=== ④ initgroups() 是 setgroups() 的封装 ===\n");
    errno = 0;
    r = initgroups("ce", (gid_t) 10240);
    printf("  initgroups(\"ce\", 10240) = %d  errno=%d (%s)\n", r, errno, strerror(errno));
    printf("  man 里 initgroups(3) 的 EPERM 一条写的就是 \"See the underlying\n");
    printf("  system call setgroups(2)\" —— 所以它不可能有别的失败方式。\n");

    printf("\n=== ⑤ getgrouplist() 与 /etc/group 的关系 ===\n");
    {
        int ng = 8;
        gid_t gl[8];
        errno = 0;
        r = getgrouplist("ce", (gid_t) 10240, gl, &ng);
        printf("  getgrouplist(\"ce\", 10240, ...) = %d  ngroups=%d  errno=%d (%s)\n",
               r, ng, errno, strerror(errno));
        printf("  它只会写进 basegid + 从组数据库查到的组；本容器没有 /etc/group，\n");
        printf("  所以只剩 basegid 一个（errno 是查库时留下的 ENOENT，不是函数失败）。\n");
    }

    printf("\n=== ⑥ 内核会排序，所以别把「顺序」当契约 ===\n");
    printf("  kernel/groups.c:200 setgroups() 里（:219）调 groups_sort()；\n");
    printf("  :94 groups_search() 是 bsearch —— 排序是为了让二分查找成立；\n");
    printf("  :234 in_group_p() 先比 cred->fsgid，再二分查补充组（**不是 egid**）。\n");
    return 0;
}
