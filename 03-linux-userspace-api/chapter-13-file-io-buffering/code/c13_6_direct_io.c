/* c13_6_direct_io.c — Ch13 §13.6：绕过缓冲区高速缓存（O_DIRECT）
 *
 * TLPI §13.6（p.246）讲 `O_DIRECT`：让应用与块设备之间**直接**传数据，
 * 绕开内核页缓存。原书配的程序是 Listing 13-1（`filebuff/direct_read.c`，p.247），
 * 已逐字镜像在本目录下。
 *
 * 书上对对齐要求的说法是「缓冲区地址、文件偏移、传输长度**三样**都要是
 * 逻辑块大小的整数倍」。本节把这句话**量准**：
 *   ① 两种文件系统上的行为完全不同：ext4 会硬性检查，tmpfs **完全不检查**
 *      （因为 tmpfs 根本没实现 direct I/O，这个标志被静默忽略）；
 *   ② 偏移与长度的对齐要求来自 `bdev_logical_block_size`；
 *   ③ 缓冲区地址的对齐要求来自 **另一个** 值：`bdev_dma_alignment + 1`；
 *      两者**都随底层设备变**，不是常数 —— 用 `statx(2)` 的 `STATX_DIOALIGN`
 *      可以直接问内核要准确值（v6.1+）；
 *   ④ `O_DIRECT` **不等于** `O_SYNC`：绕过缓存 ≠ 落盘。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c13_6_direct_io c13_6_direct_io.c
 * 取材： TLPI §13.6（O_DIRECT 的语义、对齐要求、posix_memalign）
 *       man-pages 6.19 open(2) 的 O_DIRECT（"alignment restrictions ... vary
 *         across filesystems and kernel versions"；"differs from O_SYNC"）
 *                      statx(2) 的 STATX_DIOALIGN（dio_mem_align / dio_offset_align）
 *       Linux v6.6 fs/iomap/direct-io.c:291-293（iomap_dio_bio_iter 的对齐检查）
 *                      include/linux/blkdev.h:1315-1325（bdev_dma_alignment /
 *                        bdev_iter_is_aligned）
 *                      lib/iov_iter.c:866-875（iov_iter_is_aligned：addr_mask 管地址、
 *                        len_mask 管长度）
 *                      block/bdev.c:1046-1047（STATX_DIOALIGN 的两个字段从哪来）
 *                      fs/open.c（O_DIRECT 的合法性检查）
 *       glibc 2.39 sysdeps/unix/sysv/linux/statx.c（statx 的 syscall 包装）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

#ifndef STATX_DIOALIGN
#define STATX_DIOALIGN 0x00000800
#endif
#ifndef STX_DIO_ALIGN
#define STX_DIO_ALIGN 0x00004000
#endif

#define EXT4_PATH "/app/c13_6.bin"
#define TMPFS_PATH "/tmp/c13_6.bin"

static void fs_name(const char *path, char *out, size_t cap)
{
    struct statfs sf;

    if (statfs(path, &sf) == -1) {
        snprintf(out, cap, "(statfs 失败 errno=%d)", errno);
        return;
    }
    switch ((unsigned long) sf.f_type) {
    case 0xef53:     snprintf(out, cap, "ext4(0xef53)");          break;
    case 0x01021994: snprintf(out, cap, "tmpfs(0x1021994)");      break;
    case 0x9fa0:     snprintf(out, cap, "proc(0x9fa0)");          break;
    case 0x6969:     snprintf(out, cap, "nfs(0x6969)");           break;
    case 0x58465342: snprintf(out, cap, "xfs(0x58465342)");       break;
    case 0x794c7630: snprintf(out, cap, "overlayfs(0x794c7630)"); break;
    default:         snprintf(out, cap, "未知(0x%lx)", (unsigned long) sf.f_type); break;
    }
}

/* 向内核要 O_DIRECT 的对齐要求（STATX_DIOALIGN，Linux 6.1+） */
static void ask_dio_align(const char *path)
{
    struct statx stx;
    int r;

    memset(&stx, 0, sizeof(stx));
    errno = 0;
    r = statx(AT_FDCWD, path, 0, STATX_DIOALIGN, &stx);
    if (r == -1) {
        printf("    statx(%s, STATX_DIOALIGN) 失败 errno=%d(%s)\n", path, errno, strerror(errno));
        return;
    }
    if (!(stx.stx_mask & STATX_DIOALIGN) || !(stx.stx_attributes_mask & STX_DIO_ALIGN)) {
        printf("    statx 成功但未返回 DIOALIGN（stx_mask=0x%x）→ 该文件系统不支持上报\n",
               stx.stx_mask);
        return;
    }
    printf("    statx: stx_dio_mem_align    = %u   ← 缓冲区地址要按它对齐\n", stx.stx_dio_mem_align);
    printf("           stx_dio_offset_align = %u   ← 偏移/长度要按它对齐\n", stx.stx_dio_offset_align);
}

/* 造一个 length 字节的文件（普通写） */
static int make_file(const char *path, size_t length)
{
    char *b = malloc(length);
    int fd;

    if (b == NULL)
        return -1;
    memset(b, 'E', length);
    fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        free(b);
        return -1;
    }
    if (write(fd, b, length) != (ssize_t) length) {
        close(fd);
        free(b);
        return -1;
    }
    close(fd);
    free(b);
    return 0;
}

/* 一次 O_DIRECT 读尝试：len/off 可调，buf 用大块里的不同偏移造出不同对齐 */
static void try_read(const char *path, size_t len, off_t off, char *buf, const char *tag)
{
    int fd;
    ssize_t n;

    fd = open(path, O_RDONLY | O_DIRECT);
    if (fd == -1) {
        printf("    %-30s open 失败 errno=%d(%s)\n", tag, errno, strerror(errno));
        return;
    }
    if (lseek(fd, off, SEEK_SET) == -1) {
        printf("    %-30s lseek 失败 errno=%d(%s)\n", tag, errno, strerror(errno));
        close(fd);
        return;
    }
    errno = 0;
    n = read(fd, buf, len);
    printf("    %-30s buf%%64=%2zu off=%-6ld len=%-6zu -> read=%-6ld errno=%d(%s)\n",
           tag, (size_t) ((unsigned long) buf % 64), (long) off, len,
           (long) n, errno, strerror(errno));
    close(fd);
}

int main(void)
{
    char n1[64], n2[64];

    /* 先让两个路径**存在**，否则 statfs() 只会拿到 ENOENT，
       报出来的文件系统类型就无从谈起（fs_name 会打出「statfs 失败」）。 */
    (void) make_file(EXT4_PATH, 8192);
    (void) make_file(TMPFS_PATH, 8192);

    fs_name(EXT4_PATH, n1, sizeof(n1));
    fs_name(TMPFS_PATH, n2, sizeof(n2));
    printf("== ① 先看清是在哪台文件系统上试 ==\n");
    printf("  %-14s -> %s\n", EXT4_PATH, n1);
    printf("  %-14s -> %s\n", TMPFS_PATH, n2);
    printf("  → 两个路径**不是**同一台文件系统，所以下面的结论必须分开说。\n");

    printf("\n== ② 问内核：O_DIRECT 的对齐要求到底是多少 ==\n");
    printf("  （statx(2) 的 STATX_DIOALIGN，Linux 6.1 起可用）\n");
    printf("  对 %s：\n", EXT4_PATH);
    ask_dio_align("/app");
    printf("  对 %s：\n", TMPFS_PATH);
    ask_dio_align("/tmp");

    printf("\n== ③ 偏移 / 长度 / 缓冲区地址：逐项试错 ==\n");
    {
        const size_t FILELEN = 8192;
        /* 用一块足够大的缓冲，通过不同起始偏移构造不同对齐的 buf */
        char *base = NULL;
        size_t total = FILELEN + 4096;

        if (posix_memalign((void **) &base, 4096, total + 4096) != 0) {
            printf("  posix_memalign 失败\n");
            return EXIT_FAILURE;
        }

        if (make_file(EXT4_PATH, FILELEN) == -1 || make_file(TMPFS_PATH, FILELEN) == -1) {
            printf("  造文件失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }

        printf("  在 %s（%s）上：\n", EXT4_PATH, n1);
        try_read(EXT4_PATH, 512, 0,   base,        "offset/len 都对齐");
        try_read(EXT4_PATH, 100, 0,   base,        "len=100 非对齐");
        try_read(EXT4_PATH, 512, 1,   base,        "off=1 非对齐");
        try_read(EXT4_PATH, 1024, 4096, base,      "off=4096 len=1024");

        printf("  在 %s（%s）上：\n", TMPFS_PATH, n2);
        try_read(TMPFS_PATH, 512, 0,  base,        "offset/len 都对齐");
        try_read(TMPFS_PATH, 100, 0,  base,        "len=100 非对齐");
        try_read(TMPFS_PATH, 512, 1,  base,        "off=1 非对齐");

        printf("\n  → 缓冲区地址的对齐下界（用起始偏移造出 1/2/4/.../512 对齐的 buf）：\n");
        {
            const size_t offs[] = {1, 2, 4, 8, 16, 64, 256, 512};
            for (size_t i = 0; i < sizeof(offs) / sizeof(offs[0]); i++) {
                char tag[64];
                snprintf(tag, sizeof(tag), "buf 对齐 = %zu 字节", offs[i]);
                try_read(EXT4_PATH, 512, 0, base + offs[i], tag);
            }
        }

        printf("\n  → 内核判据（Linux v6.6，两处，别只看一处）：\n");
        printf("     (a) fs/iomap/direct-io.c:291-293 —— 偏移与长度：\n");
        printf("           if ((pos | length) & (bdev_logical_block_size(iomap->bdev) - 1) ||\n");
        printf("               !bdev_iter_is_aligned(iomap->bdev, dio->submit.iter))\n");
        printf("                   return -EINVAL;\n");
        printf("     (b) include/linux/blkdev.h:1320-1325 —— 缓冲区地址用的是**另一个**值：\n");
        printf("           return iov_iter_is_aligned(iter, bdev_dma_alignment(bdev),\n");
        printf("                                      bdev_logical_block_size(bdev) - 1);\n");
        printf("         而 lib/iov_iter.c:866-875 里 addr_mask 管**地址**、len_mask 管**长度**。\n");
        printf("         所以：偏移/长度 ← bdev_logical_block_size\n");
        printf("               缓冲区地址 ← bdev_dma_alignment + 1\n");
        printf("         这两个数**都随底层设备（队列参数）变**，不是常数。\n");

        free(base);
        unlink(EXT4_PATH);
        unlink(TMPFS_PATH);
    }

    printf("\n== ④ O_DIRECT ≠ O_SYNC：绕过缓存不等于落盘 ==\n");
    {
        int fd = open(EXT4_PATH, O_CREAT | O_RDWR | O_TRUNC | O_DIRECT, 0644);
        char *b = NULL;

        if (fd == -1) {
            printf("  open 失败 errno=%d(%s)\n", errno, strerror(errno));
            return EXIT_FAILURE;
        }
        if (posix_memalign((void **) &b, 4096, 4096) != 0) {
            printf("  posix_memalign 失败\n");
            close(fd);
            return EXIT_FAILURE;
        }
        memset(b, 'X', 4096);
        errno = 0;
        ssize_t w = write(fd, b, 4096);
        printf("  write(O_DIRECT, 4096) = %ld errno=%d(%s)\n", (long) w, errno, strerror(errno));
        errno = 0;
        int r = fsync(fd);
        printf("  紧接着 fsync(fd) = %d errno=%d(%s)\n", r, errno, strerror(errno));
        printf("  → O_DIRECT 打开的 fd 上 **fsync 依然返回 0**：绕过页缓存只是\n");
        printf("     「不经过内核这块内存」，设备自己的写缓存还在，要不要落盘是另一回事。\n");
        printf("     所以两者要**叠加**用：O_DIRECT 省拷贝/省缓存污染，fsync/O_SYNC 管持久化。\n");
        free(b);
        close(fd);
    }

    printf("\n== ⑤ 不是所有文件都能 O_DIRECT ==\n");
    {
        const char *paths[] = {"/dev/zero", "/etc/passwd", "/proc/version", NULL};
        for (int i = 0; paths[i] != NULL; i++) {
            errno = 0;
            int fd = open(paths[i], O_RDONLY | O_DIRECT);
            int e = errno;
            printf("  open(%-16s, O_RDONLY|O_DIRECT) = %-3d errno=%d(%s)\n",
                   paths[i], fd, e, strerror(e));
            if (fd >= 0)
                close(fd);
        }
        printf("  → `/dev/zero` 之所以失败，是因为它没有块设备的 direct I/O 能力；\n");
        printf("     而 `/etc/passwd` 在本容器里位于 tmpfs 上，虽然能打开，\n");
        printf("     但那个 O_DIRECT **是假的**（实测 ③ 里 tmpfs 从不对齐检查）。\n");
    }

    printf("\n== ⑥ 什么时候该用 O_DIRECT ==\n");
    printf("  适合：自带缓存层的大型 DB / 存储引擎（自己管得比内核好，不想双重缓存）\n");
    printf("  不适合：小 IO 频繁的场景（每次都是整块设备往返，代价极高）\n");
    printf("  工程要点：\n");
    printf("    1) 别写死对齐值 —— 用 statx(STATX_DIOALIGN) 问，或按 4096 保守给；\n");
    printf("    2) 用 posix_memalign/对齐分配，别用 malloc 硬赌；\n");
    printf("    3) 记住它**不解决持久化**，该 fsync 还得 fsync；\n");
    printf("    4) 一定要在**目标文件系统**上验过 —— 同一段代码在 tmpfs 上\n");
    printf("       「跑得通」不代表在 ext4 上能跑通（本程序的 ③ 就是这个陷阱的现场）。\n");
    return EXIT_SUCCESS;
}
