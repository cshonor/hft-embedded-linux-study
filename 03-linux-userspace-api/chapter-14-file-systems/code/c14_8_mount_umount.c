/* c14_8_mount_umount.c —— TLPI §14.8 Mounting and Unmounting File Systems
 *
 *  §14.8.1 mount() / §14.8.2 umount() 与 umount2()。
 *
 *  这一节最值得实测的不是「怎么调成功」，而是**失败时的 errno 顺序**：
 *  内核在真正尝试挂载之前会先做路径查找、再做权限检查，
 *  所以「fstype 是假的」这种参数错，在无特权容器里**根本轮不到被检查**。
 *
 *  本 demo 逐条试错并报告 errno：
 *    ① 先证明自己为什么没权限：/proc/self/status 的 CapEff
 *    ② mount() 的六种调用（好参数 / 坏 fstype / 坏 target / bind / propagation）
 *    ③ umount2() 的四个 flag
 *    ④ 本章用到的 MS_* 与 MNT_* 常量（逐字取自本机头文件）
 *    ⑤ 「MS_* 是 ABI 层、MNT_* 是内核内部层」——两套位值完全不同
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_8 c14_8_mount_umount.c
 */
#define _GNU_SOURCE
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void sec(const char *t)
{
    printf("\n== %s ==\n", t);
}

static void try_mount(const char *desc, const char *src, const char *tgt,
                      const char *fstype, unsigned long flags)
{
    int r;

    errno = 0;
    r = mount(src, tgt, fstype, flags, NULL);
    if (r == -1)
        printf("  %-34s -> -1  errno=%2d (%s)\n", desc, errno, strerror(errno));
    else
        printf("  %-34s ->  0  **成功**\n", desc);
}

static void try_umount2(const char *desc, const char *tgt, int flags)
{
    int r;

    errno = 0;
    r = umount2(tgt, flags);
    if (r == -1)
        printf("  %-34s -> -1  errno=%2d (%s)\n", desc, errno, strerror(errno));
    else
        printf("  %-34s ->  0  **成功**\n", desc);
}

int main(void)
{
    /* ---------- ① 能力 ---------- */
    sec("① 为什么必然失败：CapEff 里没有 CAP_SYS_ADMIN");
    {
        int fd = open("/proc/self/status", O_RDONLY);
        char buf[8192];
        ssize_t n = fd == -1 ? -1 : read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            for (char *l = strtok(buf, "\n"); l != NULL; l = strtok(NULL, "\n"))
                if (!strncmp(l, "CapEff", 6) || !strncmp(l, "CapPrm", 6) ||
                    !strncmp(l, "NoNewPrivs", 10))
                    printf("  %s\n", l);
        }
        if (fd != -1)
            close(fd);
    }
    printf("  CAP_SYS_ADMIN 是 bit 21（掩码 0x200000）。CapEff 全 0 ⇒ mount/umount 一律 EPERM。\n");

    /* ---------- ② mount(2) ---------- */
    sec("② mount(2) 的六种调用");
    try_mount("-t tmpfs none /app", "none", "/app", "tmpfs", 0);
    try_mount("(NULL,NULL) MS_REC|MS_PRIVATE /", NULL, "/", NULL, MS_REC | MS_PRIVATE);
    try_mount("-t no_such_fs_type", "none", "/app", "no_such_fs_type", 0);
    try_mount("MS_BIND /tmp -> /app", "/tmp", "/app", NULL, MS_BIND);
    try_mount("-t tmpfs none /no/such/dir", "none", "/no/such/dir", "tmpfs", 0);
    try_mount("-t tmpfs none /proc/version", "none", "/proc/version", "tmpfs", 0);
    printf("  ↑ 注意顺序：**target 不存在** 时先报 ENOENT(2)，\n");
    printf("    target 存在但**参数合法**时才是 EPERM(1)；\n");
    printf("    「fstype 是假的」也报 EPERM —— 说明权限检查排在 fstype 解析之前。\n");

    /* ---------- ③ umount2(2) ---------- */
    sec("③ umount2(2) 的四个 flag");
    try_umount2("umount2(\"/\", 0)", "/", 0);
    try_umount2("umount2(\"/\", MNT_FORCE)", "/", MNT_FORCE);
    try_umount2("umount2(\"/\", MNT_DETACH)", "/", MNT_DETACH);
    try_umount2("umount2(\"/\", MNT_EXPIRE)", "/", MNT_EXPIRE);
    try_umount2("umount2(\"/no/such/dir\", 0)", "/no/such/dir", 0);
    try_umount2("umount2(\"/app\", 0)", "/app", 0);
    printf("  ↑ MNT_FORCE / MNT_DETACH / MNT_EXPIRE 都先撞同一道权限门，\n");
    printf("    所以无法在本容器里比较它们的语义差别（只能按 man 2 umount2 讲）。\n");

    /* ---------- ④ 常量 ---------- */
    sec("④ 本章用到的常量（逐字取自本机头文件）");
    printf("  MS_RDONLY      = %#-10lx MS_NOSUID     = %#lx\n",
           (unsigned long) MS_RDONLY, (unsigned long) MS_NOSUID);
    printf("  MS_NODEV       = %#-10lx MS_NOEXEC    = %#lx\n",
           (unsigned long) MS_NODEV, (unsigned long) MS_NOEXEC);
    printf("  MS_SYNCHRONOUS = %#-10lx MS_REMOUNT   = %#lx\n",
           (unsigned long) MS_SYNCHRONOUS, (unsigned long) MS_REMOUNT);
    printf("  MS_BIND        = %#-10lx MS_MOVE      = %#lx\n",
           (unsigned long) MS_BIND, (unsigned long) MS_MOVE);
    printf("  MS_REC         = %#-10lx MS_PRIVATE   = %#lx\n",
           (unsigned long) MS_REC, (unsigned long) MS_PRIVATE);
    printf("  MS_SLAVE       = %#-10lx MS_SHARED    = %#lx\n",
           (unsigned long) MS_SLAVE, (unsigned long) MS_SHARED);
    printf("  MS_RELATIME    = %#-10lx MS_LAZYTIME  = %#lx\n",
           (unsigned long) MS_RELATIME, (unsigned long) MS_LAZYTIME);
    printf("  MNT_FORCE=%d  MNT_DETACH=%d  MNT_EXPIRE=%d  UMOUNT_NOFOLLOW=%d\n",
           MNT_FORCE, MNT_DETACH, MNT_EXPIRE, UMOUNT_NOFOLLOW);

    /* ---------- ⑤ 两套位值 ---------- */
    sec("⑤ MS_* 与 MNT_* 是两套完全不同的位（内核内部对不上 ABI）");
    printf("  | 用户态 MS_*（uapi）      | 内核 mnt_flags（MNT_*） |\n");
    printf("  | MS_NOSUID   = %-9lu | MNT_NOSUID   = 0x01     |\n", (unsigned long) MS_NOSUID);
    printf("  | MS_NODEV    = %-9lu | MNT_NODEV    = 0x02     |\n", (unsigned long) MS_NODEV);
    printf("  | MS_NOEXEC   = %-9lu | MNT_NOEXEC   = 0x04     |\n", (unsigned long) MS_NOEXEC);
    printf("  | MS_NOATIME  = %-9lu | MNT_NOATIME  = 0x08     |\n", (unsigned long) MS_NOATIME);
    printf("  | MS_NODIRATIME= %-8lu | MNT_NODIRATIME = 0x10   |\n", (unsigned long) MS_NODIRATIME);
    printf("  | MS_RELATIME = %-9lu | MNT_RELATIME = 0x20     |\n", (unsigned long) MS_RELATIME);
    printf("  | MS_RDONLY   = %-9lu | MNT_READONLY = 0x40     |\n", (unsigned long) MS_RDONLY);
    printf("  值完全不同 —— 它们属于两层：MS_* 是 mount(2) 的入参编码，\n");
    printf("  MNT_* 存在 vfsmount->mnt_flags 里，是挂载**实例**的选项。\n");
    printf("  第三套是 statfs 的 ST_*（见 §14.11），第四套是 mount_setattr() 的 MOUNT_ATTR_*。\n");

    printf("\n=== c14_8 done ===\n");
    return 0;
}
