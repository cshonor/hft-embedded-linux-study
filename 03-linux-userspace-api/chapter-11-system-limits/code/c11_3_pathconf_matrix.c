/* c11_3_pathconf_matrix.c — Ch11 §11.3：pathconf() / fpathconf() 到底查了什么
 *
 * §11.3 的核心不是「会调用这两个函数」，而是搞清哪些 _PC_* 真的会去问文件
 * 系统、哪些其实只是把编译期常量原样返回。本程序用一个矩阵把它分开：
 *
 *   A. 同一组问题，问两个**不同文件系统**上的路径（/ 是 tmpfs，/lib 是 ext4）。
 *      只有 _PC_LINK_MAX / _PC_FILESIZEBITS 变了 —— 它们是按 statfs().f_type
 *      分派的；_PC_NAME_MAX 来自 statvfs().f_namemax；而 _PC_PATH_MAX 与
 *      _PC_PIPE_BUF 无论问谁都是同一个编译期常量。
 *
 *   B. 同一组问题，问四种**不同 fd 类型**。_PC_ASYNC_IO 会分叉（普通文件 1，
 *      管道/字符设备 -1），其余照旧。
 *
 *   C. 错误码矩阵：空串、不存在的路径、路径中间不是目录、非法 name、非法 fd。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c11_3_pathconf_matrix c11_3_pathconf_matrix.c
 * 取材： glibc 2.39 sysdeps/unix/sysv/linux/pathconf.c:45-55（按 statfs 分派四项）
 *        glibc 2.39 sysdeps/unix/sysv/linux/pathconf.c:63-128（distinguish_extX：/sys/dev/block -> /proc/mounts）
 *        glibc 2.39 sysdeps/posix/pathconf.c:32-36（空串 -> ENOENT）
 *        glibc 2.39 sysdeps/posix/pathconf.c:40-42（default -> EINVAL）
 *        glibc 2.39 sysdeps/posix/pathconf.c:65-86（_PC_NAME_MAX -> statvfs.f_namemax）
 *        glibc 2.39 sysdeps/posix/pathconf.c:89-93（_PC_PATH_MAX -> PATH_MAX 常量）
 *        glibc 2.39 sysdeps/posix/pathconf.c:96-100（_PC_PIPE_BUF -> PIPE_BUF 常量）
 *        glibc 2.39 sysdeps/unix/sysv/linux/linux_fsinfo.h:256 EXT4_LINK_MAX=65000
 *        glibc 2.39 sysdeps/unix/sysv/linux/linux_fsinfo.h:268 LINUX_LINK_MAX=127
 *        Linux v6.6 include/uapi/linux/limits.h:13-14 PATH_MAX/PIPE_BUF
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <unistd.h>

struct item { int name; const char *lbl; };

static const struct item PC[] = {
    { _PC_NAME_MAX,  "_PC_NAME_MAX" },
    { _PC_LINK_MAX,  "_PC_LINK_MAX" },
    { _PC_FILESIZEBITS, "_PC_FILESIZEBITS" },
    { _PC_PATH_MAX,  "_PC_PATH_MAX" },
    { _PC_PIPE_BUF,  "_PC_PIPE_BUF" },
    { _PC_NO_TRUNC,  "_PC_NO_TRUNC" },
    { _PC_CHOWN_RESTRICTED, "_PC_CHOWN_RESTRICTED" },
    { _PC_ASYNC_IO,  "_PC_ASYNC_IO" },
};

/* 两列对照打印：值 + errno + 是否不同 */
static void row(const char *what, long a, int ea, long b, int eb)
{
    printf("  %-20s %-8ld (errno %d)   %-8ld (errno %d)   %s\n",
           what, a, ea, b, eb, (a != b) ? "** 不同 **" : "相同");
}

static void matrix_paths(void)
{
    const char *p1 = "/";       /* CE 沙箱里是 tmpfs */
    const char *p2 = "/lib";    /* 绑定挂载的 ext4 */
    FILE *f;
    char dev[128], mnt[128], type[32];

    printf("### A. 同一组问题问两个文件系统\n");
    printf("  %-20s %-19s %-19s %s\n", "", p1, p2, "");
    for (size_t i = 0; i < sizeof(PC) / sizeof(PC[0]); i++) {
        errno = 0; long a = pathconf(p1, PC[i].name); int ea = errno;
        errno = 0; long b = pathconf(p2, PC[i].name); int eb = errno;
        row(PC[i].lbl, a, ea, b, eb);
    }

    printf("\n  两者的身份（读 /proc/mounts + statfs）：\n");
    for (int k = 0; k < 2; k++) {
        const char *p = k ? p2 : p1;
        struct statfs sf;
        if (statfs(p, &sf) != 0) { printf("    statfs(%s) 失败\n", p); continue; }
        struct statvfs sv;
        long nm = statvfs(p, &sv) == 0 ? (long)sv.f_namemax : -1;
        printf("    %-6s f_type=0x%-8lx f_namemax=%-4ld", p, (unsigned long)sf.f_type, nm);
        f = fopen("/proc/mounts", "r");
        if (f) {
            while (fscanf(f, "%127s %127s %31s %*s %*s %*s", dev, mnt, type) == 3)
                if (strcmp(mnt, p) == 0) break;
            fclose(f);
            printf(" 挂载类型=%s\n", type);
        } else {
            printf("\n");
        }
    }
    printf("\n  glibc 的分工（源码）：_PC_LINK_MAX/_PC_FILESIZEBITS 按 statfs().f_type 查表；\n");
    printf("  _PC_NAME_MAX 取 statvfs().f_namemax；_PC_PATH_MAX 直接 return PATH_MAX；\n");
    printf("  _PC_PIPE_BUF 直接 return PIPE_BUF。后两个与文件系统无关。\n");
    printf("  （A 段 _PC_LINK_MAX 问 /lib 那一格带着 errno=2：glibc 区分 ext2/3/4 时要先\n");
    printf("    readlink /sys/dev/block/MAJ:MIN，失败再退到 /proc/mounts；失败那一次把\n");
    printf("    ENOENT 留在了 errno 里。值是对的，errno 脏了。）\n");
}

static void matrix_fds(void)
{
    int pipefd[2];
    if (pipe(pipefd) != 0) { perror("pipe"); return; }
    const char *regpath = "/etc/hostname";
    int reg = open(regpath, O_RDONLY);
    if (reg < 0) { regpath = "/proc/version"; reg = open(regpath, O_RDONLY); }
    int nul = open("/dev/null", O_RDONLY);

    struct { const char *lbl; int fd; } fds[] = {
        { "STDIN_FILENO", STDIN_FILENO },
        { "pipe read end", pipefd[0] },
        { "regular file",  reg },
        { "/dev/null",     nul },
    };

    printf("\n### B. 同一组问题问四种 fd\n");
    printf("  %-20s %-12s %-12s %-12s %-12s\n", "", fds[0].lbl, fds[1].lbl,
           fds[2].lbl, fds[3].lbl);
    for (size_t i = 0; i < sizeof(PC) / sizeof(PC[0]); i++) {
        long v[4]; int e[4];
        printf("  %-20s", PC[i].lbl);
        for (int k = 0; k < 4; k++) {
            errno = 0;
            v[k] = (fds[k].fd < 0) ? -1 : fpathconf(fds[k].fd, PC[i].name);
            e[k] = errno;
            if (v[k] == -1 && e[k] == 0) printf(" %-12s", "-1(indet)");
            else if (v[k] == -1)         printf(" %-12s", "ERR");
            else                          printf(" %-12ld", v[k]);
        }
        printf("\n");
    }
    printf("  说明：_PC_PIPE_BUF 对任何 fd 都返回 4096（编译期常量，不是「这个管道的容量」）；\n");
    printf("        _PC_ASYNC_IO 只在普通文件/块设备上是 1（glibc 对 path 做 S_ISREG||S_ISBLK 判断）。\n");
    printf("        B 段「regular file」这一列实际打开的是 %s。\n", regpath);

    if (reg >= 0) close(reg);
    if (nul >= 0) close(nul);
    close(pipefd[0]);
    close(pipefd[1]);
}

static void err_matrix(void)
{
    printf("\n### C. 错误码矩阵\n");
#define E(call, expect)                                                  \
    do {                                                                 \
        errno = 0;                                                       \
        long r = (call);                                                 \
        printf("  %-46s -> %-4ld errno=%-2d %-22s %s\n", #call, r,       \
               errno, strerror(errno),                                   \
               (errno == (expect)) ? "" : "<- 与预期不符");              \
    } while (0)

    E(pathconf("", _PC_NAME_MAX),            ENOENT);
    E(pathconf("/no/such/dir/xyz", _PC_NAME_MAX), ENOENT);
    E(pathconf("/proc/version/inside", _PC_NAME_MAX), ENOTDIR);
    E(pathconf("/", 9999),                   EINVAL);
    E(fpathconf(9999, _PC_NAME_MAX),         EBADF);
    E(fpathconf(STDIN_FILENO, 9999),         EINVAL);
#undef E
    printf("  三点注意：\n");
    printf("  1) pathconf(\"\") 不是 EINVAL 而是 ENOENT —— glibc 在 sysdeps/posix/pathconf.c:32-36\n");
    printf("     专门为它加了一个前置判断（空串在 open 语义里就等于「没给路径」）。\n");
    printf("  2) 「路径里某一段不存在」和「路径中间那一段不是目录」是两个不同的 errno：\n");
    printf("     第 2 行 ENOENT / 第 3 行 ENOTDIR。写重试逻辑时不能只判 ENOENT。\n");
    printf("  3) 非法的 name 与「本实现不支持把 name 关联到这个文件」共用 EINVAL，\n");
    printf("     和 sysconf 一样：-1 + errno 分不清「问错了」还是「不回答」。\n");
}

int main(void)
{
    matrix_paths();
    matrix_fds();
    err_matrix();

    printf("\n### D. 这三个值在别处也见得到，别混为一谈\n");
    printf("  <limits.h> 的 PATH_MAX   = %d  (= <linux/limits.h> 里的 #define PATH_MAX 4096)\n",
           PATH_MAX);
    printf("  <limits.h> 的 PIPE_BUF   = %d  (决定 write(2) 的原子上界，不是管道容量)\n",
           PIPE_BUF);
    printf("  <limits.h> 的 NAME_MAX   = %d  (内核 uapi 里写死的，而 pathconf 会按 fs 回话)\n",
           NAME_MAX);
    printf("  管道容量是另一回事：本机 pipe() 出来默认 65536，可用 fcntl(F_SETPIPE_SZ) 改。\n");
    return 0;
}
