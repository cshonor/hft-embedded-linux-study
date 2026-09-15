/* c5_12_temp_files.c — §5.12 创建临时文件：mkstemp / tmpfile / O_TMPFILE
 *
 * 三种做法的取舍：
 *   mkstemp(template)  —— 有名字、有 fd；模板末尾 6 个 X 由内核填随机串，
 *                         创建与打开是原子的（不会被人抢名字）。用完自己 unlink。
 *   tmpfile()          —— 无名文件，FILE* 接口，**关闭即消失**（glibc 内部
 *                         用 O_TMPFILE 或"创建后立刻 unlink"实现）。
 *   O_TMPFILE + linkat —— 真正匿名的文件，连名字都不曾有过；
 *                         需要正式名字时用 linkat 从 /proc/self/fd/N 挂上去。
 *
 * 演示五件事：
 *   1) mkstemp：模板被就地改写、fd 可用、权限是 0600（不是 0644）
 *   2) mkostemp 加 O_CLOEXEC
 *   3) tmpfile：readlink 看真名（通常显示 "(deleted)"）
 *   4) O_TMPFILE：写进匿名文件，再用 linkat 挂一个正式名字出来
 *   5) 三者的"清场"验证：unlink 之后旧的 fd 仍然可读
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_12_temp_files c5_12_temp_files.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static void show_link(const char *tag, int fd)
{
    char path[64];
    char buf[256];
    ssize_t r;

    snprintf(path, sizeof path, "/proc/self/fd/%d", fd);
    r = readlink(path, buf, sizeof buf - 1);
    if (r >= 0)
        buf[r] = '\0';
    printf("    [%-18s] fd=%d 指向 %s\n", tag, fd, r >= 0 ? buf : "?");
}

int main(void)
{
    printf("== 1. mkstemp()：有名字、0600、原子创建 ==\n");
    {
        char tmpl[] = "/tmp/c5tmpXXXXXX";
        int fd = mkstemp(tmpl);
        struct stat st;

        if (fd < 0) {
            perror("mkstemp");
            return 1;
        }
        printf("    内核把模板改写成 \"%s\"\n", tmpl);
        fstat(fd, &st);
        printf("    权限 = 0%o（mkstemp 强制 0600，避免他人抢先读写）\n",
               (unsigned) (st.st_mode & 07777));
        if (write(fd, "secret", 6) != 6)
            perror("write");
        printf("    再读回：");
        {
            char b[16];
            ssize_t n;

            lseek(fd, 0, SEEK_SET);
            n = read(fd, b, sizeof b - 1);
            b[n > 0 ? n : 0] = '\0';
            printf("\"%s\"\n", b);
        }
        printf("    unlink 之后 fd 依然可读（名字没了、inode 还活着）：");
        unlink(tmpl);
        {
            char b[16];
            ssize_t n;

            lseek(fd, 0, SEEK_SET);
            n = read(fd, b, sizeof b - 1);
            b[n > 0 ? n : 0] = '\0';
            printf("\"%s\"\n", b);
        }
        close(fd);
    }

    printf("\n== 2. mkostemp()：要加 O_CLOEXEC ==\n");
    {
        char tmpl[] = "/tmp/c5tmpXXXXXX";
        int fd = mkostemp(tmpl, O_CLOEXEC);

        if (fd < 0)
            perror("mkostemp");
        else {
            printf("    fd=%d，F_GETFD=0x%x（FD_CLOEXEC=0x%x 已置）\n",
                   fd, fcntl(fd, F_GETFD), FD_CLOEXEC);
            close(fd);
            unlink(tmpl);
        }
    }

    printf("\n== 3. tmpfile()：返回 FILE*，名字通常已经不存在 ==\n");
    {
        FILE *tf = tmpfile();

        if (tf == NULL) {
            perror("tmpfile");
        } else {
            int fd = fileno(tf);

            show_link("tmpfile", fd);
            fputs("hello from tmpfile\n", tf);
            fflush(tf);
            rewind(tf);
            printf("    读回来：");
            {
                char b[64];

                if (fgets(b, sizeof b, tf) != NULL)
                    printf("\"%s\"", b);
            }
            printf("    fclose 之后文件自动消失（内部用的是匿名/已 unlink 的文件）\n");
            fclose(tf);
        }
    }

    printf("\n== 4. O_TMPFILE + linkat：先匿名写，想留名时再挂上去 ==\n");
    {
        int fd = open("/tmp", O_RDWR | O_TMPFILE, 0600);

        if (fd < 0) {
            printf("    open(\"/tmp\", O_TMPFILE) 失败：errno=%d (%s)\n",
                   errno, strerror(errno));
        } else {
            char path[64];
            int re;

            show_link("O_TMPFILE", fd);
            if (write(fd, "anonymous!", 10) != 10)
                perror("write");
            snprintf(path, sizeof path, "/proc/self/fd/%d", fd);
            errno = 0;
            re = linkat(AT_FDCWD, path, AT_FDCWD, "/tmp/c5_linked.txt", AT_SYMLINK_FOLLOW);
            if (re == -1) {
                printf("    linkat 挂名失败：errno=%d (%s)\n", errno, strerror(errno));
            } else {
                printf("    挂上名字后 readlink(\"/tmp/c5_linked.txt\") 之外还能读：");
                {
                    int rf = open("/tmp/c5_linked.txt", O_RDONLY);
                    char b[32];
                    ssize_t n;

                    n = rf < 0 ? -1 : read(rf, b, sizeof b - 1);
                    b[n > 0 ? n : 0] = '\0';
                    if (rf >= 0)
                        close(rf);
                    printf("\"%s\"\n", b);
                }
                unlink("/tmp/c5_linked.txt");
            }
            close(fd);
        }
    }

    printf("\n== 5. 小结 ==\n");
    printf("    要名字 + fd        → mkstemp / mkostemp（记得 unlink）\n");
    printf("    只是临时放点数据    → tmpfile / O_TMPFILE（天然不留痕迹）\n");
    printf("    安全要点：不要用 mktemp()（不原子）、也不要用固定文件名 + O_CREAT\n");
    return 0;
}
