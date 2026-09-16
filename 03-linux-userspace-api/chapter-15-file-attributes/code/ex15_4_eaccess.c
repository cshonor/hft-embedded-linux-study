/* ex15_4_eaccess.c — 习题 15-4：写一个按「有效 ID」判定权限的 access()
 *
 * 题面：access() 按 real UID/GID 判权限；写一个按 **effective** UID/GID
 * （含补充组）判定的对应函数。
 *
 * 实现 §15.4.3 的判定算法（switch 语义，不叠加）：
 *   euid == st_uid                    → 属主三元组
 *   egid ∈ {st_gid} ∪ 补充组          → 属组三元组
 *   其余                              → 其他三元组
 * 与 access() 不同，本函数**基于 fstat 的静态快照**判定（拿到 fd 的
 * 时刻就定了），没有 access() 的路径 TOCTOU 窗口——这正是它的价值。
 *
 * 编译： cc -Wall -Wextra -o ex15_4_eaccess ex15_4_eaccess.c
 * 取材： TLPI §15.4.3/15.4.4 习题 15-4；man-pages 6.19 access(2) NOTES
 */
#include <fcntl.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* euid/egid 是否落在补充组里（结果缓存，进程内组表不变） */
static int memberOfSuppGroup(gid_t gid)
{
    static gid_t *groups = NULL;
    static int ngroups = -2;
    if (ngroups == -2) {
        ngroups = getgroups(0, NULL);
        groups = malloc((size_t) (ngroups > 0 ? ngroups : 1) * sizeof(gid_t));
        getgroups(ngroups, groups);
    }
    for (int i = 0; i < ngroups; i++)
        if (groups[i] == gid)
            return 1;
    return 0;
}

/* 返回 0=允许，-1=拒绝（errno 不动，调用方自行置 EACCES 亦可）
 * whichClass 输出命中了哪个三元组：0=属主 1=属组 2=其他 */
static int
eaccessFstat(const struct stat *sb, int mode, int *whichClass)
{
    uid_t euid = geteuid();
    gid_t egid = getegid();
    unsigned base;                      /* 该类的 R 基准位（W/X 依次 <<1 <<2） */

    if (euid == sb->st_uid) {           /* 第一优先：属主 */
        base = S_IRUSR;
        if (whichClass) *whichClass = 0;
    } else if (egid == sb->st_gid || memberOfSuppGroup(sb->st_gid)) {
        base = S_IRGRP;                 /* 第二优先：属组（含补充组） */
        if (whichClass) *whichClass = 1;
    } else {
        base = S_IROTH;                 /* 最后：其他 */
        if (whichClass) *whichClass = 2;
    }

    int ok = 1;
    if (mode & R_OK) ok = ok && (sb->st_mode & base);
    if (mode & W_OK) ok = ok && (sb->st_mode & (base << 1));
    if (mode & X_OK) ok = ok && (sb->st_mode & (base << 2));
    return ok ? 0 : -1;
}

int main(int argc, char *argv[])
{
    if (argc != 2 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct stat sb;
    if (stat(argv[1], &sb) == -1) { perror("stat"); return EXIT_FAILURE; }

    printf("文件: %s  mode=%04lo  属主=%ld 属组=%ld\n",
           argv[1], (unsigned long) (sb.st_mode & 07777),
           (long) sb.st_uid, (long) sb.st_gid);
    printf("本进程: euid=%ld egid=%ld（access 按 ruid=%ld 判，本函数按 euid 判）\n\n",
           (long) geteuid(), (long) getegid(), (long) getuid());

    const char *clsName[] = { "属主三元组", "属组三元组", "其他三元组" };
    static const int modes[] = { R_OK, W_OK, X_OK };
    const char *names[] = { "R_OK", "W_OK", "X_OK" };
    for (int i = 0; i < 3; i++) {
        int cls = -1;
        int r = eaccessFstat(&sb, modes[i], &cls);
        printf("  eaccess(%-5s) = %-2s  → 按%s判\n",
               names[i], r == 0 ? "0" : "-1", clsName[cls]);
    }

    /* 与真实 access() 对拍：普通进程里两者应当一致 */
    puts("\n普通进程（ruid==euid）里与 access() 对拍：");
    int acc = access(argv[1], R_OK);
    int mine = eaccessFstat(&sb, R_OK, NULL);
    printf("  access(R_OK)=%d  本函数(R_OK)=%d  %s\n", acc, mine,
           (acc == 0) == (mine == 0) ? "一致" : "不一致(?)");
    puts("  差异只在 SUID/SGID 程序里出现：access 按发起者，open 按有效 ID，");
    puts("  本函数模拟的是 open 的立场（基于 fstat 快照，顺带免疫路径 TOCTOU）。");
    return EXIT_SUCCESS;
}
