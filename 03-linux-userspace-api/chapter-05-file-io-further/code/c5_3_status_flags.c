/* c5_3_status_flags.c — §5.3 打开文件状态标志：哪些能在 open 之后改？
 *
 * 关键事实：F_SETFL 只能改「状态标志」里的一小撮，而且访问模式（O_RDONLY/
 * O_WRONLY/O_RDWR）**改不了**——它是 open 那一刻就定死的。内核里干这件事的是
 * fs/fcntl.c 的 setfl()：它只认 O_APPEND / FASYNC / O_NONBLOCK / O_DIRECT /
 * O_NOATIME，其余位直接无视（不报错，静默忽略）。
 *
 * 演示四件事：
 *   1) F_SETFL 能改的：O_APPEND / O_ASYNC / O_NONBLOCK（读回 F_GETFL 验证）
 *   2) F_SETFL 改不了的：O_CREAT / O_TRUNC / O_EXCL —— 静默忽略
 *   3) 访问模式改不了：只读 fd 上 F_SETFL(O_WRONLY) 之后照样写不进去
 *   4) 状态标志属于「打开文件表项」：dup 出来的 fd 共享它
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_3_status_flags c5_3_status_flags.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static const char *accmode(int fl)
{
    switch (fl & O_ACCMODE) {
    case O_RDONLY: return "O_RDONLY";
    case O_WRONLY: return "O_WRONLY";
    case O_RDWR:   return "O_RDWR";
    default:       return "?";
    }
}

int main(void)
{
    const char *p = "/tmp/c5_status.txt";
    int fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        perror("open");
        return 1;
    }
    write(fd, "0123456789", 10);

    printf("== 1. F_SETFL 能改的标志 ==\n");
    printf("  起始: F_GETFL=0x%05x (%s) O_APPEND=%d O_ASYNC=%d O_NONBLOCK=%d\n",
           fcntl(fd, F_GETFL), accmode(fcntl(fd, F_GETFL)),
           !!(fcntl(fd, F_GETFL) & O_APPEND), !!(fcntl(fd, F_GETFL) & O_ASYNC),
           !!(fcntl(fd, F_GETFL) & O_NONBLOCK));

    for (int i = 0; i < 3; i++) {
        int bit = i == 0 ? O_APPEND : i == 1 ? O_ASYNC : O_NONBLOCK;
        const char *nm = i == 0 ? "O_APPEND" : i == 1 ? "O_ASYNC" : "O_NONBLOCK";

        int fl = fcntl(fd, F_GETFL);

        if (fcntl(fd, F_SETFL, fl | bit) == -1)
            printf("  F_SETFL(%s) 失败: %s\n", nm, strerror(errno));
        else
            printf("  F_SETFL(%s) -> F_GETFL=0x%05x，该位=%d\n",
                   nm, fcntl(fd, F_GETFL), !!(fcntl(fd, F_GETFL) & bit));
        /* 复位，免得互相干扰 */
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~bit);
    }

    printf("    为什么 O_ASYNC 这一行没设上？看内核 v6.6 fs/fcntl.c:35：\n");
    printf("      #define SETFL_MASK (O_APPEND | O_NONBLOCK | O_NDELAY | O_DIRECT | O_NOATIME)\n");
    printf("    这个掩码里**没有 FASYNC**；同文件还有一句注释：\n");
    printf("      \"->fasync() is responsible for setting the FASYNC bit.\"\n");
    printf("    也就是说它要靠 file_operations->fasync 钩子去设。普通文件没有这个钩子，\n");
    printf("    于是 F_SETFL(O_ASYNC) 既不报错、也不生效 —— 「静默失败」的典型。\n");
    {
        int pp[2];

        if (pipe(pp) == 0) {                     /* 管道有 .fasync = pipe_fasync */
            int fl = fcntl(pp[0], F_GETFL);

            if (fcntl(pp[0], F_SETFL, fl | O_ASYNC) == -1)
                printf("    管道上 F_SETFL(O_ASYNC) 失败: %s\n", strerror(errno));
            else
                printf("    同一段代码换到管道（fs/pipe.c:1230 有 .fasync）：F_GETFL=0x%05x，"
                       "O_ASYNC 位=%d\n",
                       fcntl(pp[0], F_GETFL), !!(fcntl(pp[0], F_GETFL) & O_ASYNC));
            close(pp[0]);
            close(pp[1]);
        }
    }

    printf("\n== 2. F_SETFL 改不了的标志（静默忽略，不报错）==\n");
    {
        int before = fcntl(fd, F_GETFL);
        int rc = fcntl(fd, F_SETFL, before | O_CREAT | O_TRUNC | O_EXCL);
        int after = fcntl(fd, F_GETFL);

        printf("  F_SETFL(O_CREAT|O_TRUNC|O_EXCL) 返回 %d（成功），但：\n", rc);
        printf("    before=0x%05x  after=0x%05x  变化=0x%05x\n",
               before, after, before ^ after);
        printf("    O_CREAT=%d O_TRUNC=%d O_EXCL=%d —— 一个都没进去\n",
               !!(after & O_CREAT), !!(after & O_TRUNC), !!(after & O_EXCL));
        printf("  O_CREAT/O_TRUNC/O_EXCL 是「open 时的一次性开关」，不是持续状态\n");
    }

    printf("\n== 3. 访问模式改不了 ==\n");
    {
        int rfd = open(p, O_RDONLY);

        fcntl(rfd, F_SETFL, fcntl(rfd, F_GETFL) | O_WRONLY);
        printf("  只读 fd 上 F_SETFL(O_WRONLY) 后 F_GETFL 的访问模式 = %s\n",
               accmode(fcntl(rfd, F_GETFL)));
        errno = 0;
        ssize_t n = write(rfd, "X", 1);
        printf("  往里 write -> %ld errno=%d (%s)\n", (long) n, errno, strerror(errno));
        close(rfd);
    }

    printf("\n== 4. 状态标志跟着「打开文件表项」走，所以 dup 出来的 fd 共享 ==\n");
    {
        int d = dup(fd);

        printf("  dup 前: fd F_GETFL=0x%05x  dup-fd F_GETFL=0x%05x\n",
               fcntl(fd, F_GETFL), fcntl(d, F_GETFL));
        fcntl(d, F_SETFL, fcntl(d, F_GETFL) | O_APPEND);   /* 只动 dup 的那个 */
        printf("  只对 dup-fd 设 O_APPEND 后：\n");
        printf("    fd     F_GETFL=0x%05x O_APPEND=%d\n",
               fcntl(fd, F_GETFL), !!(fcntl(fd, F_GETFL) & O_APPEND));
        printf("    dup-fd F_GETFL=0x%05x O_APPEND=%d  ← 两个一起变\n",
               fcntl(d, F_GETFL), !!(fcntl(d, F_GETFL) & O_APPEND));
        close(d);
    }

    close(fd);
    return 0;
}
