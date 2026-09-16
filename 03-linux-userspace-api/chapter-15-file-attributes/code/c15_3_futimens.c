/* c15_3_futimens.c — Ch15 §15.2.2：utimensat() / futimens() 的精确语义
 *
 * TLPI §15.2.2 钉住的几条（全部实测对上）：
 *   ① UTIME_NOW / UTIME_OMIT 两个特殊值怎么用；times 数组里每个元素
 *      独立选「现在 / 不管 / 指定值」；
 *   ② 显式设时间只要求「文件属主或 CAP_FOWNER」；而 utime() 时代
 *      「非属主但可写」也能改的老规则在 utimensat 里没有了；
 *   ③ 把 mtime 设到**未来**完全合法——`touch -d` 与 make/备份工具
 *      就是靠它；但**ctime 永远是现在**，内核盖的章盖不回去；
 *   ④ 即使显式指定的时间与旧值一模一样，ctime 也照动——因为
 *      「改时间戳」本身就是一次 inode 状态变更；
 *   ⑤ futimens() 用 fd，utimensat() 有 AT_SYMLINK_NOFOLLOW——
 *      这是全章里唯一能改「符号链接自身」时间戳的接口。
 *
 * 编译： cc -D_DARWIN_C_SOURCE -Wall -Wextra -o c15_3_futimens c15_3_futimens.c
 *   （macOS 缺 UTIME_OMIT 时常量值相同：1,111,111,111ns，为可移植自己定义）
 * 取材： man-pages 6.19 utimensat(2)（UTIME_OMIT / 权限规则段）
 *       TLPI §15.2.2（程序清单 15-3/15-4 讨论段）
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#if defined(__APPLE__)
#define AT(sb)  ((sb)->st_atimespec)
#define MT(sb)  ((sb)->st_mtimespec)
#define CT(sb)  ((sb)->st_ctimespec)
#ifndef UTIME_OMIT
#define UTIME_OMIT ((1ll << 30) - 2ll)   /* 与 Linux 同值 1,111,111,111ns */
#endif
#endif

static void dump(const char *tag, const struct stat *sb)
{
    printf("  %-24s atime=%ld.%09ld  mtime=%ld.%09ld  ctime=%ld.%09ld\n",
           tag, (long) AT(sb).tv_sec, (long) AT(sb).tv_nsec,
           (long) MT(sb).tv_sec, (long) MT(sb).tv_nsec,
           (long) CT(sb).tv_sec, (long) CT(sb).tv_nsec);
}

int main(void)
{
    const char *path = "/tmp/tlpi_c15_3.txt";
    struct stat before, after;

    unlink(path);
    int fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    if (write(fd, "x", 1) != 1) { perror("write"); return EXIT_FAILURE; }

    /* ---------- ①②③④ futimens 的四种组合 ---------- */
    printf("== ① futimens(fd, times)：UTIME_OMIT / UTIME_NOW / 显式值可逐项混搭 ==\n");
    if (fstat(fd, &before) == -1) { perror("fstat"); return EXIT_FAILURE; }
    dump("改前", &before);
    sleep(1);

    /* atime = OMIT（保持不动），mtime = 显式设到「未来」1 小时 */
    struct timespec times[2];
    times[0].tv_sec = 0; times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = MT(&before).tv_sec + 3600;   /* 未来 1 小时 */
    times[1].tv_nsec = 123456789;
    if (futimens(fd, times) == -1) { perror("futimens"); return EXIT_FAILURE; }

    if (fstat(fd, &after) == -1) { perror("fstat2"); return EXIT_FAILURE; }
    dump("atime=OMIT, mtime=未来", &after);
    printf("  → atime %s，mtime 被设到未来 +3600s（ns=%09ld），ctime 动到「现在」\n",
           AT(&before).tv_sec == AT(&after).tv_sec &&
           AT(&before).tv_nsec == AT(&after).tv_nsec ? "一字未动" : "?!",
           (long) MT(&after).tv_nsec);
    printf("  ⚠️ mtime 在未来完全合法（make 的时间戳比较、备份工具都靠它），\n");
    printf("     但 ctime 永远是「这次系统调用发生的时刻」——谁也伪造不了。\n\n");

    /* ---------- ④ 显式时间等于旧值，ctime 也动 ---------- */
    printf("== ④ 显式设成与旧值一模一样，ctime 依然要动 ==\n");
    if (stat(path, &before) == -1) { perror("stat"); return EXIT_FAILURE; }
    dump("改前", &before);
    sleep(1);
    times[0].tv_sec = 0; times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = MT(&before).tv_sec;          /* 与旧 mtime 相同 */
    times[1].tv_nsec = MT(&before).tv_nsec;
    if (futimens(fd, times) == -1) { perror("futimens2"); return EXIT_FAILURE; }
    if (stat(path, &after) == -1) { perror("stat2"); return EXIT_FAILURE; }
    dump("mtime 显式=旧值", &after);
    printf("  → mtime %s，但 ctime %s（inode 状态确实变更了一次）\n",
           MT(&before).tv_sec == MT(&after).tv_sec &&
           MT(&before).tv_nsec == MT(&after).tv_nsec ? "没变（符合预期）" : "变了?!",
           CT(&before).tv_sec != CT(&after).tv_sec ||
           CT(&before).tv_nsec != CT(&after).tv_nsec ? "动了" : "没动(?!)");
    printf("  ⚠️ 「动没动 ctime」判据看的是 inode 是否被写过，不看值是否变化。\n\n");

    /* ---------- ⑤ UTIME_NOW 快捷用法 + utimensat 作用于符号链接 ---------- */
    printf("== ⑤ UTIME_NOW 与 AT_SYMLINK_NOFOLLOW ==\n");
    if (stat(path, &before) == -1) { perror("stat3"); return EXIT_FAILURE; }
    times[0].tv_sec = 0; times[0].tv_nsec = UTIME_NOW;
    times[1].tv_sec = 0; times[1].tv_nsec = UTIME_NOW;
    if (futimens(fd, times) == -1) { perror("futimens3"); return EXIT_FAILURE; }
    if (fstat(fd, &after) == -1) { perror("fstat3"); return EXIT_FAILURE; }
    printf("  futimens(fd, {UTIME_NOW, UTIME_NOW}) → atime/mtime 一起刷成现在\n");
    dump("双 UTIME_NOW", &after);

    /* 符号链接自身的时间戳：utimensat(AT_SYMLINK_NOFOLLOW) 是唯一通道 */
    const char *lnk = "/tmp/tlpi_c15_3.lnk";
    unlink(lnk);
    if (symlink(path, lnk) == -1) { perror("symlink"); return EXIT_FAILURE; }
    struct stat sb_l;
    if (lstat(lnk, &sb_l) == -1) { perror("lstat"); return EXIT_FAILURE; }
    struct timespec lt[2] = {
        { 0, UTIME_OMIT },
        { 946684800, 0 },               /* 2000-01-01 */
    };
    int r = utimensat(AT_FDCWD, lnk, lt, AT_SYMLINK_NOFOLLOW);
    printf("  utimensat(AT_SYMLINK_NOFOLLOW) on symlink = %d（%s）\n",
           r, r == 0 ? "链接自身 mtime 已改到 2000-01-01" : strerror(errno));
    if (r == 0) {
        struct stat sb_l2;
        if (lstat(lnk, &sb_l2) == -1) { perror("lstat2"); return EXIT_FAILURE; }
        printf("  链接 mtime: %ld.%09ld（改前 %ld.%09ld）\n",
               (long) sb_l2.st_mtimespec.tv_sec, (long) sb_l2.st_mtimespec.tv_nsec,
               (long) sb_l.st_mtimespec.tv_sec, (long) sb_l.st_mtimespec.tv_nsec);
    }
    printf("  ⚠️ 不带 AT_SYMLINK_NOFOLLOW 时 utimensat 会跟随链接改到「目标文件」，\n");
    printf("     链接自身的时间戳是全章唯一只能用这个标志改的东西。\n");

    close(fd);
    unlink(lnk);
    unlink(path);
    return EXIT_SUCCESS;
}
