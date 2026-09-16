/* ex15_1_perms.c — 习题 15-1：用实验验证 §15.4 的三条权限论断（子集）
 *
 * 题面（据公开习题仓库转述，未逐字核验）：§15.4 有若干关于「各种文件系统
 * 操作需要什么权限」的论断，用 shell 命令或程序验证之。本程序实测其中
 * 三条可由非特权用户验证的：
 *   (a) 把属主权限全剥掉后，属主自己被拒——即使 group/other 全开
 *       （「三选一、不叠加」算法的直接后果，见 c15_6_access.c）；
 *   (b) 目录没有 x（穿越权）时，打开目录内文件失败 EACCES——哪怕
 *       文件本身是 0644；
 *   (c) 目录有 w 没有 r 时，可以在里面创建/删除文件（知道名字即可），
 *       但列不出目录内容。
 *
 * 编译： cc -Wall -Wextra -o ex15_1_perms ex15_1_perms.c
 * 取材： TLPI §15.4.1/15.4.2 习题 15-1
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int try_open(const char *path, int flags, const char *tag)
{
    errno = 0;
    int fd = open(path, flags);
    printf("  [%s] %s\n", tag, fd == -1 ? strerror(errno) : "OK");
    if (fd != -1) close(fd);
    return fd != -1;
}

int main(void)
{
    const char *f = "/tmp/tlpi_ex15_1_f.txt";
    const char *d = "/tmp/tlpi_ex15_1_dir";
    char inner[128];
    snprintf(inner, sizeof inner, "%s/data.txt", d);
    unlink(f); unlink(inner); rmdir(d);

    /* ---------- (a) 属主 0 位：被拒 ---------- */
    puts("== (a) 文件 mode=0077：属主被拒（group/other 全开也没用）==");
    int fd = open(f, O_RDWR | O_CREAT | O_EXCL, 0077);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    close(fd);
    if (chmod(f, 0077) == -1) { perror("chmod"); return EXIT_FAILURE; }
    int ok = try_open(f, O_RDONLY, "属主 open 0077 文件");
    printf("  结论：属主被拒 = %s（论断成立）\n\n", !ok ? "真" : "假?!");

    /* ---------- (b) 目录无 x：穿越被拒 ---------- */
    puts("== (b) 目录 mode=0644（无 x）：打开目录内文件被拒 ==");
    if (mkdir(d, 0755) == -1) { perror("mkdir"); return EXIT_FAILURE; }
    int fd2 = open(inner, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd2 == -1) { perror("open inner"); return EXIT_FAILURE; }
    close(fd2);
    if (chmod(d, 0644) == -1) { perror("chmod dir"); return EXIT_FAILURE; }
    ok = try_open(inner, O_RDONLY, "open(dir 0644)/data.txt");
    printf("  结论：无 x 不能穿越 = %s（论断成立）\n\n", !ok ? "真" : "假?!");

    /* ---------- (c) 目录有 w 没 r：能建文件、不能列目录 ---------- */
    puts("== (c) 目录 mode=0300（-wx）：能创建/删除，不能列 ==");
    if (chmod(d, 0300) == -1) { perror("chmod dir2"); return EXIT_FAILURE; }
    const char *made = "/tmp/tlpi_ex15_1_dir/new_file.txt";
    errno = 0;
    int fd3 = open(made, O_RDWR | O_CREAT | O_EXCL, 0644);
    printf("  在 -wx 目录里创建新文件: %s\n", fd3 == -1 ? strerror(errno) : "OK");
    if (fd3 != -1) close(fd3);
    errno = 0;
    DIR *dp = opendir(d);
    printf("  在 -wx 目录里 opendir() 列内容: %s\n", dp == NULL ? strerror(errno) : "OK");
    if (dp) closedir(dp);
    printf("  结论：w 管「增删」，r 管「列名」，x 管「穿越」——三者独立。\n");

    if (fd3 != -1) { }
    unlink(made);
    unlink(inner);
    chmod(d, 0755);
    rmdir(d);
    unlink(f);
    return EXIT_SUCCESS;
}
