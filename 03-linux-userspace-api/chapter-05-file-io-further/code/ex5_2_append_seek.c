/* ex5_2_append_seek.c — 原书习题 5-2
 *
 * 题目（逐字）：
 *   Write a program that opens and existing file for writing with the
 *   O_APPEND flag, and then seeks to the beginning of the file before
 *   writing some data.  Where does the data appear in the file?  Why?
 *
 * 答案：数据落在**文件末尾**。因为 O_APPEND 改变了 write() 的语义——内核在每次
 * write 时自己原子地跳到末尾再写，f_pos 被无视（不过写完后 f_pos 会被更新到新末尾）。
 *
 * 本 demo 同时跑「带 O_APPEND」和「不带 O_APPEND」两条路，把差别摆在一起看。
 *
 * 编译: gcc -O0 -Wall -Wextra -o ex5_2_append_seek ex5_2_append_seek.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static void dump(const char *tag, const char *path)
{
    int r = open(path, O_RDONLY);
    char b[64];
    ssize_t n = r < 0 ? -1 : read(r, b, sizeof b - 1);

    if (n < 0)
        n = 0;
    b[n] = '\0';
    if (r >= 0)
        close(r);
    printf("    [%s] 内容 = \"%s\"（%ld 字节）\n", tag, b, (long) n);
}

static void make_file(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0 || write(fd, "0123456789", 10) != 10)
        perror("make_file");
    if (fd >= 0)
        close(fd);
}

int main(void)
{
    const char *pa = "/tmp/c5_append_flag.txt";
    const char *pb = "/tmp/c5_no_append_flag.txt";
    char b[16];

    make_file(pa);
    make_file(pb);

    printf("== 准备：两个文件都是 \"0123456789\"（10 字节）==\n");

    printf("\n== A. 带 O_APPEND：先正常追加，再 lseek(0) 后追加 ==\n");
    {
        int fd = open(pa, O_WRONLY | O_APPEND);

        if (fd < 0) {
            perror("open O_APPEND");
            return 1;
        }
        write(fd, "AAA", 3);
        printf("    write(\"AAA\",3) 后 f_pos = %ld\n", (long) lseek(fd, 0, SEEK_CUR));
        dump("写完 AAA", pa);

        lseek(fd, 0, SEEK_SET);
        printf("    lseek(fd, 0, SEEK_SET) 后 f_pos = %ld\n", (long) lseek(fd, 0, SEEK_CUR));
        write(fd, "BBB", 3);
        printf("    在 f_pos=0 处 write(\"BBB\",3) 之后 f_pos = %ld\n",
               (long) lseek(fd, 0, SEEK_CUR));
        dump("写完 BBB", pa);
        printf("    → 数据还是追加到末尾：O_APPEND 让内核每次 write 都自己跳到末尾\n");

        close(fd);
    }

    printf("\n== B. 不带 O_APPEND：同样两步操作 ==\n");
    {
        int fd = open(pb, O_WRONLY);

        if (fd < 0) {
            perror("open");
            return 1;
        }
        write(fd, "AAA", 3);
        printf("    write(\"AAA\",3) 后 f_pos = %ld\n", (long) lseek(fd, 0, SEEK_CUR));
        dump("写完 AAA", pb);

        lseek(fd, 0, SEEK_SET);
        write(fd, "BBB", 3);
        printf("    在 f_pos=0 处 write(\"BBB\",3) 之后 f_pos = %ld\n",
               (long) lseek(fd, 0, SEEK_CUR));
        dump("写完 BBB", pb);
        printf("    → 这次老老实实从偏移 0 开始覆盖\n");

        close(fd);
    }

    printf("\n== 结论 ==\n");
    printf("    O_APPEND 与 f_pos 是两回事：前者是打开文件表项上的一个标志，\n");
    printf("    让 write() 在**同一个原子步骤**里定位到末尾再写；后者只是一个游标。\n");
    printf("    用 lseek 想\"绕开\"O_APPEND 是做不到的。\n");

    memset(b, 0, sizeof b);
    return 0;
}
