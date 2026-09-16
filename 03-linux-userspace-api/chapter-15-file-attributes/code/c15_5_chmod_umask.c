/* c15_5_chmod_umask.c — Ch15 §15.4.5/15.4.6/15.4.7：umask、chmod 与三个特殊位
 *
 * TLPI §15.4 必须实测钉住的点：
 *   ① umask 只作用于 open()/mkdir() 的 mode 参数，**不作用于 chmod()**；
 *   ② umask 的方向是反的：掩码里为 1 的位**从创建模式里被剥掉**；
 *   ③ chmod(2) 对普通文件设置 SUID/SGID/sticky 不需要特权——需要特权的
 *      是 chown；但 SUID 程序的安全性在 15.4.5 里有一整套规则；
 *   ④ 目录上的 x 位 = 「穿越权」（进得去），r 位 = 「列目录」（看得见）；
 *      目录没有 x，r 再大也只能看名字不能 open 里面的文件——这是
 *      HFT 里「目录最小权限」设计的理论根（15-1 习题一并验证）；
 *   ⑤ sticky bit 在现代系统上对**目录**才有意义（/tmp），对文件是
 *      历史遗留（连带「伪换出」语义）。
 *
 * 编译： cc -Wall -Wextra -o c15_5_chmod_umask c15_5_chmod_umask.c
 * 取材： man-pages 6.19 umask(2) / chmod(2) / path_resolution(7)
 *       TLPI §15.4.2（目录权限表）、§15.4.5-15.4.7
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void pmode(const char *tag, const char *path)
{
    struct stat sb;
    if (stat(path, &sb) == -1) { perror("stat"); exit(EXIT_FAILURE); }
    printf("  %-28s mode=%04lo\n", tag, (unsigned long) (sb.st_mode & 07777));
}

static int try_open(const char *path, int flags, const char *tag)
{
    errno = 0;
    int fd = open(path, flags);
    printf("  %-34s %s\n", tag, fd == -1 ? strerror(errno) : "OK");
    if (fd != -1) close(fd);
    return fd != -1;
}

int main(void)
{
    char f1[64], d1[64], f2[64];
    snprintf(f1, sizeof f1, "/tmp/tlpi_c15_5_f1.txt");
    snprintf(f2, sizeof f2, "/tmp/tlpi_c15_5_f2.txt");
    snprintf(d1, sizeof d1, "/tmp/tlpi_c15_5_dir");
    unlink(f1); unlink(f2); unlink(d1);

    /* ---------- ①② umask 的方向与作用范围 ---------- */
    printf("== ①② umask 剥 mode：022 掩码把 0666 剪成 0644 ==\n");
    mode_t old = umask(S_IWGRP | S_IWOTH);      /* 022 */
    printf("  旧 umask=%04lo，现设为 022\n", (unsigned long) old);
    int fd = open(f1, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    pmode("open(0666) 实得", f1);
    printf("  → 666 & ~022 = 644，去掉的正是「组的 w」和「其他的 w」\n");
    if (chmod(f1, 0666) == -1) { perror("chmod"); return EXIT_FAILURE; }
    pmode("chmod(0666) 之后", f1);
    printf("  → chmod **不受 umask 管**：umask 只在创建那一刻参与。\n");
    umask(old);

    /* mkdir 同样被剪：777 → 755 */
    mode_t u2 = umask(022);
    if (mkdir(d1, 0777) == -1) { perror("mkdir"); return EXIT_FAILURE; }
    pmode("mkdir(0777) 实得", d1);
    umask(u2);
    printf("  → 777 & ~022 = 755（目录也一样被剪）\n\n");

    /* ---------- ④ 目录 r 与 x 的区别 ---------- */
    printf("== ④ 目录的 r=「列名字」 x=「穿越/打开」 ==\n");
    int fdx = open(f2, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fdx == -1) { perror("open2"); return EXIT_FAILURE; }
    if (write(fdx, "secret\n", 7) != 7) { perror("write"); return EXIT_FAILURE; }
    close(fdx);
    char inner[128];
    snprintf(inner, sizeof inner, "%s/f2.txt", d1);
    if (rename(f2, inner) == -1) { perror("rename"); return EXIT_FAILURE; }

    if (chmod(d1, 0444) == -1) { perror("chmod dir"); return EXIT_FAILURE; }  /* r-- */
    try_open(inner, O_RDONLY, "dir=r-- 时 open(dir/file):");
    try_open(d1, O_RDONLY | O_DIRECTORY, "dir=r-- 时 open(dir):");

    if (chmod(d1, 0333) == -1) { perror("chmod dir2"); return EXIT_FAILURE; } /* -wx */
    try_open(inner, O_RDONLY, "dir=-wx 时 open(dir/file):");
    printf("  → 只有 x 没有 r：摸得到里面的文件（知道名字就能开），\n");
    printf("     但 ls 列不出目录内容——「能进不能看」的目录就是这么造的。\n\n");

    /* ---------- ③⑤ 特殊位 ---------- */
    printf("== ③⑤ 特殊位：文件 SUID/SGID、目录 sticky ==\n");
    int fd2 = open(f2, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd2 == -1) { perror("open3"); return EXIT_FAILURE; }
    close(fd2);
    if (chmod(f2, 04755) == -1) { perror("chmod suid"); return EXIT_FAILURE; }
    pmode("chmod u+s,g+rx(4755)", f2);
    if (chmod(d1, 01777) == -1) { perror("chmod sticky"); return EXIT_FAILURE; }
    pmode("chmod 1777 (sticky dir)", d1);
    printf("  → 文件高位 4 = SUID；目录高位 1 = sticky（/tmp 的防删规则），\n");
    printf("     sticky 对普通文件只是历史语义，现代内核当摆设。\n");
    printf("  ⚠️ chmod 设置特殊位不需要特权，chown 才需要——两者能力边界不同。\n");

    unlink(f2);
    chmod(d1, 0755);
    rmdir(d1);
    unlink(f1);
    return EXIT_SUCCESS;
}
