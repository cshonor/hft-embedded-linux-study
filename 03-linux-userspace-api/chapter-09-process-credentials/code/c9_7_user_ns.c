/* c9_7_user_ns.c —— 亲手造一个 user namespace，看凭证怎么被重新解释
 *
 * 这是 9.1 的一个极端例子，也是理解「uid 是个命名空间里的编号」最直观的方法。
 *
 * 现象（本程序跑出来会看到）：
 *   ① unshare(CLONE_NEWUSER) 之后、写映射之前，getuid() 变成 **65534** ——
 *      不是「没有人」，而是溢出值 overflowuid：新 ns 里还没有任何映射，
 *      内核打印自己的 uid 时取不到对应值，就给你 65534。
 *   ② 写 gid_map 成功，写 uid_map 却 EPERM —— 两条规则完全不同（见下）。
 *   ③ uid_map 写失败后，这个 ns 里**一个 uid 都没映射**，于是 setuid 任何值都 EINVAL。
 *
 * 为什么 uid_map 会 EPERM：user_namespaces(7) 第 370-381 行 ——
 *   "If updating /proc/pid/uid_map to create a mapping that maps **UID 0 in the
 *    parent namespace**, then one of the following must be true: ... it must have
 *    the CAP_SETFCAP capability in that user namespace"（Linux 5.12 起）。
 *   我们 CapEff=0，没有 CAP_SETFCAP -> 写 uid_map 被拒。
 *   而 gid_map 的规则里没有这一条，所以同一个进程写得进 gid_map。
 *
 * 还有一条规则也值得记住（:412）：非特权进程只能写「**一行**，且把写入者自己的
 * euid 映射进去」的映射。所以不存在「随便挑个 uid 映射」这回事。
 *
 * 编译: gcc -O0 -Wall -Wextra -o c9_7_user_ns c9_7_user_ns.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void dump(const char *path)
{
    FILE *fp = fopen(path, "r");
    printf("    %s: ", path);
    if (fp == NULL) {
        printf("(打不开 %s)\n", strerror(errno));
        return;
    }
    char buf[256];
    int any = 0;
    while (fgets(buf, sizeof buf, fp) != NULL) {
        printf("%s", buf);
        any = 1;
    }
    if (!any)
        printf("(空 —— 一行映射都没有)\n");
    fclose(fp);
}

static void write_one(const char *path, const char *what)
{
    errno = 0;
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        printf("    [%s] 打不开: errno=%d (%s)\n", what, errno, strerror(errno));
        return;
    }
    int r = fputs(what, fp);
    int c = fclose(fp);                 /* ⚠️ 必须 fclose 才知道写有没有失败 */
    printf("    [%s] 写 \"%s\": fputs=%d fclose=%d errno=%d (%s)\n",
           what, what, r, c, errno, errno ? strerror(errno) : "未置位");
}

int main(void)
{
    printf("=== 父进程 ===\n");
    printf("  uid=%u euid=%u gid=%u\n",
           (unsigned) getuid(), (unsigned) geteuid(), (unsigned) getgid());
    dump("/proc/self/uid_map");

    printf("\n=== 子进程：unshare(CLONE_NEWUSER) ===\n");
    fflush(stdout);                     /* ⚠️ fork 前 flush，否则父进程缓冲被复制后吐两遍 */
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        printf("  [1] fork 之后          uid=%u gid=%u\n",
               (unsigned) getuid(), (unsigned) getgid());

        errno = 0;
        int rc = unshare(CLONE_NEWUSER);
        printf("  [2] unshare(CLONE_NEWUSER) = %d  errno=%d (%s)\n",
               rc, errno, errno ? strerror(errno) : "未置位");
        if (rc == -1) {
            fflush(stdout);
            _exit(1);
        }
        printf("  [3] 写映射之前          uid=%u gid=%u   <- 65534 = overflowuid\n",
               (unsigned) getuid(), (unsigned) getgid());
        dump("/proc/self/uid_map");
        dump("/proc/self/gid_map");
        dump("/proc/self/setgroups");
        {
            FILE *fp = fopen("/proc/sys/kernel/overflowuid", "r");
            char buf[64];
            if (fp != NULL) {
                if (fgets(buf, sizeof buf, fp) != NULL)
                    printf("    /proc/sys/kernel/overflowuid = %s", buf);
                fclose(fp);
            }
        }

        printf("\n  [4] 写 setgroups=deny（写 gid_map 的前置条件）\n");
        write_one("/proc/self/setgroups", "deny");

        printf("\n  [5] 写 gid_map = \"0 0 1\"（子ns uid 0 <- 父ns gid 0）\n");
        write_one("/proc/self/gid_map", "0 0 1");
        printf("  [6] 写完之后            uid=%u gid=%u\n",
               (unsigned) getuid(), (unsigned) getgid());
        dump("/proc/self/gid_map");

        printf("\n  [7] 写 uid_map = \"0 0 1\"（同样的形态，只换了文件）\n");
        write_one("/proc/self/uid_map", "0 0 1");
        printf("  [8] 写完之后            uid=%u gid=%u   <- uid 没变\n",
               (unsigned) getuid(), (unsigned) getgid());
        dump("/proc/self/uid_map");

        printf("\n  [9] 在这种状态下调 setuid(10240)\n");
        errno = 0;
        int r2 = setuid((uid_t) 10240);
        printf("      setuid(10240) = %d  errno=%d (%s)\n",
               r2, errno, strerror(errno));

        printf("\n  [10] 结论\n");
        printf("      uid_map 里一行都没有 -> 这个 ns 里任何 uid 都「不合法」->\n");
        printf("      连 setuid 自己都做不了。这不是 bug，是映射没建起来。\n");
        fflush(stdout);
        _exit(0);
    }

    int st = 0;
    if (waitpid(pid, &st, 0) == -1)
        perror("waitpid");
    else
        printf("\n=== 父进程后续 ===\n  子进程结束: exited=%d status=%d\n",
               WIFEXITED(st), WIFEXITED(st) ? WEXITSTATUS(st) : -1);
    return 0;
}
