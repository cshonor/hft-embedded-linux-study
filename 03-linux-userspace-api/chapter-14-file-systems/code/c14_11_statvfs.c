/* c14_11_statvfs.c —— TLPI §14.11 Obtaining Information About a File System: statvfs()
 *
 *  ⭐ 本节最容易搞错的一件事：**statvfs() 不是系统调用**，它是 glibc 对 statfs() 的包装。
 *     源码坐标（glibc 2.39）：
 *       sysdeps/unix/sysv/linux/statvfs64.c:29-44
 *         __statvfs64() { struct statfs64 fsbuf;
 *                         if (__statfs64(file, &fsbuf) < 0) return -1;
 *                         __internal_statvfs64(buf, &fsbuf);   <-- 纯字段搬运
 *                         return 0; }
 *         并且文件末尾有：
 *           #if STATFS_IS_STATFS64
 *           weak_alias (__statvfs64, __statvfs)
 *           weak_alias (__statvfs64, statvfs)    <-- x86-64 上 statvfs 就是它的别名
 *           #endif
 *       sysdeps/unix/sysv/linux/internal_statvfs.c:73-112（*64 版）里只有 4 处**变换**，
 *       其余都是逐字段赋值。那 4 处就是本 demo 要逐条验的东西：
 *         f_frsize  = fsbuf.f_frsize ?: fsbuf.f_bsize   (36 行 / 79 行)
 *         f_fsid    = (val[0] & 0xffffffff) | (val[1] << 32)   (90-94 行)
 *         f_namemax = fsbuf.f_namelen                   (102 行)
 *         f_favail  = buf->f_ffree                      (109 行，上一行注释自认 "I have no idea"）
 *         f_flag    = fsbuf.f_flags ^ ST_VALID          (111 行，ST_VALID = 0x0020 见 26 行)
 *
 *  本 demo 分七段：
 *    ① 两个结构体的**尺寸与偏移**（为什么一个是 120、一个是 112）
 *    ② 同一条路径上 statfs() 与 statvfs() 并排打印，逐字段对照
 *    ③ f_flag 逐位分解，并与 /proc/self/mountinfo 的 per-mount options **对账**
 *    ④ 实证 f_flag == f_flags ^ 0x20（ST_VALID 被异或掉）
 *    ⑤ 实证 f_favail == f_ffree（glibc 照抄，不是内核给的）
 *    ⑥ f_frsize ?: f_bsize 这一支在本机**不可达**（两个 FS 都给了非 0 的 f_frsize）——诚实标注
 *    ⑦ f_fsid 是两个 32 位半字的**重组**
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_11 c14_11_statvfs.c
 */
#define _GNU_SOURCE
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1024

/* ST_VALID 是 glibc 的**内部**位值，任何公开头文件里都没有它。
 * 这里按 internal_statvfs.c:26 的 `# define ST_VALID 0x0020` 逐字取用，
 * 并加前缀避免与将来可能出现的公开宏撞名。 */
#define C14_ST_VALID 0x0020

struct fl {
    unsigned long bit;
    const char *name;
};

static const struct fl FLAGS[] = {
    { ST_RDONLY, "ST_RDONLY" },
    { ST_NOSUID, "ST_NOSUID" },
#ifdef ST_NODEV
    { ST_NODEV, "ST_NODEV" },
#endif
#ifdef ST_NOEXEC
    { ST_NOEXEC, "ST_NOEXEC" },
#endif
#ifdef ST_SYNCHRONOUS
    { ST_SYNCHRONOUS, "ST_SYNCHRONOUS" },
#endif
#ifdef ST_MANDLOCK
    { ST_MANDLOCK, "ST_MANDLOCK" },
#endif
#ifdef ST_WRITE
    { ST_WRITE, "ST_WRITE" },
#endif
#ifdef ST_APPEND
    { ST_APPEND, "ST_APPEND" },
#endif
#ifdef ST_IMMUTABLE
    { ST_IMMUTABLE, "ST_IMMUTABLE" },
#endif
#ifdef ST_NOATIME
    { ST_NOATIME, "ST_NOATIME" },
#endif
#ifdef ST_NODIRATIME
    { ST_NODIRATIME, "ST_NODIRATIME" },
#endif
#ifdef ST_RELATIME
    { ST_RELATIME, "ST_RELATIME" },
#endif
#ifdef ST_NOSYMFOLLOW
    { ST_NOSYMFOLLOW, "ST_NOSYMFOLLOW" },
#endif
};

static void sec(const char *t)
{
    printf("\n== %s ==\n", t);
}

/* 把 f_flag 拆成 "ST_RELATIME|ST_NOEXEC|..."；未知位另报 */
static void flags_str(unsigned long f, char *out, size_t n)
{
    size_t used = 0;

    out[0] = '\0';
    for (size_t i = 0; i < sizeof(FLAGS) / sizeof(FLAGS[0]); i++) {
        if ((f & FLAGS[i].bit) == 0)
            continue;
        if (used > 0 && used + 1 < n)
            out[used++] = '|';
        size_t L = strlen(FLAGS[i].name);
        if (used + L >= n)
            L = n - used - 1;
        memcpy(out + used, FLAGS[i].name, L);
        used += L;
        out[used] = '\0';
    }
    if (used == 0)
        snprintf(out, n, "(无)");
}

/* 取 /proc/self/mountinfo 里某挂载点的 per-mount options（第 6 个字段） */
static int mi_opts(const char *mp, char *out, size_t n)
{
    FILE *fp = fopen("/proc/self/mountinfo", "r");
    char line[MAXLINE];
    int found = -1;

    if (fp == NULL)
        return -1;
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *tok[16];
        int nt = 0;
        char *p = line;
        size_t L = strlen(line);

        if (L > 0 && line[L - 1] == '\n')
            line[L - 1] = '\0';
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
        if (nt < 6 || strcmp(tok[4], mp) != 0)
            continue;
        snprintf(out, n, "%s", tok[5]);
        found = 0;
        break;
    }
    fclose(fp);
    return found;
}

int main(void)
{
    static const char *PATHS[] = { "/", "/app", "/tmp", NULL };

    /* ---------- ① 尺寸与偏移 ---------- */
    sec("① 两个结构体的尺寸与偏移（x86-64 / glibc 2.39）");
    printf("  sizeof(struct statfs)   = %zu\n", sizeof(struct statfs));
    printf("  sizeof(struct statfs64) = %zu   （x86-64 上与 statfs 同布局）\n",
           sizeof(struct statfs64));
    printf("  sizeof(struct statvfs)  = %zu\n", sizeof(struct statvfs));
    printf("  sizeof(struct statvfs64)= %zu   （x86-64 上与 statvfs 同布局）\n",
           sizeof(struct statvfs64));
    printf("  offsetof(statfs , f_type) = %zu   offsetof(statfs , f_flags) = %zu   offsetof(statfs , f_spare) = %zu\n",
           offsetof(struct statfs, f_type), offsetof(struct statfs, f_flags),
           offsetof(struct statfs, f_spare));
    printf("  offsetof(statvfs, f_bsize)= %zu   offsetof(statvfs, f_flag)  = %zu   offsetof(statvfs, f_type)  = %zu\n",
           offsetof(struct statvfs, f_bsize), offsetof(struct statvfs, f_flag),
           offsetof(struct statvfs, f_type));
    printf("  ↑ 两个结构体都是「11 个 8 字节字段 = 88 字节」打头，后面各自留尾巴：\n");
    printf("    statfs  = 88 + f_spare[4]×8 = 88 + 32 = 120\n");
    printf("    statvfs = 88 + f_type(4) + __f_spare[5]×4 = 88 + 4 + 20 = 112\n");
    printf("    statvfs 的尾巴里那个 f_type 是 **glibc 扩展**（POSIX 没有），\n");
    printf("    而 statfs 的 f_flags 在 statvfs 里叫 f_flag（少一个 s）—— 两个名字都要记。\n");

    /* ---------- ② 并排 ---------- */
    sec("② 同一条路径上 statfs() 与 statvfs() 并排，逐字段对账");
    for (int i = 0; PATHS[i] != NULL; i++) {
        struct statfs sf;
        struct statvfs sv;
        int r1, r2;

        errno = 0;
        r1 = statfs(PATHS[i], &sf);
        int e1 = errno;
        errno = 0;
        r2 = statvfs(PATHS[i], &sv);
        int e2 = errno;

        printf("\n  [%s]\n", PATHS[i]);
        if (r1 == -1 || r2 == -1) {
            printf("    statfs=%d(errno=%d) statvfs=%d(errno=%d)\n", r1, e1, r2, e2);
            continue;
        }
        printf("    statfs : f_type=%#x  f_bsize=%ld  f_frsize=%ld  f_namelen=%ld\n",
               (unsigned)sf.f_type, (long)sf.f_bsize, (long)sf.f_frsize,
               (long)sf.f_namelen);
        printf("             f_blocks=%llu  f_bfree=%llu  f_bavail=%llu\n",
               (unsigned long long)sf.f_blocks, (unsigned long long)sf.f_bfree,
               (unsigned long long)sf.f_bavail);
        printf("             f_files=%llu  f_ffree=%llu  f_fsid=%08x,%08x  f_flags=%#lx\n",
               (unsigned long long)sf.f_files, (unsigned long long)sf.f_ffree,
               (unsigned)sf.f_fsid.__val[0], (unsigned)sf.f_fsid.__val[1],
               (unsigned long)sf.f_flags);
        printf("    statvfs:                 f_bsize=%lu  f_frsize=%lu  f_namemax=%lu\n",
               sv.f_bsize, sv.f_frsize, sv.f_namemax);
        printf("             f_blocks=%llu  f_bfree=%llu  f_bavail=%llu\n",
               (unsigned long long)sv.f_blocks, (unsigned long long)sv.f_bfree,
               (unsigned long long)sv.f_bavail);
        printf("             f_files=%llu  f_ffree=%llu  f_favail=%llu\n",
               (unsigned long long)sv.f_files, (unsigned long long)sv.f_ffree,
               (unsigned long long)sv.f_favail);
        printf("             f_fsid=%#lx  f_flag=%#lx  f_type=%#x\n",
               sv.f_fsid, sv.f_flag, (unsigned)sv.f_type);
        printf("    逐条对账：\n");
        printf("      f_bsize  == statfs.f_bsize            : %s\n",
               (unsigned long)sv.f_bsize == (unsigned long)sf.f_bsize ? "是" : "否");
        printf("      f_blocks == statfs.f_blocks           : %s\n",
               (unsigned long long)sv.f_blocks == (unsigned long long)sf.f_blocks ? "是" : "否");
        printf("      f_ffree  == statfs.f_ffree            : %s\n",
               (unsigned long long)sv.f_ffree == (unsigned long long)sf.f_ffree ? "是" : "否");
        printf("      f_namemax== statfs.f_namelen          : %s\n",
               (unsigned long)sv.f_namemax == (unsigned long)sf.f_namelen ? "是" : "否");
        printf("      f_type   == statfs.f_type             : %s\n",
               (unsigned)sv.f_type == (unsigned)sf.f_type ? "是" : "否");
        printf("      f_favail == statvfs.f_ffree（不是 statfs！）: %s\n",
               (unsigned long long)sv.f_favail == (unsigned long long)sv.f_ffree ? "是" : "否");
        printf("      f_frsize == statfs.f_frsize           : %s\n",
               (unsigned long)sv.f_frsize == (unsigned long)sf.f_frsize ? "是" : "否");
    }

    /* ---------- ③ f_flag 逐位分解 + 与 mountinfo 对账 ---------- */
    sec("③ f_flag 逐位分解，并与 mountinfo 的 per-mount options 对账");
    for (int i = 0; PATHS[i] != NULL; i++) {
        struct statvfs sv;
        struct statfs sf;
        char dec[256], opts[256];

        if (statvfs(PATHS[i], &sv) == -1 || statfs(PATHS[i], &sf) == -1)
            continue;
        flags_str((unsigned long)sv.f_flag, dec, sizeof(dec));
        if (mi_opts(PATHS[i], opts, sizeof(opts)) == -1)
            snprintf(opts, sizeof(opts), "(mountinfo 里没有精确匹配的挂载点)");
        printf("\n  [%s]\n", PATHS[i]);
        printf("    statvfs.f_flag                = %#lx\n", sv.f_flag);
        printf("    拆成 ST_* 名字                = %s\n", dec);
        printf("    mountinfo 的 per-mount options= %s\n", opts);
        printf("    ↑ 两者是**同一件事的两种写法**：ro ↔ ST_RDONLY 的**出现**，\n");
        printf("      rw ↔ ST_RDONLY 的**缺席**；nosuid/nodev/noexec/relatime 一一对应。\n");
        printf("      注意 mountinfo 里**没有**对应 ST_VALID 的选项 —— 它是 glibc 造出来的。\n");
    }

    /* ---------- ④ f_flag == f_flags ^ ST_VALID ---------- */
    sec("④ 实证 f_flag == statfs.f_flags ^ 0x20");
    for (int i = 0; PATHS[i] != NULL; i++) {
        struct statvfs sv;
        struct statfs sf;

        if (statvfs(PATHS[i], &sv) == -1 || statfs(PATHS[i], &sf) == -1)
            continue;
        printf("  %-6s statfs.f_flags=%#06lx   statvfs.f_flag=%#06lx   异或=%#06lx   "
               "f_flags ^ 0x20 == f_flag ? %s\n",
               PATHS[i], (unsigned long)sf.f_flags, (unsigned long)sv.f_flag,
               (unsigned long)(sf.f_flags ^ sv.f_flag),
               ((unsigned long)sf.f_flags ^ C14_ST_VALID) == (unsigned long)sv.f_flag
                   ? "是 ✔" : "否");
    }
    printf("  ↑ 内核在 f_flags 里放了 ST_VALID(0x0020) 这一位（来自 vfs_statfs() 里的\n");
    printf("    calculate_f_flags()，见 fs/statfs.c），意思是「本内核支持 f_flags」。\n");
    printf("    glibc 用**异或**把它去掉，于是 f_flag 里只剩下真正的挂载标志位。\n");
    printf("    所以 statvfs 的 f_flag 永远**看不到 0x20 这一位** —— 不是内核没给，是被抹了。\n");

    /* ---------- ⑤ f_favail ---------- */
    sec("⑤ f_favail 是什么：glibc 直接照抄 f_ffree");
    for (int i = 0; PATHS[i] != NULL; i++) {
        struct statvfs sv;

        if (statvfs(PATHS[i], &sv) == -1)
            continue;
        printf("  %-6s f_ffree=%-10llu f_favail=%-10llu 相等?%s\n",
               PATHS[i], (unsigned long long)sv.f_ffree, (unsigned long long)sv.f_favail,
               sv.f_favail == sv.f_ffree ? "是 ✔" : "否");
    }
    printf("  ↑ 内核的 statfs 结构里**根本没有 f_favail 这个字段**；\n");
    printf("    glibc internal_statvfs.c:109 那两行自己写着：\n");
    printf("      /* XXX I have no idea how to compute f_favail.  Any idea???  */\n");
    printf("      buf->f_favail = buf->f_ffree;\n");
    printf("    所以「非特权用户可用 inode 数」在 Linux 上是**假的** —— 它等于特权用户可用数。\n");
    printf("    （对比：f_bavail 才是真的，它来自内核 statfs 的 f_bavail；\n");
    printf("      ext4 上默认保留 5%% 给 root，所以 f_bavail < f_bfree 是正常现象。）\n");

    /* ---------- ⑥ f_frsize ?: f_bsize ---------- */
    sec("⑥ f_frsize ?: f_bsize —— 这一支在本机**不可达**（诚实标注）");
    for (int i = 0; PATHS[i] != NULL; i++) {
        struct statfs sf;

        if (statfs(PATHS[i], &sf) == -1)
            continue;
        printf("  %-6s statfs.f_frsize=%-8ld statfs.f_bsize=%-8ld → 走的是 %s\n",
               PATHS[i], (long)sf.f_frsize, (long)sf.f_bsize,
               sf.f_frsize != 0 ? "「取 f_frsize」那一支" : "「回退到 f_bsize」那一支");
    }
    printf("  ↑ internal_statvfs.c 的 `buf->f_frsize = fsbuf->f_frsize ?: fsbuf->f_bsize;`\n");
    printf("    是为「老内核不填 f_frsize」准备的补丁。本机内核 v6.6 一律填值，\n");
    printf("    所以**回退那一支在本容器里观察不到** —— 只能说代码在那里，不能说它被走到。\n");

    /* ---------- ⑦ f_fsid 重组 ---------- */
    sec("⑦ f_fsid 是两个 32 位半字的重组");
    for (int i = 0; PATHS[i] != NULL; i++) {
        struct statfs sf;
        struct statvfs sv;
        unsigned long long recomb;

        if (statfs(PATHS[i], &sf) == -1 || statvfs(PATHS[i], &sv) == -1)
            continue;
        recomb = ((unsigned long long)(unsigned)sf.f_fsid.__val[0])
               | ((unsigned long long)(unsigned)sf.f_fsid.__val[1] << 32);
        printf("  %-6s statfs=(%08x,%08x) → 拼起来=%#018llx   statvfs.f_fsid=%#018lx  %s\n",
               PATHS[i], (unsigned)sf.f_fsid.__val[0], (unsigned)sf.f_fsid.__val[1],
               recomb, sv.f_fsid, recomb == sv.f_fsid ? "一致 ✔" : "不一致");
    }
    printf("  ↑ 所以 `statvfs.f_fsid` 与 `statfs.f_fsid.__val[]` 是**同一个数**，\n");
    printf("    只是一个按 64 位整数看、一个按两个 32 位半字看。\n");

    /* ---------- ⑧ 结论 ---------- */
    sec("⑧ 结论");
    printf("  · statvfs() 全流程 = 一次 statfs**系统调用** + 一大段用户态字段搬运。\n");
    printf("    没有第二个系统调用，也没有第二次进内核。\n");
    printf("  · 所以 statvfs 能给你的信息**不会多于** statfs；反而少了 f_type 的宽度\n");
    printf("    （statvfs 是 unsigned int，statfs 是 __fsword_t=long）。\n");
    printf("  · 工程上：只用 Linux 就调 statfs()（字段更全、少一层包装）；\n");
    printf("    要可移植才用 statvfs()（POSIX 标准接口，其他 Unix 也有）。\n");
    printf("  · 反过来说：看到 f_flag 少一位、f_favail 与 f_ffree 相等，\n");
    printf("    别怀疑内核 —— 那是 glibc 干的。\n");

    printf("\n=== c14_11 done ===\n");
    return 0;
}
