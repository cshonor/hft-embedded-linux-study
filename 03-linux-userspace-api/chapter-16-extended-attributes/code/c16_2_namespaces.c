/* c16_2_namespaces.c — Ch16 §16.1/§16.2：namespace 规则与 inode 竞态防范
 *
 * TLPI §16.1/§16.2 钉住的点：
 *   ① namespace 规则表：user.* 是唯一"非特权进程随便玩"的命名空间；
 *      system./security./trusted. 各有特权门槛——逐一试探并如实记录结果；
 *   ② user.* 只能挂在**普通文件/目录**上（symlink、socket 等受限），
 *      Linux 的 user.* 甚至不允许挂到 symlink 上——实测；
 *   ③ 16.2 的实现细节里最重要的实用项：**xattr 与数据同 inode 存储**，
 *      所以「先 stat 后操作」的 TOCTOU 竞态同样适用于 xattr——
 *      稳妥做法是拿到 fd 后用 fgetxattr/fsetxattr（走 fd 不走路）；
 *   ④ 权限语义：读 user.* 需要"读文件权限"，写需要"写文件权限"
 *      ——xattr 不是绕过权限的后门（实测只读权限文件上的写入失败）。
 *
 * 编译： cc -Wall -Wextra -o c16_2_namespaces c16_2_namespaces.c
 * 取材： man-pages 6.19 xattr(7)（namespace 权限表）
 *       TLPI §16.1（namespace 规则）、§16.2（存储与 inode 竞态）
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <sys/xattr.h>
#define XSETF(fd,n,v,s,o)   fsetxattr(fd,n,v,s,0,o)
#define XSET(p,n,v,s)       setxattr(p,n,v,s,0,0)
#define XGET(p,n,v,s)       getxattr(p,n,v,s,0,0)
#else
#include <sys/xattr.h>
#define XSETF(fd,n,v,s,o)   fsetxattr(fd,n,v,s,o)
#define XSET(p,n,v,s)       setxattr(p,n,v,s,0)
#define XGET(p,n,v,s)       getxattr(p,n,v,s)
#ifndef XATTR_CREATE
#define XATTR_CREATE 0x1
#endif
#endif

static void try_set(const char *path, const char *name, const char *val)
{
    errno = 0;
    int r = XSET(path, name, val, strlen(val));
    printf("  setxattr(%-24s) = %d  %s\n", name, r,
           r == 0 ? "OK" : (errno ? strerror(errno) : ""));
}

int main(void)
{
    const char *path = "/tmp/tlpi_c16_2.txt";
    unlink(path);
    int fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    write(fd, "base\n", 5);
    close(fd);

    /* ---------- ① namespace 特权门槛 ---------- */
    printf("== ① namespace 特权门槛（非特权进程逐一试探，如实记录）==\n");
    try_set(path, "user.approved", "yes");          /* 一定成功 */
    try_set(path, "system.foo", "x");               /* 特权/平台相关 */
    try_set(path, "security.foo", "x");             /* 特权/平台相关 */
    try_set(path, "trusted.foo", "x");              /* Linux: 需 CAP_SYS_ADMIN */
    printf("  ⚠️ man-pages 的规则：user.* 非特权可读写；system.* 由内核/实现\n");
    printf("     解释；security.* 写要 CAP_SYS_ADMIN；trusted.* 读写都要\n");
    printf("     CAP_SYS_ADMIN。本机实测结果如上——跨平台程序只依赖 user.*。\n\n");

    /* ---------- ② user.* 在 symlink 上的限制 ---------- */
    printf("== ② user.* 与符号链接 ==\n");
    const char *lnk = "/tmp/tlpi_c16_2.lnk";
    unlink(lnk);
    if (symlink(path, lnk) == -1) { perror("symlink"); return EXIT_FAILURE; }
    errno = 0;
    int r = XSET(lnk, "user.onlink", "x", 1);          /* 跟随链接 → 设到目标上 */
    printf("  setxattr(链接, user.onlink) = %d errno=%d(%s)\n", r, errno, strerror(errno));
    if (r == 0) {
        char buf[64];
        ssize_t g = XGET(path, "user.onlink", buf, sizeof buf);
        printf("  → 不带 follow 抑制时，落在**目标文件**上（目标可查到 %zd 字节）\n", g);
    }
    printf("  ⚠️ man-pages：Linux 上 user.* **根本不允许**挂到 symlink 自身\n");
    printf("     （user xattr 只支持文件/目录）；system.* 里个别前缀才行。\n\n");

    /* ---------- ③ fd 版接口与 ④ 权限语义 ---------- */
    printf("== ③④ fsetxattr 走 fd + 权限语义（写 user.* 需要文件的写权限）==\n");
    int fd2 = open(path, O_WRONLY);
    if (fd2 == -1) { perror("open2"); return EXIT_FAILURE; }
    errno = 0;
    r = XSETF(fd2, "user.viafd", "fd-based", 8, 0);
    printf("  fsetxattr(fd, user.viafd) = %d（fd 版：stat 后文件被换也换不走）\n", r);
    close(fd2);

    if (chmod(path, 0444) == -1) { perror("chmod"); return EXIT_FAILURE; }
    errno = 0;
    r = XSET(path, "user.hack", "x", 1);
    printf("  文件改 0444 后 setxattr(user.hack) = %d errno=%d(%s)\n",
           r, errno, strerror(errno));
    printf("  → user.* 的写权限 = 文件写权限，不是后门。\n");

    unlink(lnk);
    unlink(path);
    return EXIT_SUCCESS;
}
