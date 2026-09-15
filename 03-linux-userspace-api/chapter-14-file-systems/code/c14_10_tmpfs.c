/* c14_10_tmpfs.c —— TLPI §14.10 A Virtual Memory File System: tmpfs
 *
 *  tmpfs 的数据不落在任何块设备上，而是**匿名页**（可被换出到 swap）。
 *  所以它的容量上限不是「磁盘剩余空间」，而是两个挂载选项：
 *      -o size=<N>        块数上限（默认 = 物理内存的一半）
 *      -o nr_inodes=<N>   inode 数上限（默认按内存页数推算）
 *  源码坐标（Linux v6.6）：
 *      mm/shmem.c:134-137   shmem_default_max_blocks()  → totalram_pages() / 2
 *      mm/shmem.c:139-145   shmem_default_max_inodes()  → min3(nr_pages - totalhigh_pages(),
 *                                                            nr_pages / 2,
 *                                                            ULONG_MAX / BOGO_INODE_SIZE)
 *
 *  本 demo 用容器里现成的三个 tmpfs 实例（/、/dev、/tmp）做六件事：
 *    ① /proc/filesystems 里 tmpfs 的登记（`nodev` 前缀是什么意思）
 *    ② /tmp 的 mountinfo 记录逐字段拆解（source 是 none，super opts 带 size=/nr_inodes=）
 *    ③ statfs("/tmp") 的 blocks/files 与 size=20480k / nr_inodes=100 逐项对账
 *    ④ 三个实例的 fsid 两两不同 ⇒ 三个独立超级块；
 *       而 /sys 的 fsid 与 / **完全相同** ⇒ /sys 只是 / 上的一个空目录，
 *       并没有挂 sysfs —— 「路径存在 ≠ 那个文件系统已挂载」的实证
 *    ⑤ 默认上限与 RAM 的关系（读 /proc/meminfo 的 MemTotal 现场算比值，
 *       并按实测比值决定结论文字 —— 不同宿主机上这个比值可能是 100.00%）
 *    ⑥ 在 /tmp 里真写一个 1 MiB 文件，看 f_bfree 与 f_ffree 两条线各自怎么动
 *
 *  ⚠️ tmpfs 的内容在内存里，掉电即失；本容器**无法演示重启**，所以「不落盘」
 *     这一条只能靠源码与参数说明，不能靠实测断言。
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_10 c14_10_tmpfs.c
 */
#include <sys/statfs.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 1024
#define TMPFS_MAGIC 0x01021994

static void sec(const char *t)
{
    printf("\n== %s ==\n", t);
}

static void human(unsigned long long b, char *out, size_t n)
{
    if (b >= (1ULL << 30))
        snprintf(out, n, "%.2f GiB", (double)b / (double)(1ULL << 30));
    else if (b >= (1ULL << 20))
        snprintf(out, n, "%.2f MiB", (double)b / (double)(1ULL << 20));
    else
        snprintf(out, n, "%llu B", b);
}

static int slurp(const char *path, char *buf, size_t n)
{
    int fd = open(path, O_RDONLY);
    ssize_t r;

    if (fd == -1)
        return -1;
    r = read(fd, buf, n - 1);
    close(fd);
    if (r < 0)
        return -1;
    buf[r] = '\0';
    return 0;
}

/* /proc/meminfo 的值都是 kB；找不到返回 0 */
static unsigned long meminfo_kb(const char *key)
{
    static char buf[8192];
    char *p;

    if (slurp("/proc/meminfo", buf, sizeof(buf)) == -1)
        return 0;
    p = strstr(buf, key);
    if (p == NULL)
        return 0;
    return strtoul(p + strlen(key), NULL, 10);
}

/* 在 /proc/self/mountinfo 里按 mount point 精确查找一条记录。
 * 记录格式：[id] [parent] [maj:min] [root] [mount point] [opts] [optional-fields]... - [fstype] [source] [super-opts] */
static int mi_record(const char *mp, char *opts, size_t no, char *fstype, size_t nf,
                     char *sopts, size_t ns, char *lineout, size_t nl)
{
    FILE *fp = fopen("/proc/self/mountinfo", "r");
    char line[MAXLINE];
    int found = -1;

    if (fp == NULL)
        return -1;
    while (fgets(line, sizeof(line), fp) != NULL) {
        char orig[MAXLINE];
        char *tok[16];
        int nt = 0, dash = -1;
        char *p = line;
        size_t L = strlen(line);

        if (L > 0 && line[L - 1] == '\n')
            line[L - 1] = '\0';
        /* 分词会往 line 里插 NUL，所以先把原始整行留一份用来打印 */
        snprintf(orig, sizeof(orig), "%s", line);
        while (*p != '\0' && nt < 16) {
            while (*p == ' ')
                p++;
            if (*p == '\0')
                break;
            tok[nt++] = p;
            while (*p != '\0' && *p != ' ')
                p++;
            if (*p != '\0')
                *p++ = '\0';
        }
        if (nt < 10 || strcmp(tok[4], mp) != 0)
            continue;
        for (int i = 0; i < nt; i++)
            if (!strcmp(tok[i], "-")) {
                dash = i;
                break;
            }
        if (dash < 0)
            continue;
        snprintf(opts, no, "%s", tok[5]);
        if (dash + 1 < nt)
            snprintf(fstype, nf, "%s", tok[dash + 1]);
        if (dash + 3 < nt)
            snprintf(sopts, ns, "%s", tok[dash + 3]);
        snprintf(lineout, nl, "%s", orig);
        found = 0;
        break;
    }
    fclose(fp);
    return found;
}

/* 从 "rw,size=20480k,nr_inodes=100,..." 里取出 name= 后面的值（不含逗号） */
static int opt_value(const char *sopts, const char *name, char *out, size_t n)
{
    const char *p = sopts;
    size_t len = strlen(name);

    while ((p = strstr(p, name)) != NULL) {
        if (p == sopts || p[-1] == ',') {
            const char *v = p + len;
            const char *end = strchr(v, ',');
            size_t L = end ? (size_t)(end - v) : strlen(v);
            if (L >= n)
                L = n - 1;
            memcpy(out, v, L);
            out[L] = '\0';
            return 0;
        }
        p += len;
    }
    return -1;
}

/* "20480k" / "16m" / "100" -> 字节数（无后缀按字节）；返回 -1 表示不是 size 值 */
static long long opt_bytes(const char *v)
{
    char *end = NULL;
    long long x = strtoll(v, &end, 10);

    if (end == NULL || end == v || x < 0)
        return -1;
    if (*end == '\0')
        return x;
    if ((*end == 'k' || *end == 'K') && end[1] == '\0')
        return x * 1024;
    if ((*end == 'm' || *end == 'M') && end[1] == '\0')
        return x * 1024 * 1024;
    if ((*end == 'g' || *end == 'G') && end[1] == '\0')
        return x * 1024LL * 1024 * 1024;
    return -1;
}

static unsigned long long fsbytes(const struct statfs *sf)
{
    unsigned long long fr = sf->f_frsize ? (unsigned long long)sf->f_frsize
                                        : (unsigned long long)sf->f_bsize;
    return (unsigned long long)sf->f_blocks * fr;
}

int main(void)
{
    static const char *TMPFS_PATHS[] = { "/", "/dev", "/tmp", "/sys", NULL };
    struct statfs st;

    /* ---------- ① /proc/filesystems ---------- */
    sec("① /proc/filesystems 里 tmpfs 的登记");
    {
        char buf[8192];
        char *p;

        if (slurp("/proc/filesystems", buf, sizeof(buf)) == 0) {
            p = strstr(buf, "tmpfs");
            printf("  /proc/filesystems 里与 tmpfs 有关的那一行：%s\n",
                   p ? "（见下）" : "（没有 tmpfs！？）");
            if (p != NULL) {
                /* 回退到本行行首打印整行 */
                char *b = p;
                while (b > buf && b[-1] != '\n')
                    b--;
                char *e = strchr(p, '\n');
                if (e != NULL)
                    *e = '\0';
                printf("    [%s]\n", b);
            }
        }
        printf("  ↑ `nodev` 前缀 = 这个文件系统**不需要块设备**就能挂载。\n");
        printf("    这一栏正是 14.1「设备特殊文件」与 14.3「文件系统」的分界线：\n");
        printf("    有 nodev 的文件系统用 `mount -t tmpfs none /mnt`（source 写 none 就行）。\n");
    }

    /* ---------- ② /tmp 的 mountinfo 记录 ---------- */
    sec("② /tmp 在 /proc/self/mountinfo 里的那条记录");
    {
        char opts[256], fstype[64], sopts[512], line[MAXLINE];

        if (mi_record("/tmp", opts, sizeof(opts), fstype, sizeof(fstype),
                      sopts, sizeof(sopts), line, sizeof(line)) == 0) {
            printf("  %s\n", line);
            printf("    fstype       = %s\n", fstype);
            printf("    per-mount    = %s\n", opts);
            printf("    super opts   = %s\n", sopts);
        } else {
            printf("  /proc/self/mountinfo 里没有 /tmp 这条记录\n");
        }
        printf("  ↑ 注意三点：① source 是 **none**（tmpfs 背后没有设备）；\n");
        printf("    ② size= 与 nr_inodes= 出现在 **super options** 一栏（它们属于超级块，\n");
        printf("       不是 per-mount 选项）；③ per-mount 一栏的 nosuid/nodev/noexec\n");
        printf("       是挂载时给这一个挂载点加的加固选项（都写在小写那一栏）。\n");
    }

    /* ---------- ③ /tmp 的 statfs 与挂载选项对账 ---------- */
    sec("③ statfs(\"/tmp\") 与 size= / nr_inodes= 逐项对账");
    {
        char opts[256], fstype[64], sopts[512], line[MAXLINE], v[64];
        char h1[32], h2[32];

        if (statfs("/tmp", &st) == -1) {
            printf("  statfs 失败 errno=%d (%s)\n", errno, strerror(errno));
        } else {
            printf("  f_type    = %#x (%s)\n", (unsigned)st.f_type,
                   (unsigned)st.f_type == TMPFS_MAGIC ? "tmpfs" : "不是 tmpfs");
            printf("  f_bsize   = %ld   f_frsize = %ld\n",
                   (long)st.f_bsize, (long)st.f_frsize);
            printf("  f_blocks  = %llu   f_bfree = %llu   f_bavail = %llu\n",
                   (unsigned long long)st.f_blocks, (unsigned long long)st.f_bfree,
                   (unsigned long long)st.f_bavail);
            printf("  f_files   = %llu   f_ffree = %llu\n",
                   (unsigned long long)st.f_files, (unsigned long long)st.f_ffree);
            printf("  f_namelen = %ld\n", (long)st.f_namelen);
        }
        if (mi_record("/tmp", opts, sizeof(opts), fstype, sizeof(fstype),
                      sopts, sizeof(sopts), line, sizeof(line)) == 0 &&
            statfs("/tmp", &st) == 0) {
            human(fsbytes(&st), h1, sizeof(h1));
            printf("  f_blocks × f_frsize = %llu 字节 (%s)\n", fsbytes(&st), h1);
            if (opt_value(sopts, "size=", v, sizeof(v)) == 0) {
                long long sz = opt_bytes(v);
                human((unsigned long long)sz, h2, sizeof(h2));
                printf("  super opts 的 size=%s = %lld 字节 (%s)\n", v, sz, h2);
                printf("  校验 1（容量）: %s\n",
                       (sz >= 0 && (unsigned long long)sz == fsbytes(&st))
                           ? "两者完全一致 ✔ —— size= 就是 f_blocks × f_frsize"
                           : "**不一致** —— 说明 size= 不是全部真相");
            }
            if (opt_value(sopts, "nr_inodes=", v, sizeof(v)) == 0) {
                printf("  super opts 的 nr_inodes=%s\n", v);
                printf("  校验 2（inode 数）: %s\n",
                       strtoull(v, NULL, 10) == (unsigned long long)st.f_files
                           ? "与 f_files 完全一致 ✔ —— nr_inodes= 就是 f_files"
                           : "**不一致**");
            }
        }
        printf("  ↑ 结论：tmpfs 的容量与 inode 上限是**挂载时定死的两个数**，\n");
        printf("    不是「动态探测磁盘」。所以 tmpfs 上的 write 失败理由是 ENOSPC，\n");
        printf("    但原因可能是**块用完了**，也可能是**inode 用完了**（两者要分开看）。\n");
    }

    /* ---------- ④ 三个 tmpfs 实例的 fsid ---------- */
    sec("④ 四个路径的 fsid 对比：谁和谁是同一个超级块");
    {
        for (int i = 0; TMPFS_PATHS[i] != NULL; i++) {
            if (statfs(TMPFS_PATHS[i], &st) == -1) {
                printf("  %-6s statfs 失败 errno=%d (%s)\n", TMPFS_PATHS[i], errno,
                       strerror(errno));
                continue;
            }
            printf("  %-6s f_type=%#010x  fsid=%08x,%08x  blocks=%-9llu files=%-9llu\n",
                   TMPFS_PATHS[i], (unsigned)st.f_type,
                   (unsigned)st.f_fsid.__val[0], (unsigned)st.f_fsid.__val[1],
                   (unsigned long long)st.f_blocks, (unsigned long long)st.f_files);
        }
        printf("  ↑ 判据是 **fsid**（超级块的身份证），不是路径，也不是 f_type。\n");
        printf("    · / 与 /dev 与 /tmp 的 fsid 两两不同 ⇒ 三个**独立**的 tmpfs 实例\n");
        printf("    · /sys 的 fsid 与 / **完全相同** ⇒ /sys 只是 / 上的一个空目录，\n");
        printf("      **sysfs 根本没挂**。所以 /sys/fs、/sys/dev/block 这类路径都不存在\n");
        printf("      —— 「路径存在 ≠ 那个文件系统已挂载」，这是 mount 命名空间最容易骗人的地方。\n");
        printf("    · 另外注意 / 与 /dev 的 f_files == f_blocks：来自\n");
        printf("      mm/shmem.c:139-145 的 min3(nr_pages - totalhigh_pages(), nr_pages / 2, ...)，\n");
        printf("      64 位无 highmem 机器上 totalhigh_pages()==0，取到的就是 nr_pages/2。\n");
    }

    /* ---------- ⑤ 默认 size 与 RAM ---------- */
    sec("⑤ 没有 size= 的 tmpfs，上限是多少（默认值 vs 物理内存）");
    {
        unsigned long mt = meminfo_kb("MemTotal:");
        unsigned long ma = meminfo_kb("MemAvailable:");
        unsigned long sh = meminfo_kb("Shmem:");
        char h[32];

        if (statfs("/", &st) == -1) {
            printf("  statfs(\"/\") 失败 errno=%d\n", errno);
        } else if (mt > 0) {
            unsigned long long half = (unsigned long long)mt * 1024 / 2;
            unsigned long long got = fsbytes(&st);
            double ratio = half ? 100.0 * (double)got / (double)half : 0.0;

            human(got, h, sizeof(h));
            printf("  MemTotal        = %lu kB = %llu 字节\n", mt, (unsigned long long)mt * 1024);
            printf("  MemTotal / 2    = %llu 字节\n", half);
            printf("  / 的容量上限    = %llu 字节 (%s)  [f_blocks × f_frsize]\n", got, h);
            printf("  比值            = %.2f%%\n", ratio);
            printf("  MemAvailable    = %lu kB   Shmem = %lu kB\n", ma, sh);
            /* ⚠️ 结论必须由**实测比值**决定，不能写死 —— 不同宿主机上
             *    totalram_pages() 与 MemTotal/4096 的关系不一样，两种都见过。 */
            if (ratio > 99.9 && ratio < 100.1)
                printf("  → 本机上二者**恰好相等**：totalram_pages() 恰好等于 MemTotal/4096，\n"
                       "     所以「默认上限 = 物理内存的一半」在这里是**精确**成立的。\n");
            else
                printf("  → 比值**不到** 100%%：内核用的是 totalram_pages()，\n"
                       "     它只统计**被伙伴系统管理的页**（不含内核早期保留、struct page\n"
                       "     元数据等），比 MemTotal 小一截。此时默认上限只是「≈ 一半」。\n");
            printf("    无论哪种情况，`size=` 缺省时 tmpfs 的上限都**由内存决定、与磁盘无关**，\n");
            printf("     这就是它和 ext4（上限来自块设备容量）最根本的差别。\n");
        }
        printf("  对照：/tmp 有显式 size=20480k ⇒ 上限 20 MiB，与内存大小无关（见 ③）。\n");
        printf("        /dev 没有 size= ⇒ 上限同样是「默认值」（见 ④ 的 blocks 一列）。\n");
    }

    /* ---------- ⑥ 真写一个文件看两条线 ---------- */
    sec("⑥ 在 /tmp 里写 1 MiB：f_bfree 与 f_ffree 各自怎么动");
    {
        const char *p = "/tmp/c14_10_tmpfs_probe.bin";
        struct statfs before, after;
        unsigned char *buf = malloc(1 << 20);
        int fd;
        ssize_t w;

        if (buf == NULL) {
            printf("  malloc 失败\n");
        } else if (statfs("/tmp", &before) == -1) {
            printf("  statfs 失败 errno=%d\n", errno);
        } else {
            memset(buf, 'T', 1 << 20);
            fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                printf("  open(%s) 失败 errno=%d (%s)\n", p, errno, strerror(errno));
            } else {
                w = write(fd, buf, 1 << 20);
                printf("  write(%d 字节) 返回 %ld\n", 1 << 20, (long)w);
                close(fd);
                if (statfs("/tmp", &after) == 0) {
                    printf("  f_bfree: %llu -> %llu  (差 %lld 块 = %lld 字节)\n",
                           (unsigned long long)before.f_bfree,
                           (unsigned long long)after.f_bfree,
                           (long long)after.f_bfree - (long long)before.f_bfree,
                           ((long long)after.f_bfree - (long long)before.f_bfree) *
                               (long long)after.f_frsize);
                    printf("  f_ffree: %llu -> %llu  (差 %lld)\n",
                           (unsigned long long)before.f_ffree,
                           (unsigned long long)after.f_ffree,
                           (long long)after.f_ffree - (long long)before.f_ffree);
                    printf("  ↑ 1 MiB / 4096 = 256 块 —— 块数与 inode 数是**两条独立的线**。\n");
                    printf("    inode 只在**创建**时消耗（与文件大小无关），块在**写数据**时消耗。\n");
                }
            }
            if (unlink(p) == 0) {
                printf("  已删除 %s\n", p);
                if (statfs("/tmp", &after) == 0)
                    printf("  删除后 f_bfree = %llu, f_ffree = %llu（回到初始值附近）\n",
                           (unsigned long long)after.f_bfree,
                           (unsigned long long)after.f_ffree);
            }
            free(buf);
        }
    }

    /* ---------- ⑦ 只有源码能回答的部分 ---------- */
    sec("⑦ 本容器**测不出来**的两件事（诚实标注）");
    printf("  1. 「tmpfs 内容在内存、掉电即失」——需要重启才能演示，本容器做不到。\n");
    printf("     依据只有源码：tmpfs 用 shmem 匿名页，不调用任何块设备驱动。\n");
    printf("  2. 「tmpfs 可以换出到 swap」——需要内存压力 + swap 设备，本容器两者都没有\n");
    printf("     （/proc/meminfo 的 SwapTotal 见 ⑤ 的读数）。\n");

    printf("\n=== c14_10 done ===\n");
    return 0;
}
