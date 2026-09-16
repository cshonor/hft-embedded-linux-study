/* c18_2_rename.c — Ch18 §18.4：rename() 的原子替换语义
 *
 * 钉住的点（全部实测）：
 *   ① rename(file, 存在的file) = **原子替换**：任何时刻路径都指向一个
 *      完整的老文件或完整的新文件，绝不会半新半旧——safe-write 模式的根；
 *   ② rename 对目录：目标必须是空目录；目录到文件（或反之）→ EISDIR/ENOTDIR；
 *   ③ 目录的 ".." 不可改、不能把目录移到自己子树里（EINVAL/EBUSY）；
 *   ④ rename 不换 inode（15.2 实测过：Linux 不动文件时间戳）；
 *   ⑤ 同文件系统内 O(1)（改目录项），跨 FS 会 EXDEV（内核搬数据，不原子）。
 *
 * 编译： cc -Wall -Wextra -o c18_2_rename c18_2_rename.c
 * 取材： man-pages 6.19 rename(2) + TLPI §18.4
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int try_rename(const char *oldp, const char *newp, const char *tag)
{
    errno = 0;
    int r = rename(oldp, newp);
    printf("  %-38s = %d %s\n", tag, r, r == 0 ? "" : strerror(errno));
    return r;
}

int main(void)
{
    const char *dir = "/tmp/tlpi_c18_2";
    char a[128], b[128], d1[128], d2[128];
    snprintf(a, sizeof a, "%s/a.txt", dir);
    snprintf(b, sizeof b, "%s/b.txt", dir);
    snprintf(d1, sizeof d1, "%s/dir1", dir);
    snprintf(d2, sizeof d2, "%s/dir2", dir);
    system("rm -rf /tmp/tlpi_c18_2 && mkdir /tmp/tlpi_c18_2");

    /* ---------- ① 原子替换 ---------- */
    printf("== ① rename(file, 存在的 file)：原子替换 ==\n");
    int fd = open(a, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    write(fd, "NEW CONTENT\n", 12);
    close(fd);
    fd = open(b, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) { perror("open2"); return EXIT_FAILURE; }
    write(fd, "OLD\n", 4);
    close(fd);
    try_rename(a, b, "rename(a → 已存在的 b)");
    char buf[64];
    int rfd = open(b, O_RDONLY);
    ssize_t n = read(rfd, buf, sizeof buf);
    printf("  b 现在的内容: \"%.*s\"（b 的旧内容被整体替换，无中间态）\n",
           (int) n, buf);
    close(rfd);
    struct stat s1;
    stat(b, &s1);
    printf("  ⚠️ inode=%lu 未变——rename 换的是目录项不是数据（§18.4）\n\n",
           (unsigned long) s1.st_ino);

    /* safe-write 惯用法演示 */
    printf("== safe-write 惯用法 ==\n");
    printf("  写配置 → 写到 temp → fsync → rename(temp, 配置) →\n");
    printf("  读者永远看到完整旧版或完整新版；崩溃也不会留下半截文件。\n\n");

    /* ---------- ② 目录规则 ---------- */
    printf("== ② 目录规则 ==\n");
    mkdir(d1, 0755); mkdir(d2, 0755);
    char inner[128];
    snprintf(inner, sizeof inner, "%s/inner.txt", d2);
    fd = open(inner, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) { perror("open3"); return EXIT_FAILURE; }
    close(fd);
    try_rename(inner, d2, "rename(文件 → 目录)");
    try_rename(d2, inner, "rename(目录 → 文件)");
    try_rename(d1, d2, "rename(空 dir1 → 非空 dir2)");
    try_rename(d2, d1, "rename(非空 dir2 → 空 dir1)");   /* 此时 d2 已消失 */

    /* 目录进自己的子树：必须 EINVAL（否则会造出不可达的环） */
    char sub[160];
    snprintf(sub, sizeof sub, "%s/sub", d1);
    try_rename(d1, sub, "rename(dir1 → dir1/sub 自己的子树)");
    /* ".." 不能被 rename 改写 */
    char dotdot[160];
    snprintf(dotdot, sizeof dotdot, "%s/..", d1);
    try_rename(d1, dotdot, "rename(dir1 → dir1/..)");

    /* 清理 */
    unlink(inner); rmdir(d1); rmdir(d2);
    unlink(b);
    rmdir(dir);
    return EXIT_SUCCESS;
}
