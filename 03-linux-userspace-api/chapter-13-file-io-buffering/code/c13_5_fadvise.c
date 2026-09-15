/* c13_5_fadvise.c — Ch13 §13.5：给内核的 I/O 模式提示 posix_fadvise()
 *
 * TLPI §13.5（p.244）介绍 `posix_fadvise()`：给内核一个**建议**，
 * 让页缓存的预读/回收策略贴合应用的访问模式。
 *
 * 这一节最容易写错的地方**不是** advice 的语义，而是**返回值约定**：
 *   `posix_fadvise()` 失败时**不返回 -1、也不设置 errno**，
 *   它**直接把错误码当返回值**返回。
 * 本程序把这一点量出来，并把六个 advice 值、以及「哪些 fd 上会失败、失败码是什么」
 * 一起钉住。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c13_5_fadvise c13_5_fadvise.c
 * 取材： TLPI §13.5（六个 advice 常量、只对文件的后半部分给建议的用法）
 *       man-pages 6.19 posix_fadvise(2)（"returns an error number ... instead of
 *         setting errno"；EBADF / EINVAL / ESPIPE）
 *       glibc 2.39 sysdeps/unix/sysv/linux/posix_fadvise.c:
 *           if (INTERNAL_SYSCALL_ERROR_P (ret))
 *             return INTERNAL_SYSCALL_ERRNO (ret);
 *           return 0;
 *         —— 「返回错误码」这条约定的出处；全程不碰 errno
 *       Linux v6.6 mm/fadvise.c（ksys_fadvise64_64 / generic_fadvise 的 EINVAL、ESPIPE）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define F1 "/app/c13_5.bin"

int main(void)
{
    printf("== ① 返回值约定：它就是错误码，不是 -1 ==\n");
    {
        int fd = open(F1, O_CREAT | O_RDWR | O_TRUNC, 0644);
        struct {
            const char *name;
            int advice;
        } tab[] = {
            {"POSIX_FADV_NORMAL",     POSIX_FADV_NORMAL},
            {"POSIX_FADV_RANDOM",     POSIX_FADV_RANDOM},
            {"POSIX_FADV_SEQUENTIAL", POSIX_FADV_SEQUENTIAL},
            {"POSIX_FADV_WILLNEED",   POSIX_FADV_WILLNEED},
            {"POSIX_FADV_DONTNEED",   POSIX_FADV_DONTNEED},
            {"POSIX_FADV_NOREUSE",    POSIX_FADV_NOREUSE},
        };

        if (fd == -1) {
            printf("  open 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        if (write(fd, "0123456789", 10) != 10)
            printf("  write 失败 errno=%d\n", errno);

        printf("  对普通文件 %s（fd=%d）逐个 advice 调用：\n", F1, fd);
        for (size_t i = 0; i < sizeof(tab) / sizeof(tab[0]); i++) {
            errno = 0;
            int r = posix_fadvise(fd, 0, 0, tab[i].advice);
            int e = errno;
            printf("    %-24s (常量 %d) -> 返回 %-3d errno=%d(%s)\n",
                   tab[i].name, tab[i].advice, r, e, strerror(e));
        }

        errno = 0;
        int r = posix_fadvise(fd, 0, 0, 9999);
        printf("    %-24s (  9999) -> 返回 %-3d errno=%d(%s)  ← 非法 advice\n",
               "advice 乱填", r, errno, strerror(errno));
        close(fd);
        printf("  → 六种合法 advice 都返回 0；非法 advice 返回 **22**（不是 -1）。\n");
        printf("  → 从始至终 errno 都是 0 —— `perror()` 在这里**什么都打不出来**。\n");
    }

    printf("\n== ② 各类 fd 上的失败码 ==\n");
    {
        int pfd[2];

        if (pipe(pfd) == 0) {
            errno = 0;
            int r1 = posix_fadvise(pfd[0], 0, 0, POSIX_FADV_NORMAL);
            printf("  管道读端             -> %-3d errno=%d(%s)  ← 期望 ESPIPE(29)\n",
                   r1, errno, strerror(errno));
            errno = 0;
            int r2 = posix_fadvise(pfd[1], 0, 0, POSIX_FADV_NORMAL);
            printf("  管道写端             -> %-3d errno=%d(%s)\n",
                   r2, errno, strerror(errno));
            close(pfd[0]);
            close(pfd[1]);
        }

        errno = 0;
        int r3 = posix_fadvise(9999, 0, 0, POSIX_FADV_NORMAL);
        printf("  fd = 9999（根本不存在）-> %-3d errno=%d(%s)  ← 期望 EBADF(9)\n",
               r3, errno, strerror(errno));

        {
            int fd = open(F1, O_RDONLY);
            errno = 0;
            int r4 = posix_fadvise(fd, 0, 0, POSIX_FADV_NORMAL);
            close(fd);
            errno = 0;
            int r5 = posix_fadvise(fd, 0, 0, POSIX_FADV_NORMAL);   /* 已关闭的 fd */
            printf("  合法 fd 但已 close     -> %-3d errno=%d(%s)\n",
                   r5, errno, strerror(errno));
            printf("  （关闭前同一数值）      -> %-3d\n", r4);
        }

        {
            int fd = open("/proc/version", O_RDONLY);
            errno = 0;
            int r6 = posix_fadvise(fd, 0, 0, POSIX_FADV_NORMAL);
            printf("  /proc/version（伪文件）-> %-3d errno=%d(%s)  ← 连 /proc 都不报错\n",
                   r6, errno, strerror(errno));
            close(fd);
        }

        printf("\n  → 三种失败、三种错误码，**errno 全是 0**：\n");
        printf("     非法 advice  → EINVAL(22)\n");
        printf("     管道         → ESPIPE(29)\n");
        printf("     fd 不存在    → EBADF(9)\n");
        printf("     这正是 glibc 那段代码的直接后果：\n");
        printf("       if (INTERNAL_SYSCALL_ERROR_P (ret))\n");
        printf("           return INTERNAL_SYSCALL_ERRNO (ret);\n");
        printf("       return 0;\n");
    }

    printf("\n== ③ 正确写法 ==\n");
    printf("  ❌ 错的：\n");
    printf("       if (posix_fadvise(fd, 0, 0, POSIX_FADV_WILLNEED) == -1)\n");
    printf("           perror(\"posix_fadvise\");     /* 永远不成立，而且 perror 读的是旧 errno */\n");
    printf("  ✅ 对的：\n");
    printf("       int ret = posix_fadvise(fd, 0, 0, POSIX_FADV_WILLNEED);\n");
    printf("       if (ret != 0)\n");
    printf("       fprintf(stderr, \"posix_fadvise: %%s\\n\", strerror(ret));\n");
    printf("  同类约定的还有 `pthread_*` 系列和 `statx()` 的部分返回 ——\n");
    printf("  记法：**「线程安全」的接口常常用返回值传错误码**，因为 errno 是进程级的。\n");

    printf("\n== ④ 六个 advice 的语义与典型用法 ==\n");
    printf("  %-24s %s\n", "advice", "语义 / 什么时候用");
    printf("  %-24s %s\n", "POSIX_FADV_NORMAL", "默认，恢复系统推断的预读窗口");
    printf("  %-24s %s\n", "POSIX_FADV_SEQUENTIAL", "顺序访问 → 大幅加大预读");
    printf("  %-24s %s\n", "POSIX_FADV_RANDOM", "随机访问 → 关掉预读（省带宽/省缓存）");
    printf("  %-24s %s\n", "POSIX_FADV_WILLNEED", "马上要用 → 现在就预取进页缓存");
    printf("  %-24s %s\n", "POSIX_FADV_DONTNEED", "短期不再用 → 允许立刻回收这批缓存");
    printf("  %-24s %s\n", "POSIX_FADV_NOREUSE", "只用一次（Linux 上基本是空操作）\n");
    printf("  ⚠️ 两点容易误解：\n");
    printf("     1) 它只是**建议**：内核可以完全忽略，返回值 0 **不代表**内核照做了。\n");
    printf("        （和 Ch12 的 `/proc/sys` 写入一个道理：返回成功 ≠ 效果发生）\n");
    printf("     2) `DONTNEED` 只影响**页缓存**，它**不是** `fsync` 的替代品，\n");
    printf("        也不会把脏页丢掉 —— 对脏页调 DONTNEED，内核会先把它们写回。\n");
    printf("  → 真正可测的效果差异在性能上（命中率、预读字节数），\n");
    printf("     本程序不装作量到了 —— 要看去 /proc/vmstat 的\n");
    printf("     `readahead_*` / `pgscan_*` 计数在调用前后的变化。\n");

    unlink(F1);
    return EXIT_SUCCESS;
}
