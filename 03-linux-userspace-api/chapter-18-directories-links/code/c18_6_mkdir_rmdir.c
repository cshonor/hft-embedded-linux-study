/* c18_6_mkdir_rmdir.c — Ch18 §18.6：mkdir() / rmdir() 的真实语义
 *
 * 钉住的点（全部实测）：
 *   ① mkdir() 的 mode **受 umask 削减**（与 open 同一套规则），
 *      且目录必定带上 x 位才有用（没有 x 连 cd 都进不去）；
 *   ② 目录的 st_nlink 不是「被链接次数」而是 **2 + 子目录数**：
 *      自己一条、"." 一条，每个子目录的 ".." 再各算一条；
 *   ③ rmdir() 只吃**空目录**：非空 ENOTEMPTY、指向文件的 ENOTDIR、
 *      指向目录的**符号链接**也是 ENOTDIR（要用 unlink 删链接本身）；
 *   ④ rmdir(".") / rmdir("..") → EINVAL/EBUSY，内核不许把树上节点摘断；
 *   ⑤ unlinkat(dirfd, name, AT_REMOVEDIR) 是 rmdir 的 *at 版本（§18.11）。
 *
 * 编译： cc -Wall -Wextra -o c18_6_mkdir_rmdir c18_6_mkdir_rmdir.c
 * 取材： man-pages 6.19 mkdir(2)/rmdir(2) + TLPI §18.6
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void try_mkdir(const char *p, mode_t mode, const char *tag)
{
    errno = 0;
    int r = mkdir(p, mode);
    printf("  %-42s = %d %s\n", tag, r, r == 0 ? "" : strerror(errno));
}

static void try_rmdir(const char *p, const char *tag)
{
    errno = 0;
    int r = rmdir(p);
    printf("  %-42s = %d %s\n", tag, r, r == 0 ? "" : strerror(errno));
}

int main(void)
{
    const char *base = "/tmp/tlpi_c18_6";
    char d[128], sub[160], f[160], lnk[160];
    snprintf(d,   sizeof d,   "%s/d",     base);
    snprintf(sub, sizeof sub, "%s/d/sub", base);
    snprintf(f,   sizeof f,   "%s/d/f",   base);
    snprintf(lnk, sizeof lnk, "%s/d_lnk", base);
    system("rm -rf /tmp/tlpi_c18_6 && mkdir /tmp/tlpi_c18_6");

    /* ---------- ① mode & umask ---------- */
    printf("== ① mkdir 的 mode 受 umask 削减 ==\n");
    mode_t old = umask(0);                       /* 先读出当前 umask */
    umask(old);
    printf("  当前 umask = %04o\n", (unsigned) old);
    try_mkdir(d, 0777, "mkdir(d, 0777)");
    struct stat sb;
    stat(d, &sb);
    printf("  实得 mode = %04o（= 0777 & ~%04o）\n",
           (unsigned) (sb.st_mode & 07777), (unsigned) old);

    /* ---------- ② st_nlink 的算法 ---------- */
    printf("\n== ② 目录的 st_nlink = 2 + 子目录数 ==\n");
    stat(d, &sb);
    printf("  空目录 d          st_nlink = %ld   ← 自己\n",
           (long) sb.st_nlink);
    mkdir(sub, 0755);
    stat(d, &sb);
    printf("  建 d/sub 之后 d   st_nlink = %ld   ← +子目录的 \"..\"\n",
           (long) sb.st_nlink);
    printf("  ⚠️ d/sub 自己的 st_nlink = ");
    stat(sub, &sb);
    printf("%ld（新建空目录恒为 2）\n", (long) sb.st_nlink);

    /* ---------- ③ rmdir 的失败面 ---------- */
    printf("\n== ③ rmdir 只吃空目录 ==\n");
    int fd = open(f, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    close(fd);
    try_rmdir(d, "rmdir(非空 d)");
    try_rmdir(f, "rmdir(普通文件)");
    symlink(d, lnk);
    try_rmdir(lnk, "rmdir(指向目录的符号链接)");
    printf("  → 删链接本身请用 unlink；rmdir 只跟到底层对象的类型\n");

    try_rmdir(".", "rmdir(\".\")");
    try_rmdir("..", "rmdir(\"..\")");

    /* ---------- ④ *at 版本 ---------- */
    printf("\n== ④ unlinkat(AT_REMOVEDIR) == rmdir ==\n");
    int dfd = open(d, O_RDONLY | O_DIRECTORY);
    if (dfd == -1) { perror("open dir"); return EXIT_FAILURE; }
    char *slash = strrchr(f, '/');
    errno = 0;
    int r = unlinkat(dfd, slash + 1, 0);              /* 0 = 删文件 */
    printf("  unlinkat(dfd, \"%s\", 0)             = %d %s\n",
           slash + 1, r, r == 0 ? "" : strerror(errno));
    errno = 0;
    r = unlinkat(dfd, "sub", AT_REMOVEDIR);           /* 删空目录 */
    printf("  unlinkat(dfd, \"sub\", AT_REMOVEDIR)   = %d %s\n",
           r, r == 0 ? "" : strerror(errno));
    close(dfd);

    /* ---------- ⑤ mkdir 的存在/父目录检查 ---------- */
    printf("\n== ⑤ mkdir 的前置检查（都是内建原子性，不是先 stat 再建）==\n");
    try_mkdir(d, 0755, "mkdir(已存在的 d)");
    try_mkdir("/tmp/tlpi_c18_6/no/such/parent", 0755, "mkdir(父目录不存在)");

    /* 清理 */
    unlink(lnk);
    try_rmdir(d, "rmdir(空 d)  ← 现在可以了");
    rmdir(base);
    return EXIT_SUCCESS;
}
