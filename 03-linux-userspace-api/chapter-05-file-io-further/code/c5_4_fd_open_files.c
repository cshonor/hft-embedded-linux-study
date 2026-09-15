/* c5_4_fd_open_files.c — §5.4 文件描述符与打开文件的关系：三层结构
 *
 * 内核里访问一个文件要经过三张表：
 *     进程的 fd 表           系统级打开文件表             i-node 表
 *   ┌──────────────┐      ┌────────────────────┐      ┌──────────────┐
 *   │ 0,1,2 ...    │      │ 打开文件表项        │      │  i-node      │
 *   │ fd1 ───────────────▶│ 文件偏移 f_pos      │─────▶│  文件元数据   │
 *   │ fd2 ───────────────▶│ 状态标志 f_flags    │      │  文件内容     │
 *   │ fd3 ──────┐         └────────────────────┘      └──────────────┘
 *   └───────────┼───────────┘         ▲
 *               └─────────────────────┘ 另一个打开文件表项（各自独立的偏移）
 *
 * dup 让两个 fd 指向**同一个**打开文件表项 → 共享偏移与状态标志；
 * 再 open 一次则是**另一个**表项 → 偏移各算各的，但 i-node 是同一个。
 *
 * 本 demo 复刻原书 multi_descriptors.c（习题 5-6）的四步写入，并在每步之后
 * 用一个额外的只读 fd 把文件真读出来看——原书靠 system("cat a") 打印，
 * CE 沙箱里没有 /bin/sh，所以改用原生 read()。
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_4_fd_open_files c5_4_fd_open_files.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define FILE_A "/tmp/c5_multi_a"

/* 另开一个只读 fd 把全文读出来（这个 fd 是第 4 个打开文件表项，不影响上面三个的偏移） */
static void dump(const char *tag)
{
    int r = open(FILE_A, O_RDONLY);
    char buf[64];
    ssize_t n;

    if (r < 0) {
        printf("    [%s] 打开失败: %s\n", tag, strerror(errno));
        return;
    }
    n = read(r, buf, sizeof buf - 1);
    if (n < 0)
        n = 0;
    buf[n] = '\0';
    printf("    [%-16s] 内容 = \"%s\"\n", tag, buf);
    close(r);
}

static long cur(int fd)
{
    return (long) lseek(fd, 0, SEEK_CUR);
}

static void state(int fd1, int fd2, int fd3)
{
    printf("    偏移: fd1=%ld fd2=%ld fd3=%ld  (fd1/fd2 永远相等)\n",
           cur(fd1), cur(fd2), cur(fd3));
}

int main(void)
{
    int fd1 = open(FILE_A, O_RDWR | O_CREAT | O_TRUNC, 0600);
    int fd2, fd3;
    struct stat s1, s3;

    if (fd1 < 0) {
        perror("open fd1");
        return 1;
    }
    fd2 = dup(fd1);                     /* 同一个打开文件表项 */
    fd3 = open(FILE_A, O_RDWR);         /* 另一个打开文件表项 */
    if (fd2 < 0 || fd3 < 0) {
        perror("dup/open");
        return 1;
    }
    printf("== 0. 三个 fd 就位 ==\n");
    printf("    fd1=open(O_RDWR|O_CREAT|O_TRUNC) -> %d\n", fd1);
    printf("    fd2=dup(fd1)                     -> %d\n", fd2);
    printf("    fd3=open(O_RDWR)                 -> %d\n", fd3);
    printf("    fd1/fd2 同一打开文件表项；fd3 是另一个表项\n");

    fstat(fd1, &s1);
    fstat(fd3, &s3);
    printf("    i-node：fd1 的 st_ino=%llu、fd3 的 st_ino=%llu，同一个（都是 %llu）\n",
           (unsigned long long) s1.st_ino, (unsigned long long) s3.st_ino,
           (unsigned long long) s1.st_ino);

    printf("\n== 1. write(fd1, \"Hello,\", 6) ==\n");
    if (write(fd1, "Hello,", 6) != 6)
        perror("write1");
    dump("写 fd1 后");
    state(fd1, fd2, fd3);

    printf("\n== 2. write(fd2, \" world\", 6) —— 接着 fd1 的偏移往后写 ==\n");
    if (write(fd2, " world", 6) != 6)
        perror("write2");
    dump("写 fd2 后");
    state(fd1, fd2, fd3);

    printf("\n== 3. lseek(fd2, 0, SEEK_SET) —— 只动 fd2，但 fd1 的偏移也跟着回 0 ==\n");
    if (lseek(fd2, 0, SEEK_SET) == (off_t) -1)
        perror("lseek");
    dump("lseek 后");
    state(fd1, fd2, fd3);

    printf("\n== 4. write(fd1, \"HELLO,\", 6) —— 从 0 开始覆盖 ==\n");
    if (write(fd1, "HELLO,", 6) != 6)
        perror("write3");
    dump("写 fd1 后");
    state(fd1, fd2, fd3);

    printf("\n== 5. write(fd3, \"Gidday\", 6) —— fd3 的偏移还停在 0 ==\n");
    if (write(fd3, "Gidday", 6) != 6)
        perror("write4");
    dump("写 fd3 后");
    state(fd1, fd2, fd3);

    printf("\n== 6. 状态标志也在打开文件表项里，所以 fd1/fd2 共享、fd3 不共享 ==\n");
    fcntl(fd2, F_SETFL, fcntl(fd2, F_GETFL) | O_APPEND);   /* 只对 fd2 设 */
    printf("    fd1 F_GETFL=0x%05x O_APPEND=%d\n",
           fcntl(fd1, F_GETFL), !!(fcntl(fd1, F_GETFL) & O_APPEND));
    printf("    fd2 F_GETFL=0x%05x O_APPEND=%d\n",
           fcntl(fd2, F_GETFL), !!(fcntl(fd2, F_GETFL) & O_APPEND));
    printf("    fd3 F_GETFL=0x%05x O_APPEND=%d  ← 独立表项，不受影响\n",
           fcntl(fd3, F_GETFL), !!(fcntl(fd3, F_GETFL) & O_APPEND));

    close(fd1);
    close(fd2);
    close(fd3);
    return 0;
}
