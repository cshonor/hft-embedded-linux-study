/* c5_8_truncate.c — §5.8 截断文件：truncate() / ftruncate()
 *
 * 两个函数干同一件事，差别只在「用路径还是用 fd」：
 *   truncate(path, len)  —— 按路径名，需要该文件的写权限
 *   ftruncate(fd, len)   —— 按 fd，需要 fd 是**可写的**打开方式
 *
 * len 小于当前长度 = 砍掉尾部（数据真没了）；大于当前长度 = 撑长，多出来的
 * 部分读出来是 0，但**不分配磁盘块**（是个洞）——这点用 st_blocks 看得最清楚。
 *
 * 演示六件事：
 *   1) 砍短：读回来的内容跟着短
 *   2) 撑长：新区域读出全 0，st_blocks 不变（洞）
 *   3) 对照：手动写 4096 个 0 字节，st_blocks 会涨
 *   4) 按路径 truncate 也管用
 *   5) 失败路径：不存在的文件 ENOENT / 只读 fd EINVAL / 目录 EISDIR / 负长度 EINVAL
 *   6) truncate 之后偏移可以超过新的文件末尾，但读是 0
 *
 * 编译: gcc -O0 -Wall -Wextra -o c5_8_truncate c5_8_truncate.c
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static void st(const char *tag, int fd)
{
    struct stat s;

    fstat(fd, &s);
    printf("    [%-14s] st_size=%5lld  st_blocks*512=%5lld（物理）\n",
           tag, (long long) s.st_size, (long long) s.st_blocks * 512);
}

/* 读 [off, off+n) 这一段，看看是不是全 0 */
static int region_is_zero(int fd, off_t off, size_t n)
{
    char buf[4096];
    ssize_t got = pread(fd, buf, n, off);

    if (got <= 0)
        return -1;
    for (ssize_t i = 0; i < got; i++)
        if (buf[i] != 0)
            return 0;
    return 1;
}

int main(void)
{
    const char *p = "/tmp/c5_trunc.txt";
    const char *q = "/tmp/c5_trunc_zeros.txt";
    int fd = open(p, O_RDWR | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        perror("open");
        return 1;
    }
    write(fd, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26);     /* 26 字节 */
    printf("== 0. 准备 ==\n");
    st("26 字节", fd);

    printf("\n== 1. ftruncate(fd, 10)：砍掉尾部 16 字节 ==\n");
    if (ftruncate(fd, 10) == -1)
        perror("ftruncate");
    st("截到 10", fd);
    {
        char b[64];
        ssize_t n = pread(fd, b, sizeof b - 1, 0);

        b[n > 0 ? n : 0] = '\0';
        printf("    内容变成 \"%s\"\n", b);
    }

    printf("\n== 2. ftruncate(fd, 8192)：撑长，新区域是洞 ==\n");
    if (ftruncate(fd, 8192) == -1)
        perror("ftruncate");
    st("撑到 8192", fd);
    printf("    [4096,8192) 读出全 0？%s\n",
           region_is_zero(fd, 4096, 4096) == 1 ? "是" : "否");
    printf("    ↑ 逻辑长度 8192，物理占用还停在 4096 —— 多出来的那 4096 字节是「洞」\n");

    printf("\n== 3. 对照：真的写 4096 个 0 字节，物理占用就会涨 ==\n");
    {
        int zf = open(q, O_RDWR | O_CREAT | O_TRUNC, 0644);
        char zeros[8192];

        memset(zeros, 0, sizeof zeros);
        write(zf, zeros, sizeof zeros);
        st("写满 8192", zf);
        printf("    ↑ 同样 8192 字节逻辑长度，物理实打实占了 8192（是上面那个的两倍）\n");
        close(zf);
        unlink(q);
    }

    printf("\n== 4. truncate(path, 5)：按路径名截断 ==\n");
    if (truncate(p, 5) == -1)
        perror("truncate");
    st("truncate 到 5", fd);

    printf("\n== 5. 失败路径 ==\n");
    {
        int rc = truncate("/tmp/c5_not_exist", 5);

        printf("    truncate(\"/tmp/c5_not_exist\", 5) -> %d errno=%d (%s)\n",
               rc, errno, strerror(errno));
    }
    {
        int rfd = open(p, O_RDONLY);
        int rc = ftruncate(rfd, 5);

        printf("    ftruncate(只读 fd, 5)          -> %d errno=%d (%s)\n",
               rc, errno, strerror(errno));
        close(rfd);
    }
    {
        int rc = truncate("/tmp", 5);

        printf("    truncate(\"/tmp\", 5)            -> %d errno=%d (%s)\n",
               rc, errno, strerror(errno));
    }
    {
        int rc = ftruncate(fd, -1);

        printf("    ftruncate(fd, -1)              -> %d errno=%d (%s)\n",
               rc, errno, strerror(errno));
    }

    printf("\n== 6. 截短之后，游标可以留在原处（但读不到东西）==\n");
    lseek(fd, 100, SEEK_SET);                    /* 新的末尾才 5 */
    {
        char b[8];
        ssize_t n = read(fd, b, sizeof b);

        printf("    lseek 到 100 再 read -> %ld（0 = 已经越过 EOF）\n", (long) n);
        printf("    f_pos 现在 = %ld\n", (long) lseek(fd, 0, SEEK_CUR));
    }

    close(fd);
    return 0;
}
