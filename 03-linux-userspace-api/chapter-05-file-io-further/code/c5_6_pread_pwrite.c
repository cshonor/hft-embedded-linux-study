/* c5_6_pread_pwrite.c — §5.6 在指定偏移读写：pread() / pwrite()
 *
 * 和 read()/write() 的唯一区别：**不动 f_pos**。于是「定位 + 读写」合成一个
 * 原子动作。多线程读同一个 fd 时，用 lseek()+read() 会因为偏移被别人改而错位；
 * pread() 没有这个窗口——它把偏移当参数传，压根不碰共享状态。
 *
 * 演示五件事：
 *   1) pread() 读完 f_pos 不变；lseek()+read() 读完 f_pos 变了
 *   2) pwrite() 写完 f_pos 也不变
 *   3) 偏移超过文件末尾 → pread 返回 0（EOF），pwrite 则把文件撑长、中间是洞
 *   4) 管道上 pread → ESPIPE
 *   5) 用 pread 读「同一个 fd」的多个区间，互不干扰
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_6_pread_pwrite c5_6_pread_pwrite.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static long pos(int fd)
{
    return (long) lseek(fd, 0, SEEK_CUR);
}

static void show_file(const char *tag)
{
    int r = open("/tmp/c5_pread.txt", O_RDONLY);
    char b[64];
    ssize_t n = r < 0 ? -1 : read(r, b, sizeof b - 1);

    if (n < 0)
        n = 0;
    b[n] = '\0';
    if (r >= 0)
        close(r);
    printf("    [%s] 文件 = \"%s\"（%ld 字节）\n", tag, b, (long) n);
}

int main(void)
{
    const char *p = "/tmp/c5_pread.txt";
    int fd = open(p, O_RDWR | O_CREAT | O_TRUNC, 0644);
    char buf[16];
    ssize_t n;

    if (fd < 0) {
        perror("open");
        return 1;
    }
    write(fd, "ABCDEFGHIJ", 10);
    printf("== 0. 准备：写入 \"ABCDEFGHIJ\"，f_pos=%ld ==\n", pos(fd));

    printf("\n== 1. pread() 不碰 f_pos，lseek()+read() 会碰 ==\n");
    memset(buf, 0, sizeof buf);
    n = pread(fd, buf, 4, 0);
    printf("    pread(fd, 4, 0)        -> %ld 字节 \"%.*s\"，随后 f_pos=%ld（没动）\n",
           (long) n, (int) n, buf, pos(fd));

    lseek(fd, 0, SEEK_SET);
    memset(buf, 0, sizeof buf);
    n = read(fd, buf, 4);
    printf("    lseek(0)+read(4)       -> %ld 字节 \"%.*s\"，随后 f_pos=%ld（动了）\n",
           (long) n, (int) n, buf, pos(fd));

    printf("\n== 2. pwrite() 也不碰 f_pos ==\n");
    lseek(fd, 10, SEEK_SET);                 /* 先把游标挪到末尾 */
    printf("    写前 f_pos=%ld\n", pos(fd));
    n = pwrite(fd, "xyz", 3, 5);             /* 写到偏移 5，游标不该变 */
    printf("    pwrite(fd, \"xyz\", 3, 5) -> %ld，写后 f_pos=%ld（还是 10）\n",
           (long) n, pos(fd));
    show_file("pwrite 后");

    printf("\n== 3. 偏移超过文件末尾 ==\n");
    memset(buf, 0, sizeof buf);
    n = pread(fd, buf, 4, 1000);
    printf("    pread(offset=1000)（文件才 10 字节）-> %ld（0 = EOF）\n", (long) n);
    n = pwrite(fd, "Z", 1, 1000);            /* 撑出一个洞 */
    printf("    pwrite(offset=1000) -> %ld\n", (long) n);
    {
        struct stat st;

        fstat(fd, &st);
        printf("    文件现在 st_size=%lld，物理占用 st_blocks*512=%lld\n",
               (long long) st.st_size, (long long) st.st_blocks * 512);
        printf("    ↑ 逻辑长了，但 1000 这个偏移仍落在同一个 4096 字节块内，所以物理没省；\n");
        printf("      偏移跨出块边界时才真正产生「洞」（见 5.8 的 ftruncate 对照）\n");
    }

    printf("\n== 4. 管道上 pread ==\n");
    {
        int pp[2];

        if (pipe(pp) == 0) {
            errno = 0;
            n = pread(pp[0], buf, 4, 0);
            printf("    pread(pipe) -> %ld errno=%d (%s)  ← 管道没有「偏移」这个概念\n",
                   (long) n, errno, strerror(errno));
            close(pp[0]);
            close(pp[1]);
        }
    }

    printf("\n== 5. 同一个 fd 上并发读多个区间，各自不干扰 ==\n");
    {
        char a[4], b[4], c[4];

        pread(fd, a, 3, 0);
        pread(fd, b, 3, 5);
        pread(fd, c, 3, 10);
        a[3] = b[3] = c[3] = '\0';
        printf("    区间 [0,3)=\"%s\"  [5,8)=\"%s\"  [10,13)=\"%s\"   f_pos 始终 = %ld\n",
               a, b, c, pos(fd));
    }

    close(fd);
    return 0;
}
