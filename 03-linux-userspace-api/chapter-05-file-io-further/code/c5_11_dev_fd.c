/* c5_11_dev_fd.c — §5.11 /dev/fd 目录：用「fd 的名字」当路径名来用
 *
 * 在正常 Linux 上 /dev/fd 是指向 /proc/self/fd 的符号链接，于是有三件事能做：
 *   1) open("/dev/fd/N", ...) —— 相当于把 N 号 fd 再打开一份
 *   2) cat /dev/fd/N         —— 把某个 fd 当文件读
 *   3) 程序可以用「文件名参数」收到自己的 stdin/stdout（例如 sort /dev/fd/0）
 *
 * 本 demo 演示五件事：
 *   1) CE 沙箱里没有 /dev/fd（ENOENT）—— 它只是符号链接，缺了就缺了；
 *      同级能力改走 /proc/self/fd
 *   2) readlink("/proc/self/fd/N") 能看到这个 fd 指向哪个文件
 *   3) open("/proc/self/fd/N") 之后再读，看它和 N 号是不是共享偏移（实测说话）
 *   4) unlink 掉文件名之后，还能不能通过 /proc/self/fd/N 打开同一个文件
 *   5) 打开目录 /proc/self/fd 列一遍 fd
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_11_dev_fd c5_11_dev_fd.c
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static void list_fds(void)
{
    DIR *d = opendir("/proc/self/fd");
    struct dirent *e;

    if (d == NULL) {
        printf("    opendir 失败: %s\n", strerror(errno));
        return;
    }
    printf("    /proc/self/fd = ");
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] != '.')
            printf("%s ", e->d_name);
    }
    printf("\n");
    closedir(d);
}

int main(void)
{
    const char *p = "/tmp/c5_devfd.txt";
    char link[256];
    int fd, dup_fd;
    ssize_t n;

    printf("== 1. /dev/fd 存在吗？==\n");
    {
        struct stat s;

        errno = 0;
        if (stat("/dev/fd", &s) == -1)
            printf("    stat(\"/dev/fd\") -> -1 errno=%d (%s)：本沙箱没有这个符号链接\n",
                   errno, strerror(errno));
        else
            printf("    stat(\"/dev/fd\") 成功，模式 = 0%o\n", s.st_mode);
        errno = 0;
        if (stat("/proc/self/fd", &s) == -1)
            printf("    stat(\"/proc/self/fd\") -> -1 errno=%d (%s)\n", errno, strerror(errno));
        else
            printf("    stat(\"/proc/self/fd\") 成功 —— 真正干活的是它\n");
    }

    fd = open(p, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    write(fd, "0123456789", 10);

    printf("\n== 2. readlink(\"/proc/self/fd/N\") ==\n");
    {
        char path[64];
        ssize_t r;

        snprintf(path, sizeof path, "/proc/self/fd/%d", fd);
        r = readlink(path, link, sizeof link - 1);
        if (r >= 0)
            link[r] = '\0';
        printf("    fd %d 指向 \"%s\"\n", fd, r >= 0 ? link : "?");
    }

    printf("\n== 3. 列出当前所有 fd ==\n");
    list_fds();

    printf("\n== 4. open(\"/proc/self/fd/N\") 之后，偏移共享吗？（实测）==\n");
    {
        char path[64];

        snprintf(path, sizeof path, "/proc/self/fd/%d", fd);
        dup_fd = open(path, O_RDWR);
        printf("    原 fd = %d，经由 %s 打开的新 fd = %d\n", fd, path, dup_fd);
        if (dup_fd >= 0) {
            printf("    参考：dup(%d) 得到的最小可用号 = %d\n", fd, dup(fd));
            lseek(fd, 0, SEEK_SET);
            {
                char b[8];
                ssize_t got = read(dup_fd, b, 4);     /* 通过新 fd 读 4 字节 */

                printf("    read(新 fd, 4) -> %ld 字节\n", (long) got);
                printf("    原 fd 的 f_pos 现在 = %ld\n", (long) lseek(fd, 0, SEEK_CUR));
                printf("    → 若为 4 则两者共享打开文件表项（等价 dup）；若为 0 则是各自独立的表项\n");
            }
        }
    }

    printf("\n== 5. unlink 掉文件名之后，还能通过 /proc/self/fd/N 打开吗？==\n");
    if (unlink(p) == 0) {
        char path[64];
        ssize_t r;

        snprintf(path, sizeof path, "/proc/self/fd/%d", fd);
        r = readlink(path, link, sizeof link - 1);
        if (r >= 0)
            link[r] = '\0';
        printf("    unlink 后 readlink -> \"%s\"\n", r >= 0 ? link : "?");
        errno = 0;
        {
            int re = open(path, O_RDWR);

            printf("    open(\"%s\") -> %d errno=%d (%s)\n", path, re, errno, strerror(errno));
            if (re >= 0) {
                char b[8];

                lseek(re, 0, SEEK_SET);
                n = read(re, b, 4);
                b[n > 0 ? n : 0] = '\0';
                printf("    从它读回来 -> \"%s\"（文件已经没有名字了，但内容还在）\n", b);
                close(re);
            }
        }
    }

    if (dup_fd >= 0)
        close(dup_fd);
    close(fd);
    return 0;
}
