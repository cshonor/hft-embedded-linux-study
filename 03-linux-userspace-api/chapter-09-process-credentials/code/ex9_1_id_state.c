/* ex9_1_id_state.c —— 原书练习 9-1 的**可执行验证器**
 *
 * 原书练习 9-1（TLPI 9.9）：
 *   "假设进程的初始用户 ID 集合是 real=1000, effective=0, saved=0, file-system=0。
 *    在下列调用之后，这些 ID 分别是什么状态？
 *      (a) setuid(2000)
 *      (b) setreuid(-1, 2000)
 *      (c) seteuid(2000)
 *      (d) setfsuid(2000)
 *      (e) setresuid(-1, 2000, 3000)"
 *
 * 每一个小题都要**从同一个初始状态出发**，而一个进程的凭证改了就回不去，
 * 所以本程序一次只做一题（用 argv[1] 选 a..e），每题都要重启一个新进程。
 *
 * 怎么造出 R=1000 / E=0 / S=0 这个初始状态：
 *   正是「一个 set-user-ID-root 程序被 uid 1000 的用户执行」时的状态。
 *      gcc -O0 -Wall -Wextra -o ex9_1_id_state ex9_1_id_state.c
 *      sudo chown root ex9_1_id_state
 *      sudo chmod u+s ex9_1_id_state
 *      sudo -u '#1000' ./ex9_1_id_state a      # 依次 a..e
 *
 * 本程序会先**自检**初始状态；不符就明确说「这台机器上做不了这个实验」并给出上面的
 * 步骤，而不是把一个假的成功打印出来。
 *
 * 「期望值」一栏怎么来的（全部按 kernel/sys.c 的源码推，不是抄的）：
 *   特权分支 setuid()      : new->suid = new->uid = uid; 然后 new->fsuid = new->euid = uid
 *   特权分支 setreuid(r,e) : suid 在「r != -1 或 e != 旧 ruid」时被设为新 euid；
 *                            最后恒有 new->fsuid = new->euid
 *   setresuid(r,e,s)       : 三者各自独立改（-1 = 不改），最后 new->fsuid = new->euid
 *   setfsuid(u)            : 只改 fsuid，不动 R/E/S
 *
 * 编译: gcc -O0 -Wall -Wextra -o ex9_1_id_state ex9_1_id_state.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fsuid.h>
#include <sys/types.h>
#include <unistd.h>

struct quad {
    unsigned r, e, s, f;
};

static struct quad snap(void)
{
    struct quad q;
    uid_t r, e, s;
    if (getresuid(&r, &e, &s) == -1) {
        perror("getresuid");
        exit(1);
    }
    q.r = (unsigned) r;
    q.e = (unsigned) e;
    q.s = (unsigned) s;
    q.f = (unsigned) setfsuid((uid_t) -1);
    return q;
}

static void print_quad(const char *tag, struct quad q)
{
    printf("  %-10s real=%-6u effective=%-6u saved=%-6u fs=%-6u\n",
           tag, q.r, q.e, q.s, q.f);
}

int main(int argc, char *argv[])
{
    if (argc != 2 || strchr("abcde", argv[1][0]) == NULL || argv[1][1] != '\0') {
        fprintf(stderr, "用法: %s <a|b|c|d|e>\n", argv[0]);
        return 2;
    }
    char which = argv[1][0];

    printf("=== 初始状态自检 ===\n");
    struct quad q0 = snap();
    print_quad("实测", q0);
    printf("  期望  real=1000   effective=0      saved=0      fs=0\n");
    if (!(q0.r != 0 && q0.e == 0 && q0.s == 0)) {
        printf("\n  ✗ 初始状态不符 —— 本环境做不了这个实验。\n");
        printf("    它需要「一个 set-user-ID-root 程序被 uid 1000 的用户执行」。\n");
        printf("    在容器里做不到，因为容器只映射了 1 个 uid，chown 不出 root 属主：\n");
        printf("      cat /proc/self/uid_map      # 只有一行 \"0 <host> 1\"\n");
        printf("    真机上照上面注释里的四条命令做即可。\n");
        printf("    本题的答案（按 kernel/sys.c 推导）见 note 9.9 的答案表。\n");
        return 0;                    /* 不是程序出错，是环境不具备 */
    }

    printf("\n=== 执行 (%c) ===\n", which);
    errno = 0;
    int rc = 0;
    switch (which) {
    case 'a': rc = setuid((uid_t) 2000);                    break;
    case 'b': rc = setreuid((uid_t) -1, (uid_t) 2000);      break;
    case 'c': rc = seteuid((uid_t) 2000);                   break;
    case 'd': rc = setfsuid((uid_t) 2000);                  break;
    case 'e': rc = setresuid((uid_t) -1, (uid_t) 2000, (uid_t) 3000); break;
    }
    printf("  返回值 %d   errno=%d (%s)\n", rc, errno, errno ? strerror(errno) : "未置位");

    struct quad q1 = snap();
    print_quad("结果", q1);

    /* 期望值表（练习 9-1 的答案） */
    struct quad want;
    const char *why;
    switch (which) {
    case 'a':
        want = (struct quad) { 2000, 2000, 2000, 2000 };
        why = "有 CAP_SETUID -> suid 与 uid 一起设；fsuid 跟随 euid";
        break;
    case 'b':
        want = (struct quad) { 1000, 2000, 2000, 2000 };
        why = "ruid=-1 不动；euid 改了且 != 旧 ruid -> suid 也设成新 euid";
        break;
    case 'c':
        want = (struct quad) { 1000, 2000, 0, 2000 };
        why = "seteuid 只改 euid；suid 保持 0；fsuid 跟随 euid";
        break;
    case 'd':
        want = (struct quad) { 1000, 0, 0, 2000 };
        why = "setfsuid 只改 fsuid，R/E/S 一点不动";
        break;
    default:
        want = (struct quad) { 1000, 2000, 3000, 2000 };
        why = "三者各自生效；fsuid 跟随 euid";
        break;
    }
    print_quad("期望", want);
    printf("  依据: %s\n", why);
    printf("  一致? %s\n",
           (q1.r == want.r && q1.e == want.e && q1.s == want.s && q1.f == want.f)
               ? "是" : "**否** —— 若初值自检通过而这里不一致，说明内核行为与推导不符");
    return 0;
}
