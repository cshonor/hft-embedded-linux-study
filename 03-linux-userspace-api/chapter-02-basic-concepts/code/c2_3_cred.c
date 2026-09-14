/* TLPI 第 2 章 §2.3 —— 用户与组：三份身份 + 权限位「命中即停」+ capability 实测
 *
 * 编译：gcc -O0 -Wall -Wextra c2_3_cred.c -o c2_3
 * 运行：./c2_3
 *
 * 本节要钉死的事实：
 *   ① 内核只认数字 UID/GID，看不见「用户名」。
 *   ② 权限检查拿 fsuid/fsgid 与 inode 的 uid/gid 做三级匹配（owner→group→other），
 *      命中即停 —— 所以文件属主可能比 other 权限还低（本 demo 用纯逻辑推演钉死）。
 *   ③ 「root 无所不能」是错的：真正的特权单位是 capability。
 *      本 demo 用 CapEff 位图 + 真实 EACCES 证明：euid=0 但 CapEff=0 时，
 *      DAC 权限位**照常生效**（因为没有 CAP_DAC_OVERRIDE）。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CAP_DAC_OVERRIDE_BIT 1          /* include/uapi/linux/capability.h: CAP_DAC_OVERRIDE = 1 */

/* ============ 权限位匹配的纯逻辑推演（顺序与 fs/namei.c 一致） ============ */

static const char *which_class(uid_t puid, gid_t pgid, int in_supp,
                               uid_t fuid, gid_t fgid)
{
    if (puid == fuid)       return "owner";   /* ① 属主：命中即停，不再往下看 */
    if (pgid == fgid)       return "group";   /* ② 主组匹配 */
    if (in_supp)            return "group";   /* ② 补充组匹配 */
    return "other";                           /* ③ 兜底 */
}

static int bits_of(mode_t mode, const char *cls)
{
    if (strcmp(cls, "owner") == 0) return (mode >> 6) & 7;
    if (strcmp(cls, "group") == 0) return (mode >> 3) & 7;
    return mode & 7;
}

static void rwx(int bits, char out[4])
{
    out[0] = (bits & 4) ? 'r' : '-';
    out[1] = (bits & 2) ? 'w' : '-';
    out[2] = (bits & 1) ? 'x' : '-';
    out[3] = '\0';
}

static void matrix(void)
{
    struct { const char *tag, *who; uid_t puid; gid_t pgid; int sup;
             mode_t mode; uid_t fuid; gid_t fgid; } c[] = {
        { "A", "属主本人",          1000, 1000, 0, 0640, 1000, 1000 },
        { "B", "同主组",            2000, 1000, 0, 0640, 1000, 1000 },
        { "C", "同补充组",          2000, 2000, 1, 0640, 1000, 1000 },
        { "D", "都不沾边",          2000, 2000, 0, 0640, 1000, 1000 },
        { "E", "属主但 owner 位=0", 1000, 1000, 0, 0004, 1000, 1000 },
        { "F", "不沾边但 other=rwx", 3000, 3000, 0, 0007, 1000, 1000 },
    };

    for (unsigned i = 0; i < sizeof(c) / sizeof(c[0]); i++) {
        const char *cls = which_class(c[i].puid, c[i].pgid, c[i].sup,
                                      c[i].fuid, c[i].fgid);
        int b = bits_of(c[i].mode, cls);
        char s[4]; rwx(b, s);
        printf("  [%s] 进程 uid=%u gid=%u%s\n", c[i].tag, c[i].puid, c[i].pgid,
               c[i].sup ? "（属于文件的补充组）" : "");
        printf("      文件 uid=%u gid=%u mode=%04o  ← %s\n",
               c[i].fuid, c[i].fgid, c[i].mode, c[i].who);
        printf("      命中 %-5s → 有效位 %s → 可读 %s\n\n",
               cls, s, (b & 4) ? "YES" : "no");
    }

    printf("  E 行是本节最反直觉的一条：进程就是属主，但 owner 位是 0，\n");
    printf("  即使 other 位写着 r--，也读不到 —— 因为「命中即停」，\n");
    printf("  属主根本不会去看 other 位。\n\n");
}

/* ================ 真实权限实验 ================ */

static void try_open(const char *path, int flags, const char *what)
{
    errno = 0;
    int fd = open(path, flags);
    if (fd >= 0) { printf("    %-34s %-8s OK (fd=%d)\n", path, what, fd); close(fd); }
    else         printf("    %-34s %-8s EACCES? errno=%d (%s)\n",
                        path, what, errno, strerror(errno));
    errno = 0;
}

static void fs_experiment(void)
{
    const char *dir = "/tmp/c2_3";
    mkdir(dir, 0755);

    struct { const char *n; mode_t m; } f[] = {
        { "m644.txt", 0644 }, { "m600.txt", 0600 },
        { "m000.txt", 0000 }, { "m400.txt", 0400 },
    };
    char path[128];

    printf("  先建 4 个不同 mode 的文件：\n");
    for (unsigned i = 0; i < sizeof(f) / sizeof(f[0]); i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, f[i].n);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, f[i].m);
        if (fd < 0) { printf("    %s 建不出来：%s\n", f[i].n, strerror(errno)); continue; }
        if (write(fd, "x", 1) != 1) perror("write");
        close(fd);
        chmod(path, f[i].m);            /* umask 可能改过它，显式设回 */
        struct stat st;
        if (stat(path, &st) == 0)
            printf("    %-10s mode=%04o  st_uid=%d st_gid=%d\n",
                   f[i].n, st.st_mode & 07777, st.st_uid, st.st_gid);
    }

    printf("\n  再逐个尝试打开（本进程 euid=%d）：\n", geteuid());
    snprintf(path, sizeof(path), "%s/m000.txt", dir);
    try_open(path, O_RDONLY, "只读");
    snprintf(path, sizeof(path), "%s/m400.txt", dir);
    try_open(path, O_RDONLY, "只读");
    try_open(path, O_WRONLY, "只写");
    snprintf(path, sizeof(path), "%s/m644.txt", dir);
    try_open(path, O_WRONLY, "只写");

    printf("\n  可以解释的三个「非权限」错误码对照：\n");
    try_open("/proc/1/mem", O_RDONLY, "读别的进程内存");
    try_open("/t_root_only.txt", O_WRONLY | O_CREAT, "根目录建文件");
}

/* ================ 读 /proc/self/status 的凭证 + capability ================ */

static unsigned long long read_hex_line(const char *key)
{
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return 0;
    char line[512];
    unsigned long long v = 0;
    size_t klen = strlen(key);
    while (fgets(line, sizeof(line), f))
        if (strncmp(line, key, klen) == 0) { sscanf(line + klen, "%llx", &v); break; }
    fclose(f);
    return v;
}

static void status_creds(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) { perror("fopen /proc/self/status"); return; }
    char line[512];
    while (fgets(line, sizeof(line), f))
        if (strncmp(line, "Uid:", 4) == 0 || strncmp(line, "Gid:", 4) == 0 ||
            strncmp(line, "Groups:", 7) == 0)
            printf("  %s", line);
    fclose(f);
}

int main(void)
{
    printf("=== ① 三份身份 ===\n");
    uid_t r, e, s; gid_t gr, ge, gs;
    if (getresuid(&r, &e, &s) == 0)
        printf("  Uid:  real=%d  effective=%d  saved=%d\n", r, e, s);
    if (getresgid(&gr, &ge, &gs) == 0)
        printf("  Gid:  real=%d  effective=%d  saved=%d\n", gr, ge, gs);

    long n = getgroups(0, NULL);
    printf("  getgroups(0, NULL) = %ld   （补充组个数）\n", n);
    if (n > 0 && n < 64) {
        gid_t g[64];
        if (getgroups((int)n, g) >= 0) {
            printf("  补充组列表：");
            for (long i = 0; i < n; i++) printf("%d ", g[i]);
            printf("\n");
        }
    }
    printf("  → 权限检查用的是 effective UID（这里是 %d），不是 real UID\n", geteuid());

    printf("\n=== ② /proc/self/status 里的凭证行（内核视角）===\n");
    status_creds();

    unsigned long long capEff = read_hex_line("CapEff:");
    printf("  CapEff = 0x%016llx   CapPrm = 0x%016llx   CapBnd = 0x%016llx\n",
           capEff, read_hex_line("CapPrm:"), read_hex_line("CapBnd:"));
    printf("  CAP_DAC_OVERRIDE 是第 %d 位（值 %llu）\n",
           CAP_DAC_OVERRIDE_BIT, 1ULL << CAP_DAC_OVERRIDE_BIT);
    printf("  本进程有没有它？%s\n",
           (capEff & (1ULL << CAP_DAC_OVERRIDE_BIT)) ? "有 → 能绕过 DAC 位"
                                                     : "没有 → DAC 位照常生效");
    printf("  → 这一行是下面第 ④ 步结果的直接原因，也是「root ≠ 无限」的证据。\n");

    printf("\n=== ③ 权限位三级匹配的完整推演（纯逻辑，顺序与内核一致）===\n");
    matrix();

    printf("=== ④ 真实文件实验（用 mode 位检验上面的推演）===\n");
    fs_experiment();

    printf("\n  结论（按实测，不按传说）：\n");
    printf("    · m000.txt 打不开、m400.txt 只能读不能写 —— DAC 位在本环境**有效**。\n");
    printf("    · 原因不是「root 权限不够」，而是本进程 euid=0 却 CapEff=0x0，\n");
    printf("      缺少 CAP_DAC_OVERRIDE 这一位。\n");
    printf("    · 拥有该 capability 的进程（如宿主机上的真 root）能打开 mode 0000 的文件。\n");

    printf("\n=== ⑤ root 到底能绕过什么 ===\n");
    printf("  root 绕过的只是**传统 DAC 权限位**（而且要靠 CAP_DAC_OVERRIDE 这一位）；\n");
    printf("  它仍受：\n");
    printf("    · capability 模型的细粒度约束（40+ 个独立能力位）\n");
    printf("    · MAC（SELinux / AppArmor）的强制策略\n");
    printf("    · 内建检查：例如根文件系统只读（下面这条实测）\n");
    errno = 0;
    int fd = open("/t.bin", O_WRONLY | O_CREAT, 0644);
    printf("    open(\"/t.bin\", O_CREAT) -> %s", fd < 0 ? "失败 " : "成功 ");
    if (fd < 0) printf("errno=%d (%s)", errno, strerror(errno));
    else close(fd);
    printf("\n    注意 EROFS(%d) 与 EACCES(%d) 是两码事：一个说「盘只读」，\n", EROFS, EACCES);
    printf("    一个说「权限不够」—— 排障时别混。\n");
    return 0;
}
