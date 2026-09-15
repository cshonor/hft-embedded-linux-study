/* c13_4_two_layers.c — Ch13 §13.4：I/O 缓冲总结（两层数据流，逐级量出来）
 *
 * TLPI §13.4（p.243）把前面三节合成一张图：
 *
 *   写：应用 → [stdio 缓冲] --fflush--> [内核页缓存] --fsync/fdatasync--> 磁盘
 *   读：磁盘 → [内核页缓存] --read----> [stdio 缓冲] --fgets/fread------> 应用
 *
 * 这张图人人都能背，但「某一刻数据到底在哪一层」很少被真量过。
 * 本程序用**外部探针**逐级问同一个问题：
 *   「另开一个 fd 去看这个文件，能看见多少字节？」
 * 因为 stdio 的缓冲、内核的页缓存、磁盘三者对外部观察者的可见性**层层不同**，
 * 所以这个探针能把「数据在哪一层」一格一格地量出来。
 *
 * 每一级同时记录 /proc/vmstat 的 nr_dirty（整机脏页数），
 * 用来区分「进了页缓存」与「落到磁盘」。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c13_4_two_layers c13_4_two_layers.c
 * 取材： TLPI §13.4（Figure 13-2 / 13-3 的两层数据流）
 *       man-pages 6.19 fflush(3)（只刷 stdio 缓冲，不落盘）
 *                      fsync(2)（把 in-core 数据交给存储设备）
 *       Linux v6.6 fs/sync.c:180-190（vfs_fsync_range）
 *                      mm/page-writeback.c（nr_dirty 的维护与 writeback 触发）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define F1 "/app/c13_4.txt"
#define PAYLOAD "0123456789ABCDEF"

static long vmstat_val(const char *key)
{
    FILE *fp = fopen("/proc/vmstat", "r");
    char line[256];
    long v = -1;

    if (fp == NULL)
        return -1;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, key, strlen(key)) == 0) {
            if (sscanf(line + strlen(key), " %ld", &v) != 1)
                v = -2;
            break;
        }
    }
    fclose(fp);
    return v;
}

/* 外部探针：另开一个 fd 读文件，返回实际能读到的字节数（并回填内容） */
static long probe_seen_by_other_fd(char *out, size_t cap)
{
    int fd = open(F1, O_RDONLY);
    ssize_t n;

    if (fd == -1)
        return -1;
    memset(out, 0, cap);
    n = read(fd, out, cap - 1);
    close(fd);
    return n < 0 ? -1 : (long) n;
}

/* 外部探针 2：另开一个 fd 看 st_size（不用 close 写端，看的是页缓存的元数据） */
static long probe_size_by_other_fd(void)
{
    struct stat st;
    int fd = open(F1, O_RDONLY);
    long sz;

    if (fd == -1)
        return -1;
    if (fstat(fd, &st) == -1)
        sz = -1;
    else
        sz = (long) st.st_size;
    close(fd);
    return sz;
}

static void report(const char *stage, const char *where)
{
    char seen[64];
    long n = probe_seen_by_other_fd(seen, sizeof(seen));

    printf("  %-34s 别处可见 %2ld 字节  文件大小 %2ld  nr_dirty=%ld\n",
           stage, n, probe_size_by_other_fd(), vmstat_val("nr_dirty"));
    if (n > 0)
        printf("  %-34s   ↳ 内容是 [%s]\n", "", seen);
    printf("  %-34s   ↳ 数据在：%s\n", "", where);
}

int main(void)
{
    const size_t LEN = strlen(PAYLOAD);

    printf("== 用「另一个 fd 能看见多少」来定位数据在哪一层 ==\n");
    printf("  载荷 = \"%s\"（%zu 字节）\n", PAYLOAD, LEN);
    printf("  列的含义：'别处可见' = 另开 O_RDONLY fd 实际读到的字节数；\n");
    printf("            '文件大小' = 另开 fd 的 st_size；'nr_dirty' = 整机脏页数\n\n");

    unlink(F1);
    report("0. 文件还不存在", "——");

    FILE *fp = fopen(F1, "w");
    int fd;
    if (fp == NULL) {
        printf("  fopen 失败 errno=%d(%s)\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }
    fd = fileno(fp);
    printf("  已 fopen(\"%s\", \"w\")，fileno(fp)=%d\n\n", F1, fd);

    fprintf(fp, "%s", PAYLOAD);
    report("1. fprintf 之后（未 fflush）", "**用户态 stdio 缓冲**里");

    fflush(fp);
    report("2. fflush 之后", "**内核页缓存**里（stdio 缓冲已空）");

    errno = 0;
    int fr = fsync(fd);
    printf("\n  fsync(fileno(fp)) = %d errno=%d(%s)\n", fr, errno, strerror(errno));
    report("3. fsync 之后", "页缓存 **和** 磁盘上都有（页缓存不清）");

    fclose(fp);
    report("4. fclose 之后（它会隐式 fflush）", "页缓存 + 磁盘（同上）");

    printf("\n== 这张阶梯说明了什么 ==\n");
    printf("  ┌ 第 0 级 → 1 级：fprintf 只是把字节拷进用户态数组，**外部完全看不见**\n");
    printf("  ├ 第 1 级 → 2 级：fflush 把字节交给内核，**外部立刻可见**（这就是页缓存）\n");
    printf("  ├ 第 2 级 → 3 级：fsync 让数据到达设备；但「外部可见性」**不再变化**\n");
    printf("  │   → 所以「看得见」只能证明「至少到了页缓存」，**证明不了已落盘**：\n");
    printf("  │     要判落盘只能看 nr_dirty / Dirty 这类**内核侧计数器**，或者换台机器验。\n");
    printf("  └ 第 3 级 → 4 级：fclose 会隐式 fflush（标准规定），所以不 fflush 也不 fclose\n");
    printf("     就该怀疑丢数据 —— 那正是「进程被 SIGKILL 掉」时的情形。\n");
    printf("\n  ⚠️ 每一级的 nr_dirty 都可能被别的进程干扰（整机口径），\n");
    printf("     而且 16 字节远小于一页，脏页计数在这么小的量上**看不出变化**。\n");
    printf("     这里诚实地记下来：小 IO 下 nr_dirty 不是有效指标，用 st_size 与可见性判断。\n");

    printf("\n== 两条链路的完整写法 ==\n");
    printf("  写：fprintf -> [stdio buf] -fflush-> [page cache] -fsync-> disk\n");
    printf("  读：read    <- [page cache] <- 设备      （stdio 侧：fgets <- [stdio buf] <- read）\n");
    printf("  ⚠️ 读方向有个容易漏的点：stdio 的读缓冲是**预读**的。\n");
    printf("     你对一个 FILE* 调 fread(buf, 1, 1, fp)，内核那边可能被要求读了\n");
    printf("     一整个 st_blksize（本机 4096）—— 所以「读了多少」在两层里是不同的数。\n");
    printf("     要看这种差异就得用 `read(fileno(fp), ...)` 绕开 stdio，或者用 strace。\n");

    unlink(F1);
    return EXIT_SUCCESS;
}
