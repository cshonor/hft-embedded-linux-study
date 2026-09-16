/* ex15_6_chmod_arX.c — 习题 15-6：实现 chmod a+rX 的 X 语义
 *
 * 题面：chmod a+rX 的意思是「所有人加 r；且仅当它是目录、或它的任何
 * 执行位原本已置位时，才给所有人加 x」（注意是大写 X，小写 x 是无条件加）。
 * 用程序实现这个语义。
 *
 * 实现要点：X 的判定需要**旧 mode**，所以先 stat 再算新 mode，最后
 * 一次 chmod 生效——不能像 chmod(1) 那样只在内核里悄悄做。
 *
 * 编译： cc -Wall -Wextra -o ex15_6_chmod_arX ex15_6_chmod_arX.c
 * 取材： TLPI §15.4.7 习题 15-6；chmod(1) 手册对大写 X 的定义
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* 实现 a+rX：返回 0 成功。入口 mode_out 是计算后的新权限 */
static int
chmodArX(const char *path, mode_t *mode_out)
{
    struct stat sb;
    if (stat(path, &sb) == -1)
        return -1;

    mode_t mode = sb.st_mode;
    mode_t newmode = mode | S_IRUSR | S_IRGRP | S_IROTH;   /* a+r 无条件 */

    int isDir = S_ISDIR(mode);
    int anyX = (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0;
    if (isDir || anyX)                  /* 大写 X：目录或已有任一 x */
        newmode |= S_IXUSR | S_IXGRP | S_IXOTH;

    *mode_out = newmode;
    return chmod(path, newmode);
}

int main(int argc, char *argv[])
{
    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++) {
        struct stat before;
        if (stat(argv[i], &before) == -1) { perror(argv[i]); continue; }
        mode_t after;
        if (chmodArX(argv[i], &after) == -1) {
            perror("chmodArX");
            continue;
        }
        printf("%s: %04lo → %04lo %s\n", argv[i],
               (unsigned long) (before.st_mode & 07777),
               (unsigned long) (after & 07777),
               S_ISDIR(before.st_mode) ? "(目录: x 全加)"
               : (before.st_mode & 0111) ? "(原有 x: x 全加)"
               : "(无 x: 只加 r)");
    }
    return EXIT_SUCCESS;
}
