/* ex5_4_my_dup.c — 原书习题 5-4
 *
 * 题目（逐字）：
 *   Implement dup() and dup2() using fcntl() and, where necessary,
 *   close().  (You may ignore the fact that dup2() and fcntl() return
 *   different errno values for some error cases.)  For dup2(), remember to
 *   handle the special case where oldfd equals newfd.  In this case, you
 *   should check whether oldfd is valid, which can be done by, for
 *   example, checking if fcntl(oldfd, F_GETFL) succeeds.  If oldfd is not
 *   valid, then the function should return -1 with errrno set to EBADF.
 *
 * 思路：
 *   my_dup(fd)         = fcntl(fd, F_DUPFD, 0)      —— 0 表示「不小于 0 的最小可用号」
 *   my_dup2(old, new)  = 先处理 old == new 的特例（只校验有效性，别关它！）
 *                        否则先 close(new) 腾号，再 fcntl(old, F_DUPFD, new)
 *
 * 讲这个题的意义在于：dup2() 的"若 new 已打开就先关掉它"这一步，只有在
 * fcntl(F_DUPFD) 这个原语之上才是安全的（关完之后目标号一定空着，不会被抢）。
 *
 * 编译: gcc -O0 -Wall -Wextra -o ex5_4_my_dup ex5_4_my_dup.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* dup() 的等价实现：复制到「不小于 0 的最小可用号」 */
static int my_dup(int fd)
{
    return fcntl(fd, F_DUPFD, 0);
}

/* dup2() 的等价实现 */
static int my_dup2(int oldfd, int newfd)
{
    if (newfd < 0) {                    /* 负数号不合法（dup2 对它是 EBADF） */
        errno = EBADF;
        return -1;
    }

    if (oldfd == newfd) {
        /* 特例：不能 close 再 dup——那会把唯一的 fd 弄丢。
           只需要校验 oldfd 是否有效即可。 */
        return fcntl(oldfd, F_GETFL) == -1 ? -1 : newfd;
    }

    /* 先确认 oldfd 有效（也让错误码是 EBADF 而不是别的东西） */
    if (fcntl(oldfd, F_GETFL) == -1)
        return -1;

    close(newfd);                       /* newfd 原样打开着就先关掉（关一个没开的号也只是 EBADF） */
    return fcntl(oldfd, F_DUPFD, newfd);   /* 取「不小于 newfd 的最小可用号」——此时就是 newfd */
}

int main(void)
{
    const char *p = "/tmp/c5_mydup.txt";
    int fd = open(p, O_RDWR | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        perror("open");
        return 1;
    }
    write(fd, "ABCDEFGHIJ", 10);

    printf("== 1. my_dup() 与真 dup() 对照 ==\n");
    {
        int a = my_dup(fd);
        int b = dup(fd);
        char x[4] = { 0 }, y[4] = { 0 };

        printf("    my_dup(%d) -> %d    真 dup(%d) -> %d\n", fd, a, fd, b);
        lseek(a, 3, SEEK_SET);                      /* a 动偏移 */
        printf("    lseek(a, 3) 之后：a 的 f_pos=%ld，原 fd 的 f_pos=%ld（共享）\n",
               (long) lseek(a, 0, SEEK_CUR), (long) lseek(fd, 0, SEEK_CUR));
        lseek(fd, 0, SEEK_SET);
        if (read(a, x, 3) != 3 || read(fd, y, 3) != 3)
            perror("read");
        printf("    经由 a 和原 fd 各读 3 字节：\"%s\" / \"%s\"（同一个打开文件表项）\n", x, y);
        close(a);
        close(b);
    }

    printf("\n== 2. my_dup2(fd, 42)：指定目标号 ==\n");
    {
        int r = my_dup2(fd, 42);

        printf("    my_dup2(%d, 42) -> %d\n", fd, r);
        printf("    lseek(42, 7) 之后原 fd 的 f_pos = %ld（共享偏移，确认是同一个表项）\n",
               (long) (lseek(42, 7, SEEK_SET), lseek(fd, 0, SEEK_CUR)));
        close(42);
    }

    printf("\n== 3. my_dup2(fd, fd)：old == new 的特例 ==\n");
    printf("    my_dup2(%d, %d) -> %d（原样返回，不能关也不能复制）\n",
           fd, fd, my_dup2(fd, fd));
    {
        int rc = my_dup2(999, 999);              /* 先调用、后取 errno */

        printf("    my_dup2(999, 999) -> %d errno=%d (%s)  ← 无效 fd 必须报 EBADF\n",
               rc, errno, strerror(errno));
    }
    {
        int rc = dup2(999, 999);

        printf("    真 dup2(999, 999) -> %d errno=%d (%s)  ← 行为一致\n",
               rc, errno, strerror(errno));
    }

    printf("\n== 4. 目标号已被占用时，先关再复制 ==\n");
    {
        int other = open("/dev/null", O_RDONLY);     /* 占一个新号 */

        printf("    先用 /dev/null 占住 %d 号\n", other);
        printf("    my_dup2(%d, %d) -> %d，随后向 %d 写数据看落到哪里：\n",
               fd, other, my_dup2(fd, other), other);
        if (write(other, "ZZZ", 3) != 3)
            perror("write");
        {
            int rf = open(p, O_RDONLY);
            char b[32];
            ssize_t n = read(rf, b, sizeof b - 1);

            b[n > 0 ? n : 0] = '\0';
            close(rf);
            printf("    文件现在是 \"%s\"  ← 说明 %d 号确实被 my_dup2 重新指向了文件\n",
                   b, other);
        }
        close(other);
    }

    printf("\n== 5. 错误路径 ==\n");
    {
        int rc = my_dup(999);

        printf("    my_dup(999)      -> %d errno=%d (%s)\n", rc, errno, strerror(errno));
    }
    {
        int rc = my_dup2(999, 7);

        printf("    my_dup2(999, 7)  -> %d errno=%d (%s)\n",
               rc, errno, strerror(errno));
    }

    close(fd);
    return 0;
}
