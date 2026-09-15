/* c5_5_dup.c — §5.5 复制文件描述符：dup / dup2 / dup3 / F_DUPFD
 *
 * 四个接口的差别只有两条：**新号怎么选**、**目标号已占用怎么办**、**CLOEXEC 带不带**。
 *   dup(fd)                 最小可用号，不带 CLOEXEC
 *   dup2(old, new)          指定 new；若 new 已打开则先关掉它；old==new 时特例：
 *                           不关，直接校验 old 有效后原样返回
 *   dup3(old, new, flags)   Linux 特有；old==new 报 EINVAL；flags 只能是 O_CLOEXEC
 *   fcntl(fd, F_DUPFD, min) 不小于 min 的最小可用号（dup2 的"原子版"替代品）
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_5_dup c5_5_dup.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
    const char *p = "/tmp/c5_dup.txt";
    int fd = open(p, O_RDWR | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        perror("open");
        return 1;
    }
    write(fd, "ABCDEFGHIJ", 10);

    printf("== 1. dup() 取「最小可用号」——所以关掉 0 之后它会给 0 ==\n");
    printf("  fd = %d\n", fd);
    {
        int d = dup(fd);

        printf("  dup(fd)                     -> %d（3 已被占，给最小的空号）\n", d);
        close(d);
        close(0);                               /* 故意腾出 0 号 */
        d = dup(fd);
        printf("  close(0) 之后 dup(fd)       -> %d  ← 把 0 号占了\n", d);
        close(d);
    }
    /* 0 号已经关了，后面 open 会拿到 0；先把它补回来，免得干扰 */
    if (open("/dev/null", O_RDONLY) != 0)
        fprintf(stderr, "警告：没能补回 0 号\n");

    printf("\n== 2. dup2(old, new)：指定目标号，必要时先关旧的 ==\n");
    {
        int r = dup2(fd, 7);

        printf("  dup2(fd, 7)                 -> %d，7 号现在指向同一打开文件表项\n", r);
        printf("    偏移共享验证：write(7) 前 fd 的偏移=%ld\n", (long) lseek(fd, 0, SEEK_CUR));
        lseek(7, 4, SEEK_SET);
        printf("    lseek(7, 4) 之后 fd 的偏移=%ld  ← 一个动了两个都动\n",
               (long) lseek(fd, 0, SEEK_CUR));
        close(7);
    }

    printf("\n== 3. dup2(fd, fd)：old == new 的特例，不关、直接返回原号 ==\n");
    printf("  dup2(fd, fd) -> %d（fd 本身）\n", dup2(fd, fd));
    {
        int rc = dup2(999, 999);                 /* 先调用、后取 errno */

        printf("  dup2(999, 999) -> %d errno=%d (%s)  ← 此时要校验 old 有效\n",
               rc, errno, strerror(errno));
    }

    printf("\n== 4. dup3(old, new, flags)：Linux 特化版，old==new 直接 EINVAL ==\n");
    {
        int rc = dup3(fd, fd, 0);

        printf("  dup3(fd, fd, 0) -> %d errno=%d (%s)\n",
               rc, errno, strerror(errno));
    }
    {
        int d = dup3(fd, 20, O_CLOEXEC);

        printf("  dup3(fd, 20, O_CLOEXEC) -> %d，F_GETFD=0x%x（FD_CLOEXEC=0x%x 已置）\n",
               d, fcntl(d, F_GETFD), FD_CLOEXEC);
        close(d);
    }

    printf("\n== 5. CLOEXEC 差异：dup 不带，dup3 才带 ==\n");
    {
        int a = dup(fd);
        int b = dup3(fd, 30, O_CLOEXEC);

        printf("  dup(fd)          -> %d F_GETFD=0x%x\n", a, fcntl(a, F_GETFD));
        printf("  dup3(fd,30,CLOEXEC) -> %d F_GETFD=0x%x\n", b, fcntl(b, F_GETFD));
        printf("  （CLOEXEC 的意思是 exec 之后这个 fd 会被自动关掉）\n");
        close(a);
        close(b);
    }

    printf("\n== 6. 错误路径 ==\n");
    {
        int rc = dup(999);

        printf("  dup(999)      -> %d errno=%d (%s)\n", rc, errno, strerror(errno));
    }
    {
        int rc = dup2(999, 7);

        printf("  dup2(999, 7)  -> %d errno=%d (%s)\n", rc, errno, strerror(errno));
    }

    close(fd);
    return 0;
}
