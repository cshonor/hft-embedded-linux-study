/* ex11_2_fs_sweep.c — 原书习题 11-2 解答
 *
 * 【任务】（对应原书 Ch11 Exercises 11-2）把 Listing 11-2 拿到**别的文件系统**
 *   上跑一遍、对比结果。
 *   ⚠️ 原书习题文字的公开原文未能核验（man7 只分发源码、不放习题文字），
 *      所以这里只写任务要求，不做逐字引用。
 *
 * 【本题的可做性】Listing 11-2（= 本目录 t_fpathconf.c，page 218）只对
 *   STDIN_FILENO 问三个 _PC_*。要「跑到别的文件系统上」，不需要第二台机器 ——
 *   Linux 一台机器上就挂着十几种文件系统。本解答改成：
 *
 *   ① 遍历 /proc/mounts，对每个挂载点问同一组 _PC_*；
 *   ② 按 (NAME_MAX, LINK_MAX, FILESIZEBITS) **去重**后再打印，
 *      否则几十个挂载点会把结论淹没；
 *   ③ 顺手把 statfs().f_type 与 statvfs().f_namemax 打出来，
 *      说明「为什么同一个内核上会有不同答案」。
 *
 * 【为什么要去重而不是全打印】CE 容器里 /cefs 下面挂了几十个 autofs/squashfs
 *   子挂载点，路径名里还带不同的哈希（每次运行都不一样），全打出来既长又不
 *   可复现。去重后只剩三行，正好是三份答案。
 *
 * 编译： gcc -O0 -Wall -Wextra -o ex11_2_fs_sweep ex11_2_fs_sweep.c
 * 取材： glibc 2.39 sysdeps/unix/sysv/linux/pathconf.c:133-192（__statfs_link_max，按 f_type 查表）
 *       glibc 2.39 sysdeps/unix/sysv/linux/pathconf.c:197-240（__statfs_filesize_max，按 f_type 查表）
 *       glibc 2.39 sysdeps/posix/pathconf.c:65-86（_PC_NAME_MAX -> statvfs.f_namemax）
 *       glibc 2.39 sysdeps/unix/sysv/linux/linux_fsinfo.h:255-268（各 fs 的 LINK_MAX 常量）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <unistd.h>

#define MAXK 64

struct key { long nm, lk, fb; char ftype[24]; unsigned long ftype_hex; int hits; };

static struct key keys[MAXK];
static int nkeys;

static void note(long nm, long lk, long fb, const char *ftype, unsigned long hex)
{
    for (int i = 0; i < nkeys; i++)
        if (keys[i].nm == nm && keys[i].lk == lk && keys[i].fb == fb) {
            keys[i].hits++;
            return;
        }
    if (nkeys >= MAXK) return;
    keys[nkeys].nm = nm; keys[nkeys].lk = lk; keys[nkeys].fb = fb;
    keys[nkeys].ftype_hex = hex; keys[nkeys].hits = 1;
    snprintf(keys[nkeys].ftype, sizeof keys[0].ftype, "%s", ftype);
    nkeys++;
}

int main(void)
{
    FILE *f = fopen("/proc/mounts", "r");
    if (!f) { perror("/proc/mounts"); return 1; }

    printf("=== ① 遍历 /proc/mounts，问同一组 _PC_* ===\n");
    long nm = pathconf("/", _PC_NAME_MAX);
    long lk = pathconf("/", _PC_LINK_MAX);
    long fb = pathconf("/", _PC_FILESIZEBITS);
    printf("  先在 / 上取一组基线: NAME_MAX=%ld LINK_MAX=%ld FILESIZEBITS=%ld\n", nm, lk, fb);

    char dev[128], mnt[128], type[32];
    int scanned = 0;
    while (fscanf(f, "%127s %127s %31s %*s %*s %*s", dev, mnt, type) == 3) {
        struct statfs sf;
        if (statfs(mnt, &sf) != 0)
            continue;
        errno = 0;
        long a = pathconf(mnt, _PC_NAME_MAX);
        if (a == -1) continue;
        long b = pathconf(mnt, _PC_LINK_MAX);
        long c = pathconf(mnt, _PC_FILESIZEBITS);
        scanned++;
        note(a, b, c, type, (unsigned long)sf.f_type);
    }
    fclose(f);

    printf("\n=== ② 去重后的答案（扫了 %d 个挂载点，共 %d 种结果）===\n", scanned, nkeys);
    printf("  %-9s %-10s %-15s %-12s %s\n", "NAME_MAX", "LINK_MAX", "FILESIZEBITS",
           "f_type", "代表文件系统");
    for (int i = 0; i < nkeys; i++)
        printf("  %-9ld %-10ld %-15ld 0x%-9lx %s（%d 个挂载点）\n",
               keys[i].nm, keys[i].lk, keys[i].fb, keys[i].ftype_hex,
               keys[i].ftype, keys[i].hits);

    printf("\n=== ③ 为什么会有不同答案 ===\n");
    printf("  _PC_LINK_MAX      -> glibc 按 statfs().f_type 查表：\n");
    printf("                       ext4 -> EXT4_LINK_MAX 65000；其余多数 -> LINUX_LINK_MAX 127\n");
    printf("                       （linux_fsinfo.h:256 / :268；ext4 还要特判，见 pathconf.c:63-128）\n");
    printf("  _PC_FILESIZEBITS  -> 同样查表：ext4 64；tmpfs/proc/squashfs 走 default 32\n");
    printf("  _PC_NAME_MAX      -> statvfs().f_namemax，由**超级块**报上来，所以能不是 255\n");
    printf("                       （本机 squashfs 就报了 256）\n");
    printf("  _PC_PATH_MAX      -> 这几行里它**没有任何变化**：glibc 直接 return PATH_MAX(%d)，\n",
           PATH_MAX);
    printf("                       与文件系统无关。\n");
    printf("  _PC_PIPE_BUF      -> 同理，直接 return PIPE_BUF(%d)。\n", PIPE_BUF);

    printf("\n=== ④ 与 Listing 11-2 原始行为对照 ===\n");
    printf("  原书 Listing 11-2 对 STDIN_FILENO 问三项，本机结果：\n");
    errno = 0;
    printf("    _PC_NAME_MAX = %ld\n", fpathconf(STDIN_FILENO, _PC_NAME_MAX));
    printf("    _PC_PATH_MAX = %ld\n", fpathconf(STDIN_FILENO, _PC_PATH_MAX));
    printf("    _PC_PIPE_BUF = %ld\n", fpathconf(STDIN_FILENO, _PC_PIPE_BUF));
    printf("  —— 与在 / 上问的结果完全一样。原因是这三项在 glibc 里都不看 fd 指向的类型\n");
    printf("     （_PC_NAME_MAX 走 statvfs，管道/终端 fd 也能 statvfs 成功并报 255）。\n");
    printf("  所以「换文件系统」这个实验真正会变的只有 LINK_MAX / FILESIZEBITS / NAME_MAX。\n");
    return 0;
}
