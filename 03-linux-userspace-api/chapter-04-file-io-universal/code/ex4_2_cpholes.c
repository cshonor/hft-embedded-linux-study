/* ex4_2_cpholes.c — TLPI 练习 4-2：像 cp 一样拷贝，但保留源文件的「空洞」
 *
 * 【题干（原书 Exercise 4-2，逐字）】
 *   Write a program like cp that, when used to copy a regular file that
 *   contains holes (sequences of null bytes), also creates correspdonding
 *   holes in the target file.
 *
 * 【朴素做法错在哪】
 *   普通 copy 用 `while (read(...) > 0) write(...)`，读到空洞区会拿到一堆
 *   真实的 0 字节，然后老老实实写进目标文件 —— 空洞被「灌实」了，
 *   目标文件物理占用暴涨。源文件 4 KB、目标文件 1 MB 的荒唐结果从此而来。
 *
 * 【正确做法】
 *   用 lseek(fd, off, SEEK_DATA) / SEEK_HOLE 问文件系统「哪里有数据、
 *   哪里有洞」，把有数据的段读出来写过去，洞则在目标文件里用 lseek
 *   跳过去（不写）。目标文件的洞是「跳出来」的，不是「写零写出来」的。
 *
 * 【注意】SEEK_HOLE/SEEK_DATA 需要文件系统支持（ext4/xfs/btrfs 都支持，
 *   某些陈旧 FS 或 overlayfs 可能不支持，会返回 EINVAL）。
 *
 * 编译: gcc -O0 -Wall -o ex4_2_cpholes ex4_2_cpholes.c
 * 用法: ./ex4_2_cpholes src dst
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#define BUF_SIZE 4096

static ssize_t writeAll(int fd, const char *buf, size_t len)
{
    size_t written = 0;
    while (written < len) {
        ssize_t n = write(fd, buf + written, len - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        written += (size_t) n;
    }
    return (ssize_t) written;
}

static void report(const char *tag, const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0) return;
    printf("  %-14s st_size = %8lld B   st_blocks*512 = %8lld B   块数 = %lld\n",
           tag, (long long) st.st_size, (long long) st.st_blocks * 512,
           (long long) st.st_blocks);
}

int main(int argc, char *argv[])
{
    const char *src, *dst;

    if (argc == 3) {
        src = argv[1];
        dst = argv[2];
    } else if (argc == 1) {
        /* 无参数模式：自己造一个带洞的源文件再拷它 —— 便于在无预置数据的
           环境（如 Compiler Explorer 沙箱）里自测。 */
        src = "/tmp/c4_holes_src.bin";
        dst = "/tmp/c4_holes_dst.bin";
        printf("== 0. 先造一个带洞的源文件 ==\n");
        int f = open(src, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (f < 0) { perror("open src"); return 1; }
        write(f, "HEAD-", 5);                        /* 数据段 1 */
        lseek(f, 512 * 1024, SEEK_SET);              /* 洞 1：跳 500 KB */
        write(f, "MIDDLE", 6);                       /* 数据段 2 */
        lseek(f, 2 * 1024 * 1024, SEEK_SET);         /* 洞 2：再跳 1.5 MB */
        write(f, "TAIL!", 5);                        /* 数据段 3 */
        /* 洞 3（**尾部洞**）：只把长度撑到 3 MiB，不写任何数据。
           这一段专门用来把「尾部是洞」这个场景造出来 —— 它同时逼出
           拷贝循环里的 ENXIO 分支和收尾的 ftruncate()。 */
        ftruncate(f, 3 * 1024 * 1024);
        close(f);
        report("造好的源文件", src);
        printf("\n== 开始拷贝 ==\n");
    } else {
        fprintf(stderr, "Usage: %s [src dst]\n", argv[0]);
        return 1;
    }

    int in = open(src, O_RDONLY);
    if (in < 0) { fprintf(stderr, "open %s: %s\n", src, strerror(errno)); return 1; }

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { fprintf(stderr, "open %s: %s\n", dst, strerror(errno)); return 1; }

    struct stat st;
    fstat(in, &st);
    off_t size = st.st_size;
    printf("  源文件 %s: st_size = %lld B, st_blocks*512 = %lld B\n",
           src, (long long) st.st_size, (long long) st.st_blocks * 512);

    off_t pos = 0;
    int holes = 0, data_segs = 0;
    char buf[BUF_SIZE];

    while (pos < size) {
        /* 从 pos 找下一段数据；没有数据就是纯洞，直接收尾 */
        off_t data = lseek(in, pos, SEEK_DATA);
        if (data < 0) {
            if (errno == ENXIO) {            /* 后面全是洞 */
                printf("  从偏移 %lld 起到文件末尾：全是洞，目标文件直接 lseek 跳过\n",
                       (long long) pos);
                holes++;
                break;
            }
            fprintf(stderr, "SEEK_DATA(%lld): %s\n", (long long) pos, strerror(errno));
            return 1;
        }

        if (data > pos) {                    /* [pos, data) 是一段洞 */
            printf("  洞   : 偏移 %8lld .. %8lld  （%lld 字节）\n",
                   (long long) pos, (long long) data, (long long) (data - pos));
            holes++;
            if (lseek(out, data, SEEK_SET) < 0) {   /* 目标文件也跳过这段 */
                fprintf(stderr, "lseek out: %s\n", strerror(errno));
                return 1;
            }
        }

        /* 从 data 找下一个洞 → [data, hole) 是有数据的段 */
        off_t hole = lseek(in, data, SEEK_HOLE);
        if (hole < 0) hole = size;           /* 文件系统不报洞就一路拷到底 */
        printf("  数据 : 偏移 %8lld .. %8lld  （%lld 字节）\n",
               (long long) data, (long long) hole, (long long) (hole - data));
        data_segs++;

        if (lseek(in, data, SEEK_SET) < 0) return 1;
        if (lseek(out, data, SEEK_SET) < 0) return 1;
        off_t remaining = hole - data;
        while (remaining > 0) {
            size_t want = remaining < (off_t) sizeof buf ? (size_t) remaining
                                                         : sizeof buf;
            ssize_t n = read(in, buf, want);
            if (n < 0) { if (errno == EINTR) continue;
                         fprintf(stderr, "read: %s\n", strerror(errno)); return 1; }
            if (n == 0) break;
            if (writeAll(out, buf, (size_t) n) < 0) {
                fprintf(stderr, "write: %s\n", strerror(errno)); return 1;
            }
            remaining -= n;
        }
        pos = hole;
    }

    /* 关键一步：把目标文件「撑」到和源文件一样的逻辑大小。
       本轮源文件以一段尾部洞收尾（循环是靠 ENXIO 退出的），所以最后那个
       数据段之后什么都没有写 —— 不执行这一句，目标文件就比源文件短。
       ftruncate 扩出来的区域不分配磁盘块，所以洞依然保留。 */
    if (ftruncate(out, size) < 0)
        fprintf(stderr, "ftruncate: %s\n", strerror(errno));

    printf("  共 %d 段数据、%d 处洞\n", data_segs, holes);
    close(in);
    close(out);

    printf("\n== 结果对比 ==\n");
    report(src, src);
    report(dst, dst);
    printf("  → 两者 st_size 一样（逻辑内容相同），st_blocks 也接近（洞被保留）。\n");
    printf("    若用朴素 read/write 拷贝，目标 st_blocks 会被灌到跟 st_size 一个量级。\n");
    return 0;
}
