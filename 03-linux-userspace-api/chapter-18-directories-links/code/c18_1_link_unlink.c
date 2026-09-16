/* c18_1_link_unlink.c — Ch18 §18.1/§18.3：硬链接计数与 unlink 的真实语义
 *
 * 钉住的点（全部实测）：
 *   ① link() = 在目录表里加一条「名字→inode」映射，st_nlink++；
 *      两个名字完全平等，没有"原件/副本"之分；
 *   ② unlink() 删的是**目录项**；st_nlink==0 且无进程持有 fd 才真删；
 *   ③ 「打开后再 unlink」的技巧：fd 继续可读写，close 时内容蒸发
 *      ——临时文件的经典做法（竞态面为零：名字从未存在过）；
 *   ④ 目录不能被 unlink（要 rmdir），且硬链接不能跨文件系统（EXDEV）。
 *
 * 编译： cc -Wall -Wextra -o c18_1_link_unlink c18_1_link_unlink.c
 * 取材： man-pages 6.19 link(2)/unlink(2) + TLPI §18.1/§18.3（Listing 18-1 t_unlink.c）
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void nlink_of(const char *tag, const char *path)
{
    struct stat sb;
    if (stat(path, &sb) == -1) { printf("  %-22s (不存在)\n", tag); return; }
    printf("  %-22s st_nlink=%ld\n", tag, (long) sb.st_nlink);
}

int main(void)
{
    const char *a = "/tmp/tlpi_c18_1_a.txt";
    const char *b = "/tmp/tlpi_c18_1_b.txt";
    unlink(a); unlink(b);

    /* ---------- ① 硬链接：两个平等的名字 ---------- */
    printf("== ① link(): 两个名字，一个 inode ==\n");
    int fd = open(a, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    if (link(a, b) == -1) { perror("link"); return EXIT_FAILURE; }
    nlink_of(a, a);
    struct stat sa, sb;
    stat(a, &sa); stat(b, &sb);
    printf("  %s 与 %s 的 inode 相同? %d（%lu）\n", a, b,
           sa.st_ino == sb.st_ino, (unsigned long) sa.st_ino);
    if (write(fd, "via a\n", 6) != 6) { perror("write"); return EXIT_FAILURE; }
    char buf[64];
    int rfd = open(b, O_RDONLY);
    ssize_t n = read(rfd, buf, sizeof buf);
    printf("  通过 b 读到 %zd 字节（\"%.*s\"）——写 a 读 b，同一份数据\n",
           n, (int) n, buf);
    close(rfd);

    /* ---------- ②③ unlink 与 open 技巧 ---------- */
    printf("\n== ②③ unlink 后 fd 仍可读写，close 即蒸发 ==\n");
    if (unlink(a) == -1) { perror("unlink a"); return EXIT_FAILURE; }
    nlink_of("unlink a 后的 b", b);
    if (write(fd, "still writable\n", 15) != 15) { perror("write2"); return EXIT_FAILURE; }
    if (unlink(b) == -1) { perror("unlink b"); return EXIT_FAILURE; }

    errno = 0;
    int sret = stat(a, &sb);
    printf("  两个名字都没了: stat(a) = %d errno=%d(%s)\n",
           sret, errno, strerror(errno));
    if (lseek(fd, 0, SEEK_SET) == (off_t) -1) { perror("lseek"); return EXIT_FAILURE; }
    n = read(fd, buf, sizeof buf);
    printf("  但 fd 还活着: 读回 %zd 字节（\"%.*s\"）\n", n, (int) n, buf);
    printf("  ⚠️ close(fd) 后内容才真正释放——临时文件先 open 后 unlink，\n");
    printf("     名字从未暴露给其他进程，竞态面为零。\n");

    /* ---------- ④ 目录与跨文件系统 ---------- */
    printf("\n== ④ 边界行为 ==\n");
    errno = 0;
    int r = unlink("/tmp");
    printf("  unlink(/tmp) = %d errno=%d(%s)  ← 目录要走 rmdir\n",
           r, errno, strerror(errno));
    errno = 0;
    r = link("/tmp/tlpi_c18_1_a.txt", "/tmp/x");  /* 源已不存在 */
    printf("  link(不存在的名字) = %d errno=%d(%s)\n", r, errno, strerror(errno));
    printf("  ⚠️ 跨文件系统硬链接 → EXDEV（硬链接是同一 inode 的第二条目录项，\n");
    printf("     inode 换了 FS 就无从谈起）；跨设备请用 symlink（18.5）。\n");

    close(fd);
    return EXIT_SUCCESS;
}
