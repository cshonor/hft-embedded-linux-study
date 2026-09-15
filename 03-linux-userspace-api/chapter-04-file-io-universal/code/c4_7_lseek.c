/* c4_7_lseek.c — lseek()：只改偏移、不碰磁盘；越尾写就造出空洞
 *
 * 演示六件事：
 *   1) SEEK_SET / SEEK_CUR / SEEK_END 三种基准
 *   2) lseek(fd, 0, SEEK_CUR) 是「查当前偏移」的惯用法
 *   3) lseek 不改数据：同一偏移重复读，内容一样
 *   4) 越尾写造空洞：st_size 大、st_blocks 小（逻辑大小 ≠ 物理占用）
 *   5) 空洞区读出来是 0
 *   6) 管道上 lseek -> ESPIPE
 *
 * 编译: gcc -O0 -Wall -o c4_7_lseek c4_7_lseek.c
 */
#define _GNU_SOURCE     /* 必须在所有 include 之前：SEEK_HOLE / SEEK_DATA 靠它才可见 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

static void report_size(const char *tag, const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0) return;
    printf("  %-22s st_size = %8lld 字节   st_blocks = %6lld 块  →  物理占用 ≈ %lld 字节\n",
           tag, (long long) st.st_size, (long long) st.st_blocks,
           (long long) st.st_blocks * 512);
}

int main(void)
{
    char c;
    const char *path = "/tmp/c4_lseek.txt";

    printf("== 1. 三种基准 ==\n");
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    write(fd, "ABCDEFGHIJ", 10);             /* 10 字节 */
    printf("  文件内容 \"ABCDEFGHIJ\"（10 字节），写完偏移 = %ld\n",
           (long) lseek(fd, 0, SEEK_CUR));

    lseek(fd, 3, SEEK_SET);
    read(fd, &c, 1);
    printf("  SEEK_SET 到 3 再读 1 字节 -> '%c'   新偏移 = %ld\n",
           c, (long) lseek(fd, 0, SEEK_CUR));

    lseek(fd, -2, SEEK_CUR);
    read(fd, &c, 1);
    printf("  SEEK_CUR 回退 2 再读 1 字节 -> '%c'  新偏移 = %ld\n",
           c, (long) lseek(fd, 0, SEEK_CUR));

    lseek(fd, -1, SEEK_END);
    read(fd, &c, 1);
    printf("  SEEK_END 回退 1 再读 1 字节 -> '%c'  新偏移 = %ld\n",
           c, (long) lseek(fd, 0, SEEK_CUR));

    printf("\n== 2. lseek 只改偏移，不改数据 ==\n");
    lseek(fd, 0, SEEK_SET);
    read(fd, &c, 1);
    printf("  偏移 0 读到 '%c'\n", c);
    lseek(fd, 0, SEEK_SET);
    read(fd, &c, 1);
    printf("  再回偏移 0 读到 '%c'  ← 数据没被动过，动的只是游标\n", c);

    printf("\n== 3. 越尾写 = 文件空洞（sparse file）==\n");
    close(fd);
    unlink(path);
    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    write(fd, "HEAD", 4);                     /* 真实数据 4 字节 */
    report_size("只写 4 字节", path);

    off_t at = lseek(fd, 1024 * 1024, SEEK_SET);   /* 跳到 1 MiB 处 */
    printf("  lseek 到 1 MiB -> %ld\n", (long) at);
    write(fd, "TAIL", 4);
    report_size("越尾写 4 字节后", path);
    printf("  → 逻辑大小 ≈ 1 MiB + 4，物理占用只有几 KB。中间是空洞。\n");

    printf("\n== 4. 空洞区读出来是 0 ==\n");
    lseek(fd, 4096, SEEK_SET);                /* 跳进空洞中间 */
    char zbuf[8];
    ssize_t n = read(fd, zbuf, sizeof zbuf);
    printf("  从空洞里读 %zd 字节，全部是零？%s\n", n,
           (n == 8 && zbuf[0] == 0 && zbuf[7] == 0) ? "是" : "否");
    printf("  → 没有磁盘块存在，内核在页缓存里「现场造零」。\n");

    printf("\n== 5. SEEK_HOLE / SEEK_DATA：问文件系统「哪里有洞」==\n");
#ifdef SEEK_HOLE
    lseek(fd, 0, SEEK_SET);
    off_t d = lseek(fd, 0, SEEK_DATA);
    printf("  lseek(0, SEEK_DATA) -> %lld   （0：开头就是数据）\n", (long long) d);
    off_t h = lseek(fd, 4, SEEK_HOLE);
    printf("  lseek(4, SEEK_HOLE) -> %lld   ← 不是 4！文件系统按**块**报边界\n",
           (long long) h);
    if (h > 0) {
        d = lseek(fd, h, SEEK_DATA);
        printf("  lseek(%lld, SEEK_DATA) -> %lld   （下一个数据段的起点）\n",
               (long long) h, (long long) d);
    }
    printf("  → 粒度是文件系统块（这里 4096 字节），不是字节。所以洞的范围会被\n");
    printf("    向上取整到块边界；别拿它做逐字节的精确判断。\n");
#else
    printf("  （本平台没定义 SEEK_HOLE；需要 _GNU_SOURCE + 支持该特性的文件系统）\n");
#endif
    close(fd);

    printf("\n== 6. 管道上 lseek：ESPIPE ==\n");
    int p[2];
    pipe(p);
    errno = 0;
    off_t r = lseek(p[0], 0, SEEK_CUR);
    printf("  lseek(pipe, 0, SEEK_CUR) -> %ld  errno=%d (%s)\n",
           (long) r, errno, strerror(errno));
    printf("  → 管道是流，数据读走即消失，根本没有「当前位置」这个概念。\n");
    close(p[0]);
    close(p[1]);
    unlink(path);
    return 0;
}
