/* c18_7_remove.c — Ch18 §18.7：remove() 是「分派器」，不是新 syscall
 *
 * 钉住的点（全部实测）：
 *   ① remove() **不是系统调用**，是 C 标准库（stdio.h）里的分派器：
 *      实现上先用 lstat（）判类型 → 目录走 rmdir()，其余走 unlink()；
 *   ② 因此它删**符号链接**时删的是链接本身（lstat 不跟目标）——
 *      哪怕链接指向一个目录也不会去 rmdir 那个目录；
 *   ③ 它继承了两边的全部失败面：非空目录 ENOTEMPTY、不存在 ENOENT；
 *   ④ 对比 unlink(目录)：Linux 给 EISDIR，macOS 给 EPERM——
 *      「都拒绝」是共识，「码是什么」不是。
 *
 * 编译： cc -Wall -Wextra -o c18_7_remove c18_7_remove.c
 * 取材： man-pages 6.19 remove(3)/unlink(2) + TLPI §18.7
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void try_remove(const char *p, const char *tag)
{
    errno = 0;
    int r = remove(p);
    printf("  %-40s = %d %s\n", tag, r, r == 0 ? "" : strerror(errno));
}

int main(void)
{
    const char *base = "/tmp/tlpi_c18_7";
    char d[128], f[160], lnk[160], keep[160];
    snprintf(d,    sizeof d,    "%s/d",     base);
    snprintf(f,    sizeof f,    "%s/d/f",   base);
    snprintf(keep, sizeof keep, "%s/d/keep", base);
    snprintf(lnk,  sizeof lnk,  "%s/d_lnk", base);
    system("rm -rf /tmp/tlpi_c18_7 && mkdir -p /tmp/tlpi_c18_7/d");
    int fd = open(f, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    close(fd);

    /* ---------- ① 分派：文件走 unlink ---------- */
    printf("== ① remove() 自动分派 ==\n");
    try_remove(f, "remove(普通文件)     → unlink 分支");
    try_remove(f, "再删一次（已不存在）");

    /* ---------- ② 分派：空目录走 rmdir ---------- */
    printf("\n== ② 空目录走 rmdir 分支 ==\n");
    try_remove(d, "remove(空目录)       → rmdir 分支");
    mkdir(d, 0755);
    fd = open(keep, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) { perror("open2"); return EXIT_FAILURE; }
    close(fd);
    try_remove(d, "remove(非空目录)     → 继承 ENOTEMPTY");

    /* ---------- ③ 符号链接：删链接，不动目标 ---------- */
    printf("\n== ③ 符号链接：remove 删的是链接本身（lstat 不跟目标）==\n");
    symlink("d", lnk);                       /* 链接指向**目录** d */
    try_remove(lnk, "remove(指向目录的符号链接)");
    struct stat sb;
    printf("  目标 d 还在？ stat(d) = %d（目录安然无恙，只掉了链接）\n",
           stat(d, &sb));

    /* ---------- ④ 对比 unlink(目录) ---------- */
    printf("\n== ④ unlink(目录)：Linux=EISDIR / macOS=EPERM（都拒绝，码不同）==\n");
    errno = 0;
    int r = unlink(d);
    printf("  unlink(目录) = %d errno=%d(%s)\n", r, errno, strerror(errno));
    printf("  → remove() 存在的意义：屏蔽这层类型判断，给你一个统一入口。\n");

    /* 清理 */
    unlink(keep);
    rmdir(d);
    rmdir(base);
    return EXIT_SUCCESS;
}
