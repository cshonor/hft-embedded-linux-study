/* c15_1_stat_family.c — Ch15 §15.1：stat() / lstat() / fstat() 三兄弟到底差在哪
 *
 * TLPI §15.1 只讲了三个调用与 struct stat，但有几个必须实测钉住的点：
 *   ① stat() 跟随符号链接、lstat() 看链接自身——两者对同一个链接
 *      返回的 st_ino / st_size 完全是两个 inode 的信息；
 *   ② fstat() 看的是「fd 打开那一刻就定下来的对象」，路径之后被
 *      换掉（rename / 换符号链接指向）都不影响它——这是躲开
 *      TOCTOU 竞态的正解；
 *   ③ st_size 是逻辑字节数，st_blocks 是 512B 块数；普通文件
 *      st_blocks*512 >= st_size，稀疏文件则可以 st_blocks*512 << st_size；
 *   ④ st_blksize 是「优选 IO 块」，IO 按它的倍数做才不亏；
 *   ⑤ st_dev 用 makedev 的 major/minor 拆开看才是设备号的本义；
 *   ⑥ 现代 glibc/Darwin 的 struct stat 里三个时间戳已经带纳秒
 *      （Linux: st_atim；macOS: st_atimespec）——秒级精度是历史接口。
 *
 * 编译： cc -Wall -Wextra -o c15_1_stat_family c15_1_stat_family.c
 * 取材： man-pages 6.19 stat(2)；TLPI §15.1（Listing 15-1 t_stat.c 的展开版）
 *       Linux v6.6 include/uapi/asm-generic/stat.h（st_blocks 单位 512B）
 *       Darwin xnu bsd/sys/vnode.h（st_blksize 由 FS 层给出）
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void ts_dump(const char *tag, const struct stat *sb)
{
    /* Linux 用 st_atim（struct timespec），macOS 用 st_atimespec */
#if defined(__APPLE__)
    const struct timespec at = sb->st_atimespec;
    const struct timespec mt = sb->st_mtimespec;
    const struct timespec ct = sb->st_ctimespec;
#else
    const struct timespec at = sb->st_atim;
    const struct timespec mt = sb->st_mtim;
    const struct timespec ct = sb->st_ctim;
#endif
    printf("  %s: atime=%ld.%09ld mtime=%ld.%09ld ctime=%ld.%09ld\n",
           tag, (long) at.tv_sec, (long) at.tv_nsec,
           (long) mt.tv_sec, (long) mt.tv_nsec,
           (long) ct.tv_sec, (long) ct.tv_nsec);
}

int main(void)
{
    const char *dir = "/tmp/tlpi_c15_1";
    char target[PATH_MAX], linkpath[PATH_MAX];

    snprintf(target, sizeof target, "%s/target.txt", dir);
    snprintf(linkpath, sizeof linkpath, "%s/link.lnk", dir);

    unlink(linkpath);
    unlink(target);
    mkdir(dir, 0755);

    /* 造一个目标文件 + 一个指向它的符号链接 */
    int fd = open(target, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) { perror("open target"); return EXIT_FAILURE; }
    const char *msg = "hello TLPI ch15\n";
    if (write(fd, msg, strlen(msg)) != (ssize_t) strlen(msg)) {
        perror("write"); return EXIT_FAILURE;
    }
    if (symlink(target, linkpath) == -1) { perror("symlink"); return EXIT_FAILURE; }

    /* ---------- ① stat vs lstat：同一个路径，两个 inode ---------- */
    printf("== ① stat() 跟随链接，lstat() 看链接自身 ==\n");
    struct stat sb_t, sb_l;
    if (stat(linkpath, &sb_t) == -1) { perror("stat"); return EXIT_FAILURE; }
    if (lstat(linkpath, &sb_l) == -1) { perror("lstat"); return EXIT_FAILURE; }
    printf("  %-16s st_ino=%-8lu type=%s st_size=%ld\n",
           "stat(链接)", (unsigned long) sb_t.st_ino,
           S_ISREG(sb_t.st_mode) ? "regular" : "?", (long) sb_t.st_size);
    printf("  %-16s st_ino=%-8lu type=%s st_size=%ld\n",
           "lstat(链接)", (unsigned long) sb_l.st_ino,
           S_ISLNK(sb_l.st_mode) ? "symlink" : "?", (long) sb_l.st_size);
    char buf[PATH_MAX + 1];
    ssize_t n = readlink(linkpath, buf, sizeof buf - 1);
    if (n >= 0) { buf[n] = '\0'; }
    printf("  链接的 st_size=%ld 恰好等于它装的目标路径 \"%s\"（%zd 字节）\n",
           (long) sb_l.st_size, n > 0 ? buf : "?", n);
    printf("  ⚠️ 符号链接的 st_size 不是文件内容长度，是「链接里存的路径串长度」。\n\n");

    /* ---------- ② fstat 与路径解耦：TOCTOU 正解 ---------- */
    printf("== ② fstat() 认 fd 不认路径：rename 之后照样看老对象 ==\n");
    struct stat sb_fd;
    if (fstat(fd, &sb_fd) == -1) { perror("fstat"); return EXIT_FAILURE; }
    printf("  fstat(fd) 的 st_ino = %lu\n", (unsigned long) sb_fd.st_ino);
    /* 把路径换成另一个文件，fd 与 fstat 结果不变 */
    char other[PATH_MAX];
    snprintf(other, sizeof other, "%s/other.txt", dir);
    int fd2 = open(other, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd2 == -1) { perror("open other"); return EXIT_FAILURE; }
    if (rename(target, linkpath) == 0) {
        /* 把 target rename 到 linkpath 上（覆盖链接本身） */
        struct stat sb_after;
        if (fstat(fd, &sb_after) == -1) { perror("fstat2"); return EXIT_FAILURE; }
        printf("  rename(target → linkpath) 后：路径 linkpath 已是普通文件\n");
        if (stat(linkpath, &sb_after) == -1) { perror("stat2"); return EXIT_FAILURE; }
        printf("  stat(linkpath) st_ino = %lu（路径是新的，对象还是老 inode——同目录 rename 不换 inode）\n",
               (unsigned long) sb_after.st_ino);
        struct stat sb_fd2;
        if (fstat(fd, &sb_fd2) == -1) { perror("fstat3"); return EXIT_FAILURE; }
        printf("  fstat(fd)     st_ino = %lu（还是打开时的老 inode！%s）\n",
               (unsigned long) sb_fd2.st_ino,
               sb_fd2.st_ino == sb_fd.st_ino ? "与 rename 前一致" : "?");
        printf("  ⚠️ 这就是「先 open 再 fstat」能躲开 TOCTOU 的原因：\n");
        printf("     路径解析只发生在 open 那一刻，之后对手怎么改路径都碰不到它。\n");
    }
    close(fd2);

    /* ---------- ③④ st_size vs st_blocks vs st_blksize ---------- */
    printf("\n== ③ st_size / st_blocks / st_blksize 三个数各说各的 ==\n");
    int fd3 = open(other, O_RDWR | O_TRUNC, 0644);
    if (fd3 == -1) { perror("open"); return EXIT_FAILURE; }
    /* 写一个稀疏文件：跳过 1 MiB 再写 1 字节 */
    if (lseek(fd3, 1024 * 1024, SEEK_SET) == (off_t) -1) { perror("lseek"); return EXIT_FAILURE; }
    if (write(fd3, "x", 1) != 1) { perror("write"); return EXIT_FAILURE; }
    struct stat sb_sp;
    if (fstat(fd3, &sb_sp) == -1) { perror("fstat"); return EXIT_FAILURE; }
    printf("  稀疏文件：st_size=%lld B, st_blocks=%lld (×512B = %lld B)\n",
           (long long) sb_sp.st_size, (long long) sb_sp.st_blocks,
           (long long) sb_sp.st_blocks * 512);
    printf("  → 逻辑大小 1 MiB+1，实际磁盘占用只有 %lld 块（%lld 字节）\n",
           (long long) sb_sp.st_blocks, (long long) sb_sp.st_blocks * 512);
    printf("  ⚠️ 判断「文件多大」用 st_size，判断「占多少磁盘」用 st_blocks×512。\n");
    printf("  st_blksize = %ld（优选 IO 块；HFT 日志 IO 按它的倍数对齐最省 syscall）\n\n",
           (long) sb_sp.st_blksize);

    /* ---------- ⑤ 设备号 major/minor ---------- */
    printf("== ④⑤ st_dev 与 st_rdev 的 major/minor ==\n");
    struct stat sb_dev;
    if (stat("/dev/zero", &sb_dev) == -1) { perror("stat /dev/zero"); return EXIT_FAILURE; }
    printf("  /dev/zero: st_dev=%lu (所在文件系统), st_rdev=%lu (设备本身)\n",
           (unsigned long) sb_dev.st_dev, (unsigned long) sb_dev.st_rdev);
printf("  st_rdev 拆开: major=%d minor=%d\n",
           (int) major(sb_dev.st_rdev), (int) minor(sb_dev.st_rdev));
    printf("  ⚠️ 只有 S_ISCHR/S_ISBLK 的文件 st_rdev 才有意义，其余类型别读它。\n\n");

    /* ---------- ⑥ 纳秒时间戳 ---------- */
    printf("== ⑥ 三个时间戳已经带纳秒 ==\n");
    ts_dump("target(现 linkpath)", &sb_t);
    ts_dump("symlink 自身      ", &sb_l);
    printf("  ⚠️ 15.2 专门讲这三个戳谁动谁；这里先记住：现代接口拿 timespec，\n");
    printf("     别再用秒级 st_atime 拼精度（见 ex15_3_nanosecond_stat.c）。\n");

    close(fd);
    close(fd3);
    unlink(linkpath);   /* 现在是普通文件（rename 覆盖了链接） */
    unlink(other);
    rmdir(dir);
    return EXIT_SUCCESS;
}
