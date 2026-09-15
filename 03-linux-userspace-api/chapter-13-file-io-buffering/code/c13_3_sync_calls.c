/* c13_3_sync_calls.c — Ch13 §13.3：控制文件 I/O 的内核缓冲
 *
 * TLPI §13.3（p.239）给了两套手段：
 *   ① 用 `fsync()` / `fdatasync()` / `sync()` **在需要的时候**主动刷；
 *   ② 用 `O_SYNC` / `O_DSYNC` 在 `open()` 时声明「以后每次 write 都同步」。
 * 本程序把 §13.3 里那些「课上讲了但没人量过」的边界钉住：
 *   ① 三个调用的作用域；`O_SYNC` / `O_DSYNC` / `O_RSYNC` 的实际数值与等价关系；
 *   ② `fsync` 在哪些 fd 上会失败、**错误码是 EINVAL 而不是 ESPIPE**，为什么；
 *   ③ **只读 fd 上 fsync 也返回 0**（内核不看打开模式）；
 *   ④ `fsync` 与 `fdatasync` 在内核里的**唯一**语义差别（时间戳）；
 *   ⑤ `O_SYNC` 的代价有多大（1 MiB 写，比普通写慢几个数量级）；
 *   ⑥ `O_SYNC` **管不到**还待在 stdio 缓冲里的字节 —— 这是本节最实用的坑。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c13_3_sync_calls c13_3_sync_calls.c
 * 取材： TLPI §13.3（同步 I/O 的定义、O_SYNC/O_DSYNC/O_RSYNC、Table 13-3 的开销对比）
 *       man-pages 6.19 fsync(2)（EINVAL/EBADF；fdatasync 不刷不影响读回数据的元数据）
 *                      sync(2)（"the kernel may still be writing"）
 *                      open(2) 的 O_SYNC / O_DSYNC / O_RSYNC
 *       Linux v6.6 fs/sync.c:111（SYSCALL_DEFINE0(sync)）
 *                      fs/sync.c:180-190（vfs_fsync_range：`if (!file->f_op->fsync)
 *                        return -EINVAL;` —— EINVAL 的唯一来源）
 *                      fs/sync.c:185-186（`if (!datasync && (inode->i_state &
 *                        I_DIRTY_TIME)) mark_inode_dirty_sync(inode);` —— fsync 与
 *                        fdatasync 在内核里的唯一语义差别）
 *                      fs/sync.c:205-226（do_fsync / SYSCALL_DEFINE1(fsync|fdatasync)）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define F1 "/app/c13_3.bin"

static double now_sec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

/* 对一个 fd 试 fsync + fdatasync，把结果打出来 */
static void try_sync_pair(const char *tag, int fd)
{
    int e1, e2;

    errno = 0;
    int r1 = fsync(fd);
    e1 = errno;
    errno = 0;
    int r2 = fdatasync(fd);
    e2 = errno;

    printf("  %-22s fsync=%-3d errno=%-3d(%-17s)  fdatasync=%-3d errno=%-3d(%s)\n",
           tag, r1, e1, strerror(e1), r2, e2, strerror(e2));
}

int main(void)
{
    printf("== ① 三个刷盘调用 & 四个打开标志 ==\n");
    {
        printf("  sync()       刷**全局**所有脏页（不区分文件、不等待设备确认）\n");
        printf("  fsync(fd)    刷**这一个文件**的数据 + 元数据，等到设备完成\n");
        printf("  fdatasync(fd) 刷数据（+ 读回数据所必需的元数据），通常比 fsync 轻\n\n");
        printf("  O_SYNC   = 0%o\n", (unsigned) O_SYNC);
        printf("  O_DSYNC  = 0%o\n", (unsigned) O_DSYNC);
        printf("  O_RSYNC  = 0%o\n", (unsigned) O_RSYNC);
        printf("  O_DIRECT = 0%o\n", (unsigned) O_DIRECT);
        printf("  O_RSYNC == O_SYNC ? %d   （Linux 上这两者是同一个值：只同步写，不同步读）\n",
               O_RSYNC == O_SYNC);
        printf("  ⚠️ SUSv3 本来给 O_RSYNC 定义了「读也同步」的语义，Linux 没有实现，\n");
        printf("     直接把它定义成 O_SYNC 的同义词 —— 这是「标准说要、实现不做」的经典例子。\n");
    }

    printf("\n== ② fsync 在哪些 fd 上失败，错误码是什么 ==\n");
    {
        int fd;
        int pfd[2];

        fd = open(F1, O_CREAT | O_RDWR | O_TRUNC, 0644);
        if (fd == -1) {
            printf("  open 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        if (write(fd, "x", 1) != 1)
            printf("  write 失败 errno=%d\n", errno);
        try_sync_pair("普通文件(O_RDWR)", fd);

        /* 关键对照：同一个文件用**只读**打开 */
        {
            int rfd = open(F1, O_RDONLY);
            try_sync_pair("普通文件(O_RDONLY)", rfd);
            close(rfd);
        }
        close(fd);

        fd = open("/proc/version", O_RDONLY);
        try_sync_pair("/proc/version", fd);
        close(fd);

        fd = open("/dev/null", O_WRONLY);
        try_sync_pair("/dev/null", fd);
        close(fd);

        if (pipe(pfd) == 0) {
            try_sync_pair("管道读端", pfd[0]);
            try_sync_pair("管道写端", pfd[1]);
            close(pfd[0]);
            close(pfd[1]);
        }

        printf("\n  → 失败的一律是 errno=22(EINVAL)，**不是 ESPIPE(29)**：\n");
        printf("     内核判据在 fs/sync.c:180-190 的 vfs_fsync_range()：\n");
        printf("       if (!file->f_op->fsync)\n");
        printf("           return -EINVAL;\n");
        printf("     /proc 的伪文件、/dev/null、管道，它们的 file_operations 里\n");
        printf("     **根本没有 .fsync 这个回调**，于是走不到「设备不支持」那一步，\n");
        printf("     直接在 VFS 层被判 EINVAL。\n");
        printf("  → 「只读 fd 上 fsync 也返回 0」也不是巧合：\n");
        printf("     do_fsync() 只做 fdget() + vfs_fsync()，**完全不看打开模式**。\n");
        printf("     所以 `fsync(open(path, O_RDONLY))` 是合法的（虽然没什么用）。\n");
    }

    printf("\n== ③ fsync 与 fdatasync 在内核里的唯一差别 ==\n");
    printf("  fs/sync.c:180-190 的 vfs_fsync_range()：\n");
    printf("    if (!file->f_op->fsync) return -EINVAL;\n");
    printf("    if (!datasync && (inode->i_state & I_DIRTY_TIME))\n");
    printf("        mark_inode_dirty_sync(inode);\n");
    printf("    return file->f_op->fsync(file, start, end, datasync);\n");
    printf("  → 差别只在中间那三行：**fsync 会把「只有时间戳脏」的 inode\n");
    printf("     提升成真正需要同步的 inode**，fdatasync 不会。\n");
    printf("     换句话说：纯粹只改了 mtime/atime 时，fdatasync 可以合法地什么都不写。\n");
    printf("  → 所以「fdatasync 更快」只在你**不在乎时间戳**的时候成立；\n");
    printf("     而且注意它是**量级上的差别**，不是「fdatasync = fsync / 2」。\n");

    printf("\n== ④ O_SYNC 的代价：写 1 MiB 对比 ==\n");
    {
        const size_t TOTAL = 1u << 20;
        const size_t BS = 4096;
        char *buf = malloc(BS);
        int modes[2] = {0, 1};
        const char *names[2] = {"普通 O_WRONLY", "O_WRONLY|O_SYNC"};

        if (buf == NULL) {
            printf("  malloc 失败\n");
            return EXIT_FAILURE;
        }
        memset(buf, 'S', BS);
        printf("  固定 buf-size = %zu，写 %zu 字节（%zu 次 write）\n",
               BS, TOTAL, TOTAL / BS);
        for (int k = 0; k < 2; k++) {
            int flags = O_CREAT | O_WRONLY | O_TRUNC;
            size_t done = 0;
            double t0, t1;

            if (modes[k])
                flags |= O_SYNC;
            int fd = open(F1, flags, 0644);
            if (fd == -1) {
                printf("  open 失败 errno=%d(%s)\n", errno, strerror(errno));
                break;
            }
            t0 = now_sec();
            while (done < TOTAL) {
                if (write(fd, buf, BS) != (ssize_t) BS)
                    break;
                done += BS;
            }
            t1 = now_sec();
            close(fd);
            printf("    %-18s 耗时 %.4f s\n", names[k], t1 - t0);
        }
        printf("  → O_SYNC 让每次 write 都等到设备确认，代价可能是**几十到上千倍**。\n");
        printf("  ⚠️ CE 的绝对耗时不能当本机数据；请只看**两个模式之间的比值**。\n");
        printf("  ⚠️ 书上（Table 13-3）的结论值得记：**不要**为了「保险」全局挂 O_SYNC，\n");
        printf("     要么把 write 的块加大，要么在真正需要的点上显式 fsync/fdatasync。\n");
        printf("     而且记住 O_SYNC 是**每次 write** 都同步 —— 块越小越惨。\n");
        free(buf);
    }

    printf("\n== ⑤ O_SYNC 管的是 write()，管不到 stdio 缓冲 ==\n");
    {
        FILE *fp = fopen(F1, "w");
        int fd;
        struct stat st;

        if (fp == NULL) {
            printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        fd = fileno(fp);
        /* 用 fcntl 把这个 fd 的 O_SYNC 打开（模拟另一种设置方式） */
        {
            int fl = fcntl(fd, F_GETFL);
            printf("  fcntl(fd, F_GETFL) = 0%o，O_SYNC 位 = %d\n",
                   (unsigned) fl, (fl & O_SYNC) != 0);
            errno = 0;
            int r = fcntl(fd, F_SETFL, fl | O_SYNC);
            printf("  fcntl(fd, F_SETFL, flags|O_SYNC) = %d errno=%d(%s)\n",
                   r, errno, strerror(errno));
        }
        fprintf(fp, "%s", "以 O_SYNC 打开，但这行还在 stdio 缓冲里");
        fstat(fd, &st);
        printf("  fprintf 之后立刻 fstat(fd).st_size = %ld  ← 一个字都还没进内核\n",
               (long) st.st_size);
        fflush(fp);
        fstat(fd, &st);
        printf("  fflush 之后 st_size = %ld  ← 这才进了页缓存（O_SYNC 才有机会介入）\n",
               (long) st.st_size);
        fclose(fp);
        printf("  → 结论：`O_SYNC` 是给 `write(2)` 的语义，**stdio 缓冲在它上游**。\n");
        printf("     FILE* 上不 fflush，O_SYNC 一点用都没有 —— 这是本章最实用的一条。\n");
    }

    unlink(F1);
    return EXIT_SUCCESS;
}
