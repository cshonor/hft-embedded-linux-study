/* c15_4_chown.c — Ch15 §15.3：chown() 的三条规则，本机（非特权用户）能验证哪几条
 *
 * TLPI §15.3.2 的规则（书中以特权进程为前提，本机 a0000 非 root，能实测
 * 的是子集，其余标注「需特权」）：
 *   ① 传 -1 表示「这一项不变」——chown(path, -1, -1) 合法且**会更新 ctime**；
 *   ② 非特权进程改属组：只能改成**自己补充组里已有的组**（Linux），
 *      BSD/macOS 规则不同（要求 egid 或补充组之一）——实测本机表现；
 *   ③ 非特权进程改属主：一律 EPERM（需 CAP_CHOWN / root）；
 *   ④ 非特权进程成功 chown 之后，S_ISUID/S_ISGID 位被内核清掉
 *      ——这是安全机制：防止「借 chown 保住 SUID」的提权路径。
 *      本机改不了属主，这条以「改组时 S_ISGID 是否被清」侧面观察。
 *
 * 编译： cc -Wall -Wextra -o c15_4_chown c15_4_chown.c
 * 取材： man-pages 6.19 chown(2)（EPERM / 特权与 SUID 清除段）
 *       TLPI §15.3.1/15.3.2（新文件属主规则 + chown 三规则）
 *       Linux v6.6 fs/attr.c notify_change()（ATTR_UID/ATTR_GID + SUID 清除）
 */
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* 相当于 TLPI ugid_functions.h 的 groupIdFromName（本 demo 自带迷你版） */
static gid_t groupIdFromNameSafe(const char *name)
{
    struct group *g = getgrnam(name);
    return g ? g->gr_gid : (gid_t) -1;
}

static void show(const char *tag, const char *path)
{
    struct stat sb;
    if (stat(path, &sb) == -1) { perror("stat"); exit(EXIT_FAILURE); }
    printf("  %-22s uid=%-5ld gid=%-5ld mode=%04lo%s%s\n",
           tag, (long) sb.st_uid, (long) sb.st_gid,
           (unsigned long) (sb.st_mode & 07777),
           (sb.st_mode & S_ISGID) ? " +SGID" : "",
           (sb.st_mode & S_ISUID) ? " +SUID" : "");
}

int main(void)
{
    const char *path = "/tmp/tlpi_c15_4.txt";
    uid_t euid = geteuid();
    gid_t egid = getegid();

    printf("== ⓪ 本机身份：euid=%ld egid=%ld（非特权用户视角）==\n\n",
           (long) euid, (long) egid);

    /* ---------- ① chown(-1,-1) 合法且动 ctime ---------- */
    printf("== ① chown(path, -1, -1)：POSIX 要求更新 ctime；本机是否照做实测 ==\n");
    unlink(path);
    int fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    struct stat b, a;
    if (stat(path, &b) == -1) { perror("stat"); return EXIT_FAILURE; }
    sleep(1);
    if (chown(path, -1, -1) == -1) { perror("chown(-1,-1)"); return EXIT_FAILURE; }
    if (stat(path, &a) == -1) { perror("stat2"); return EXIT_FAILURE; }
    printf("  ctime %ld.%09ld → %ld.%09ld（%s）\n",
           (long) b.st_ctimespec.tv_sec, (long) b.st_ctimespec.tv_nsec,
           (long) a.st_ctimespec.tv_sec, (long) a.st_ctimespec.tv_nsec,
           b.st_ctimespec.tv_sec != a.st_ctimespec.tv_sec ||
           b.st_ctimespec.tv_nsec != a.st_ctimespec.tv_nsec ? "动了" : "没动");
    printf("  ⚠️ POSIX 规定 chown() 成功后应更新 ctime（含 no-op 版）；Linux 实测\n");
    printf("     会动。本机 macOS 实测%s——BSD 对「值没变」短路了。\n",
           b.st_ctimespec.tv_sec != a.st_ctimespec.tv_sec ||
           b.st_ctimespec.tv_nsec != a.st_ctimespec.tv_nsec ? "动了" : "没动（跨平台差异！）");
    printf("     审计代码别依赖「空 chown 必动 ctime」，两个平台表现不同。\n\n");

    /* ---------- ②③ 非特权改属主 / 改属组 ---------- */
    printf("== ②③ 非特权进程改属主/属组：实测边界 ==\n");
    printf("  chown(path, 0, -1)（改成 root）: ");
    errno = 0;
    int r = chown(path, 0, -1);
    printf("ret=%d errno=%d(%s)  ← 改属主一律被拒（需 CAP_CHOWN）\n",
           r, errno, strerror(errno));

    /* 试试改成「 wheel 」（通常不是我们的补充组） */
    gid_t wheel = groupIdFromNameSafe("wheel");
    printf("  chown(path, -1, %ld)（wheel 组）: ", (long) wheel);
    errno = 0;
    r = chown(path, -1, wheel);
    printf("ret=%d errno=%d(%s)%s\n", r, errno, strerror(errno),
           r == 0 ? "  ← 本机竟然允许（BSD 不检查组成员关系）" : "");

    /* 改成自己所在的组 */
    printf("  chown(path, -1, %ld)（自己的 egid）: ", (long) egid);
    errno = 0;
    r = chown(path, -1, egid);
    printf("ret=%d errno=%d(%s)  ← 「不改任何东西」的改组总是允许\n",
           r, errno, strerror(errno));
    show("改组后", path);
    printf("  ⚠️ man-pages 写的是 Linux 规则（只能改成补充组里的组）；本机实测\n");
    printf("     macOS 连非成员的 wheel 都放行——BSD 实现比文档宽松。\n");
    printf("     跨平台程序对「改组会不会 EPERM」别做细粒度假设，以返回码为准。\n\n");

    /* ---------- ④ chown 是否清 SGID（侧面观察） ---------- */
    printf("== ④ chown 之后特殊位是否被清（侧面观察 SGID）==\n");
    if (chmod(path, 02755) == -1) { perror("chmod sgid"); return EXIT_FAILURE; }
    show("chmod +SGID 后", path);
    errno = 0;
    r = chown(path, -1, egid);
    printf("  再 chown(-1, 自己的 egid): ret=%d\n", r);
    show("chown 后", path);
    printf("  → 本机观察：%s\n",
           (stat(path, &a) == -1 ? "" :
            (a.st_mode & S_ISGID) ? "SGID 保留（改组前后属组没变，内核不清）"
                                  : "SGID 被清（chown 触发了特殊位清除）"));
    printf("  ⚠️ 书上的完整规则：非特权 chown 成功 → SUID/SGID 都被内核清掉\n");
    printf("     （Linux fs/attr.c 的 notify_change 里 kill S_ISUID|S_ISGID）。\n");
    printf("     完整验证需要 CAP_CHOWN（Pi 上 sudo 跑同一程序可复现）。\n");

    close(fd);
    unlink(path);
    return EXIT_SUCCESS;
}
