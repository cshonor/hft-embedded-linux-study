/* ex14_1_file_churn.c —— TLPI §14.13 Exercise 14-1（本章只有**一道**习题）
 *
 *  【任务】（官方 §14.13 Exercise 是单数，只此一题；题干为转引，见笔记 14.13）
 *    编写一个程序，测量在单个目录中创建并随后删除大量 1 字节文件所需的时间。
 *      · 文件名形如 xNNNNNN，其中 NNNNNN 是一个随机的六位数字；
 *      · 文件按**名称生成时的随机顺序**创建，然后按**数字递增顺序**删除
 *        （即删除顺序与创建顺序不同）；
 *      · 文件数量 NF 与目标目录都能在命令行指定。
 *    题面还问了四件事（本程序的输出直接回答前两件，后两件用 -s 开关回答）：
 *      ① 随着 NF 增大，在各文件系统上观察到什么模式？
 *      ② 不同文件系统之间比较如何？
 *      ③ 如果按数字递增顺序创建、**并且按同一顺序删除**，结果会不会变？
 *      ④ 会变的话，原因是什么？是否随文件系统类型而不同？
 *
 *  【为什么这道题有信息量】
 *    它测的不是「写 1 个字节有多快」，而是**目录项（dentry）与 inode 的分配/回收开销**：
 *      · 创建 = 新 inode + 新目录项，目录还会因插入位置不同而反复分裂/重排
 *      · 随机顺序创建 ⇒ 名字在目录里到处插，局部性差
 *      · 递增顺序创建 ⇒ 名字总落在目录末尾，局部性好
 *    所以「创建顺序」是本题的核心变量，而不是噪声。
 *    （TLPI §14.2/14.3 的目录布局与 §14.4 的 inode，正是解释这个差异的地方。）
 *
 *  【用法】
 *    ex14_1_file_churn [-s] <dir> [NF]
 *      -s        按数字递增顺序创建（默认是随机顺序）
 *      <dir>     测试目录（程序只创建/删除自己造的 xNNNNNN，不动别的文件）
 *      [NF]      文件数量，默认 2000（题面建议 1000 ~ 20000，本仓默认取小值）
 *
 *  ⚠️ 两个环境事实（不是 bug）：
 *    1. 本容器 /tmp 的 tmpfs 带 `nr_inodes=100`（见 /proc/self/mountinfo），
 *       所以在 /tmp 上跑 NF=200 会看到 ENOSPC(28) —— 那是 **inode 用完**（§14.10），
 *       不是磁盘满。
 *    2. `open(path, O_CREAT, ...)` 的第三个参数 mode **必须给**；不给的话新文件的
 *       权限位就是栈上的垃圾值（原书读者踩过的坑）。
 *
 *  编译：gcc -O0 -Wall -Wextra -o ex14_1 ex14_1_file_churn.c
 */
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define NAMELEN 8              /* "x" + 6 位数字 + '\0' */
#define MAXNF   1000000        /* xNNNNNN 只能表示 1000000 个名字 */
#define DEF_NF  2000
#define MAXTRY  40             /* 同一个槽位最多重掷多少次随机数 */

static void sec(const char *t)
{
    printf("\n== %s ==\n", t);
}

static double now_sec(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0.0;
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int cmp_name(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

static const char *fstype_name(unsigned long t)
{
    switch (t) {
    case 0xef53:        return "ext2/ext3/ext4";
    case 0x01021994:    return "tmpfs";
    case 0x9fa0:        return "proc";
    case 0x6969:        return "nfs";
    case 0x01021997:    return "hugetlbfs";
    case 0x73717368:    return "squashfs";
    case 0x794c7630:    return "overlayfs";
    case 0x01021995:    return "mqueue";
    default:            return "(未收录)";
    }
}

/* 手写六位补零，不用 snprintf —— 免得格式串与缓冲区长度之间出现截断隐患 */
static void make_name(char *dst, long num)
{
    dst[0] = 'x';
    for (int k = 6; k >= 1; k--) {
        dst[k] = (char)('0' + (int)(num % 10));
        num /= 10;
    }
    dst[7] = '\0';
}

static void print_names(const char (*names)[NAMELEN], long n, long k)
{
    for (long i = 0; i < n && i < k; i++)
        printf("%s%s", names[i], i + 1 < n && i + 1 < k ? " " : "");
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [-s] <dir> [NF]\n"
                    "  -s       create in ascending numeric order too\n"
                    "  <dir>    directory to churn in (only xNNNNNN files are touched)\n"
                    "  [NF]     number of files, default %d\n", prog, DEF_NF);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int same_order = 0, opt;
    const char *dir;
    char path[4096];
    long nf = DEF_NF;
    char (*names)[NAMELEN];
    struct statfs sb_before, sb_after, sb_now;
    struct stat sb;
    long created = 0, deleted = 0, collisions = 0;
    int create_errno = 0, delete_errno = 0;
    double t0, dt_create = 0.0, dt_delete = 0.0;

    while ((opt = getopt(argc, argv, "sn:")) != -1) {
        switch (opt) {
        case 's':
            same_order = 1;
            break;
        case 'n':
            nf = strtol(optarg, NULL, 10);
            break;
        default:
            usage(argv[0]);
        }
    }
    if (optind >= argc)
        usage(argv[0]);
    dir = argv[optind];
    if (optind + 1 < argc)
        nf = strtol(argv[optind + 1], NULL, 10);
    if (nf < 1 || nf > MAXNF) {
        fprintf(stderr, "%s: NF 必须在 1..%d 之间（现在 %ld）\n", argv[0], MAXNF, nf);
        exit(EXIT_FAILURE);
    }

    /* ---------- ① 前置检查 ---------- */
    sec("① 目标目录与它所在的文件系统");
    if (stat(dir, &sb) == -1) {
        fprintf(stderr, "  stat(%s): %s\n", dir, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("  dir           = %s\n", dir);
    printf("  S_ISDIR       = %d  (mode=%#o, st_dev=%u:%u, st_ino=%llu)\n",
           S_ISDIR(sb.st_mode), (unsigned)sb.st_mode,
           (unsigned)major(sb.st_dev), (unsigned)minor(sb.st_dev),
           (unsigned long long)sb.st_ino);
    if (!S_ISDIR(sb.st_mode)) {
        fprintf(stderr, "  %s: 不是目录\n", dir);
        exit(EXIT_FAILURE);
    }
    if (statfs(dir, &sb_before) == 0)
        printf("  statfs        = f_type=%#x (%s)  f_bsize=%ld  f_blocks=%llu  "
               "f_files=%llu\n",
               (unsigned)sb_before.f_type, fstype_name((unsigned long)sb_before.f_type),
               (long)sb_before.f_bsize, (unsigned long long)sb_before.f_blocks,
               (unsigned long long)sb_before.f_files);

    /* ---------- ② 生成名字 ---------- */
    names = malloc((size_t)nf * NAMELEN);
    if (names == NULL) {
        fprintf(stderr, "  malloc(%ld) 失败\n", nf);
        exit(EXIT_FAILURE);
    }
    srand(1);                  /* 固定种子 ⇒ 同一份日志可复现 */
    for (long i = 0; i < nf; i++)
        make_name(names[i], same_order ? i : (long)(rand() % MAXNF));

    sec("② 创建阶段（1 字节文件）");
    printf("  NF=%ld  创建顺序=%s\n", nf, same_order ? "递增（-s）" : "随机（默认）");
    printf("  创建顺序（前 6 个）: ");
    print_names(names, nf, 6);
    printf("\n");

    t0 = now_sec();
    for (long i = 0; i < nf; i++) {
        int fd = -1;

        for (int try_ = 0; try_ < MAXTRY; try_++) {
            snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
            /* 第三个参数 mode 必须给：O_CREAT 时缺了它，权限位就是栈上的垃圾值 */
            fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
            if (fd >= 0)
                break;
            if (errno != EEXIST) {
                create_errno = errno;
                break;
            }
            collisions++;
            if (same_order) {     /* 递增模式下重名说明名字空间用尽，不该发生 */
                create_errno = EEXIST;
                break;
            }
            make_name(names[i], (long)(rand() % MAXNF));
        }
        if (fd < 0)
            break;
        if (write(fd, "x", 1) != 1) {   /* 每个文件正好 1 字节 */
            create_errno = errno;
            close(fd);
            break;
        }
        close(fd);
        created++;
    }
    dt_create = now_sec() - t0;

    printf("  轮询重名次数（EEXIST）= %ld\n", collisions);
    if (create_errno != 0) {
        printf("  创建提前中止：errno=%d (%s)   已创建 %ld 个\n",
               create_errno, strerror(create_errno), created);
        printf("  ↑ 在 /tmp（tmpfs，nr_inodes=100）上跑 NF>100 会走到这里；\n");
        printf("    那是 **inode 用完**（§14.10 的 nr_inodes=），不是块用完。\n");
    } else {
        printf("  创建完成：%ld 个\n", created);
    }
    if (created > 0) {
        snprintf(path, sizeof(path), "%s/%s", dir, names[0]);
        printf("  第一个文件的 stat：");
        if (stat(path, &sb) == 0) {
            printf("size=%lld blocks=%lld nlink=%llu mode=%#o\n",
                   (long long)sb.st_size, (long long)sb.st_blocks,
                   (unsigned long long)sb.st_nlink, (unsigned)sb.st_mode);
            printf("  ↑ st_blocks 的单位是 **512 字节**（不是 f_bsize），所以 1 字节文件\n");
            printf("    看到的是 8 而不是 1；若文件系统把微小文件内联进 inode，也可能是 0。\n");
            printf("    mode=%#o 是 `open` 第三个参数生效的证据——\n", (unsigned)sb.st_mode);
            printf("    不给 mode 时这里打出来的是栈上的垃圾值。\n");
        } else {
            printf("stat 失败 errno=%d\n", errno);
        }
    }
    if (statfs(dir, &sb_after) == -1)
        sb_after = sb_before;
    printf("  创建后 f_ffree=%llu  f_bfree=%llu\n",
           (unsigned long long)sb_after.f_ffree,
           (unsigned long long)sb_after.f_bfree);

    /* ---------- ③ 删除阶段 ---------- */
    sec("③ 删除阶段：按**数字递增**顺序");
    qsort(names, (size_t)created, NAMELEN, cmp_name);
    printf("  删除顺序（前 6 个）: ");
    print_names(names, created, 6);
    printf("\n");
    printf("  两种顺序相同吗？%s\n", same_order ? "相同（-s）" : "不同 —— 这正是本题的对照");

    t0 = now_sec();
    for (long i = 0; i < created; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        if (unlink(path) == 0)
            deleted++;
        else if (delete_errno == 0)
            delete_errno = errno;
    }
    dt_delete = now_sec() - t0;

    /* ---------- ④ 结果 ---------- */
    sec("④ 结果");
    printf("  NF=%ld  created=%ld  deleted=%ld  collisions=%ld\n",
           nf, created, deleted, collisions);
    printf("  创建耗时 = %.6f s", dt_create);
    if (created > 0)
        printf("   (%.2f us/个)", dt_create / (double)created * 1e6);
    printf("\n");
    printf("  删除耗时 = %.6f s", dt_delete);
    if (deleted > 0)
        printf("   (%.2f us/个)", dt_delete / (double)deleted * 1e6);
    printf("\n");
    if (delete_errno)
        printf("  删除有失败：errno=%d (%s)\n", delete_errno, strerror(delete_errno));

    sec("⑤ inode 与块的账（创建前 / 创建后 / 删除后三次 statfs）");
    if (statfs(dir, &sb_now) == 0) {
        printf("  创建前 f_ffree=%llu  f_bfree=%llu\n",
               (unsigned long long)sb_before.f_ffree,
               (unsigned long long)sb_before.f_bfree);
        printf("  创建后 f_ffree=%llu  f_bfree=%llu   (差 %lld / %lld)\n",
               (unsigned long long)sb_after.f_ffree,
               (unsigned long long)sb_after.f_bfree,
               (long long)sb_after.f_ffree - (long long)sb_before.f_ffree,
               (long long)sb_after.f_bfree - (long long)sb_before.f_bfree);
        printf("  删除后 f_ffree=%llu  f_bfree=%llu   (差 %lld / %lld)\n",
               (unsigned long long)sb_now.f_ffree,
               (unsigned long long)sb_now.f_bfree,
               (long long)sb_now.f_ffree - (long long)sb_before.f_ffree,
               (long long)sb_now.f_bfree - (long long)sb_before.f_bfree);
    }
    printf("  ↑ inode 一列的变化应当恰好等于 created（每个文件占一个 inode）；\n");
    printf("    块一列的变化约等于 created（每个 1 字节文件占一个块）。\n");
    printf("    两条线**分开看**才能判断「是 inode 先耗尽还是块先耗尽」。\n");

    sec("⑥ 怎么读这个结果（对应题面的四个问题）");
    printf("  Q1 随 NF 增大的模式：本程序不加缓存，创建/删除都是 O(NF) 次系统调用，\n");
    printf("     所以总耗时会随 NF 近似线性增长；真正要盯的是**每个文件的平均耗时**\n");
    printf("     那一列 —— 目录变大以后它会上翘。\n");
    printf("  Q2 文件系统之间：换个目录参数重跑同一组数字即可（/app 是 ext4、/tmp 是\n");
    printf("     tmpfs），但先看清楚目标目录的容量与 inode 上限。\n");
    printf("  Q3/Q4 顺序的影响：加 -s 再跑一次，与默认的随机序创建对比。\n");
    printf("     机制上，递增顺序让名字总是**追加**到目录末尾，随机顺序则到处插。\n");
    printf("     差距大小与文件系统的目录实现有关（ext4 用 htree，tmpfs 用简单目录）。\n");
    printf("     ⚠️ 本程序**不断言方向**，也不给「应该快多少倍」的结论：\n");
    printf("        请把两组实测数字读出来自己比。\n");

    free(names);
    printf("\n=== ex14_1 done ===\n");
    return 0;
}
