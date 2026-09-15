/* ex5_5_shared_offset_flags.c — 原书习题 5-5
 *
 * 题目（逐字）：
 *   Write a program to verify that duplicated file descriptors share a
 *   file offset value and open file status flags.
 *
 * 要验证的是两样东西「共享」，同时要看清一样东西「不共享」：
 *   ✅ 文件偏移 f_pos          —— 共享（在打开文件表项里）
 *   ✅ 打开文件状态标志 F_GETFL —— 共享（同上）
 *   ❌ 文件描述符标志 F_GETFD   —— **不共享**（在 fd 表那一层，FD_CLOEXEC 各算各的）
 * 第三点是本题的鉴别器：如果连 FD_CLOEXEC 都"共享"，说明对三层结构理解错了。
 *
 * 编译: gcc -O0 -Wall -Wextra -o ex5_5_shared_offset_flags ex5_5_shared_offset_flags.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
    const char *p = "/tmp/c5_share.txt";
    int fd1 = open(p, O_RDWR | O_CREAT | O_TRUNC, 0644);
    int fd2, fd3;

    if (fd1 < 0) {
        perror("open");
        return 1;
    }
    write(fd1, "ABCDEFGHIJ", 10);
    fd2 = dup(fd1);                     /* 同一个打开文件表项 */
    fd3 = open(p, O_RDWR);              /* 另一个打开文件表项 */

    printf("== 0. 三个 fd：fd1=%d  fd2=dup(fd1)=%d  fd3=open()=%d ==\n", fd1, fd2, fd3);
    printf("    fd1/fd2 共享打开文件表项；fd3 独立\n\n");

    printf("== 1. 偏移共享 ==\n");
    lseek(fd1, 0, SEEK_SET);
    lseek(fd2, 0, SEEK_SET);
    lseek(fd3, 0, SEEK_SET);
    printf("    先把三个都摆到 0：fd1=%ld fd2=%ld fd3=%ld\n",
           (long) lseek(fd1, 0, SEEK_CUR), (long) lseek(fd2, 0, SEEK_CUR),
           (long) lseek(fd3, 0, SEEK_CUR));
    {
        char b[4] = { 0 };

        if (read(fd1, b, 3) != 3)
            perror("read");
        printf("    从 fd1 读 3 字节后：fd1=%ld fd2=%ld fd3=%ld\n",
               (long) lseek(fd1, 0, SEEK_CUR), (long) lseek(fd2, 0, SEEK_CUR),
               (long) lseek(fd3, 0, SEEK_CUR));
        printf("    → fd2 跟着动了（共享），fd3 没动（独立）\n");
    }

    printf("\n== 2. 状态标志共享（F_GETFL）==\n");
    printf("    初始：fd1=0x%05x fd2=0x%05x fd3=0x%05x\n",
           fcntl(fd1, F_GETFL), fcntl(fd2, F_GETFL), fcntl(fd3, F_GETFL));
    fcntl(fd2, F_SETFL, fcntl(fd2, F_GETFL) | O_APPEND);    /* 只对 fd2 设置 */
    printf("    只对 fd2 设 O_APPEND 之后：\n");
    printf("      fd1=0x%05x O_APPEND=%d\n",
           fcntl(fd1, F_GETFL), !!(fcntl(fd1, F_GETFL) & O_APPEND));
    printf("      fd2=0x%05x O_APPEND=%d\n",
           fcntl(fd2, F_GETFL), !!(fcntl(fd2, F_GETFL) & O_APPEND));
    printf("      fd3=0x%05x O_APPEND=%d\n",
           fcntl(fd3, F_GETFL), !!(fcntl(fd3, F_GETFL) & O_APPEND));
    printf("    → fd1 跟着变了（共享），fd3 没变（独立打开文件表项）\n");

    printf("\n== 3. 别搞错：描述符标志（F_GETFD）**不**共享 ==\n");
    printf("    初始：fd1 F_GETFD=0x%x fd2 F_GETFD=0x%x\n",
           fcntl(fd1, F_GETFD), fcntl(fd2, F_GETFD));
    fcntl(fd2, F_SETFD, FD_CLOEXEC);            /* 只对 fd2 设置 */
    printf("    只对 fd2 设 FD_CLOEXEC 之后：\n");
    printf("      fd1 F_GETFD=0x%x FD_CLOEXEC=%d\n",
           fcntl(fd1, F_GETFD), !!(fcntl(fd1, F_GETFD) & FD_CLOEXEC));
    printf("      fd2 F_GETFD=0x%x FD_CLOEXEC=%d\n",
           fcntl(fd2, F_GETFD), !!(fcntl(fd2, F_GETFD) & FD_CLOEXEC));
    printf("    → 只有 fd2 有。CLOEXEC 属于「哪一个 fd」，不属于「打开的文件」\n");

    printf("\n== 4. 小结（三层结构各管什么）==\n");
    printf("    进程 fd 表      :  fd 号 → 打开文件表项指针；FD_CLOEXEC 存在这里\n");
    printf("    打开文件表项     :  f_pos 偏移、f_flags 状态标志；dup/fork 共享它\n");
    printf("    i-node 表       :  文件类型、权限、大小、数据块；所有打开者共享\n");

    close(fd1);
    close(fd2);
    close(fd3);
    return 0;
}
