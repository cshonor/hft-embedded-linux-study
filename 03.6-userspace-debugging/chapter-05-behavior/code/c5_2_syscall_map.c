/*
 * c5_2_syscall_map.c —— 把「一行 C 代码」翻译成「一行 strace 输出」
 *
 * 用途：「卡住」和「调了不该调的」这两类问题要在 strace 里找答案，前提是
 *       你得先会**把程序语句和 syscall 对上**。这份程序每做一件事，
 *       就把「这条语句会产生什么 syscall、参数是什么、返回值和 errno 是什么」
 *       自己打印出来 —— 相当于把 strace 的一行行输出在程序里显式展开。
 *
 *       对照着读：程序打印的每一行，都是你以后在 strace 输出里要认的那一行。
 *
 * 用法：./c5_2_syscall_map
 *
 * 编译：gcc -g -O0 -Wall -Wextra -o c5_2_syscall_map c5_2_syscall_map.c
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SHOW(label, expr)                                                  \
    do {                                                                   \
        errno = 0;                                                          \
        long _r = (long)(expr);                                             \
        printf("  %-46s -> 返回 %-4ld errno=%d(%s)\n",                      \
               (label), _r, errno, errno ? strerror(errno) : "-");          \
    } while (0)

int main(void)
{
    char buf[64];
    const char *path = "/tmp/c5_demo_file.txt";

    printf("【1】openat —— 创建/打开文件（strace 里最常见的第一行）\n");
    int fd = -1;
    errno = 0;
    fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    printf("  %-46s -> 返回 %-4d errno=%d(%s)\n",
           "open(O_CREAT|O_WRONLY|O_TRUNC, 0644)", fd,
           errno, errno ? strerror(errno) : "-");
    if (fd < 0)
        return 1;

    printf("【2】write —— 真正把数据交给内核（stdio 缓冲的意义就在这里）\n");
    SHOW("write(fd, \"hello\\n\", 6)", write(fd, "hello\n", 6));

    printf("【3】打开一个不存在的文件 —— 学会读 `= -1 ENOENT`\n");
    errno = 0;
    int bad = open("/tmp/definitely_not_here_12345", O_RDONLY);
    printf("  %-46s -> 返回 %-4d errno=%d(%s)\n",
           "open(\"/tmp/definitely_not_here_12345\", O_RDONLY)",
           bad, errno, errno ? strerror(errno) : "-");

    printf("【4】读回刚写的文件 —— read 的返回值是「字节数」，不是 0/成功\n");
    close(fd);
    errno = 0;
    fd = open(path, O_RDONLY);
    printf("  %-46s -> 返回 %-4d errno=%d(%s)\n",
           "open(path, O_RDONLY)", fd, errno, errno ? strerror(errno) : "-");
    if (fd >= 0) {
        ssize_t n = read(fd, buf, sizeof buf - 1);
        if (n >= 0)
            buf[n] = '\0';
        printf("  %-46s -> 返回 %-4ld errno=%d(%s)  内容=%s",
               "read(fd, buf, 63)", (long)n,
               errno, errno ? strerror(errno) : "-", n > 0 ? buf : "");
        close(fd);
    }

    printf("【5】对一个已经关闭的 fd 操作 —— 学会读 EBADF\n");
    errno = 0;
    ssize_t n = write(fd, "x", 1);
    printf("  %-46s -> 返回 %-4ld errno=%d(%s)\n",
           "write(已关闭的 fd, \"x\", 1)", (long)n,
           errno, errno ? strerror(errno) : "-");

    printf("【6】写 /dev/full —— 学会读 ENOSPC（磁盘满/空间不足）\n");
    errno = 0;
    int full = open("/dev/full", O_WRONLY);
    if (full >= 0) {
        ssize_t m = write(full, "x", 1);
        printf("  %-46s -> 返回 %-4ld errno=%d(%s)\n",
               "write(/dev/full, \"x\", 1)", (long)m,
               errno, errno ? strerror(errno) : "-");
        close(full);
    } else {
        printf("  %-46s -> 打不开（本环境无 /dev/full），跳过\n", "open(\"/dev/full\", O_WRONLY)");
    }

    printf("【7】unlink —— 删除文件（strace 里 `unlinkat(...) = 0`）\n");
    SHOW("unlink(path)", unlink(path));

    printf("\n对照口诀：strace 里每一行的形状是\n");
    printf("  syscall(参数...) = 返回值  [errno]\n");
    printf("  返回 -1 才有 errno；返回 >=0 时 errno 的值为历史残留，别去看它\n");
    return 0;
}
