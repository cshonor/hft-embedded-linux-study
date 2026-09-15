/* c12_1_proc_nodes.c — Ch12 §12.1（节首）：/proc 不是一个真的文件系统
 *
 * TLPI §12.1 第一件事不是「有哪些文件」，而是「这是一种什么文件系统」：
 * 它的目录项由内核**运行时生成**，没有后备存储块，所以拿它当一个普通文件
 * 来推断（比如按 st_size 分配缓冲、拿 statfs 算容量）一定会错。
 *
 * 本程序用三个反直觉点把它钉死：
 *   ① statfs("/proc").f_blocks == 0  —— 一个字节的存储都没有
 *   ② stat("/proc/version").st_size == 0，但实际能读出上百字节
 *   ③ /proc/self 不是目录，是内核给的**魔术符号链接**
 *
 * 编译： gcc -O0 -Wall -Wextra -o c12_1_proc_nodes c12_1_proc_nodes.c
 * 取材： man-pages 6.19 proc(5) 开篇（"The proc filesystem is a pseudo-filesystem
 *         which provides an interface to kernel data structures."）
 *       Linux v6.6 fs/proc/root.c:31-46（proc_fs_type = .name = "proc"）
 *                      fs/proc/inode.c（proc_reg_* 的 file_operations）
 *                      fs/proc/self.c:11-24（proc_self_get_link）
 *                      include/uapi/linux/magic.h（PROC_SUPER_MAGIC 0x9fa0）
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

#define PROC_SUPER_MAGIC 0x9fa0
#define TMPFS_MAGIC      0x01021994

static const char *fsname(long t)
{
    switch ((unsigned long) t) {
    case PROC_SUPER_MAGIC: return "proc";
    case TMPFS_MAGIC:      return "tmpfs";
    case 0xef53:           return "ext2/3/4";
    case 0x6969:           return "nfs";
    case 0x73717368:       return "squashfs";
    default:               return "(其它)";
    }
}

static void show_statfs(const char *path)
{
    struct statfs sf;
    errno = 0;
    if (statfs(path, &sf) == -1) {
        printf("  statfs(%-8s) 失败 errno=%d(%s)\n", path, errno, strerror(errno));
        return;
    }
    printf("  statfs(%-8s) f_type=0x%-9lx (%-9s) f_bsize=%-6ld f_blocks=%-10llu f_files=%-10llu\n",
           path, (unsigned long) sf.f_type, fsname(sf.f_type), (long) sf.f_bsize,
           (unsigned long long) sf.f_blocks, (unsigned long long) sf.f_files);
}

int main(void)
{
    /* ---------- ① 与真的文件系统对比 ---------- */
    printf("== ① statfs 对比：/proc 的 f_blocks 是 0 ==\n");
    show_statfs("/");
    show_statfs("/proc");
    show_statfs("/dev");
    show_statfs("/tmp");
    printf("  → /proc 是伪文件系统：f_blocks/f_files 都是 0，"
           "「容量」这个概念对它不成立\n\n");

    /* ---------- ② st_size == 0 但不代表内容为空 ---------- */
    printf("== ② stat(\"/proc/version\")：st_size 是 0 ==\n");
    struct stat st;
    errno = 0;
    if (stat("/proc/version", &st) == -1) {
        printf("  stat 失败 errno=%d(%s)\n", errno, strerror(errno));
    } else {
        printf("  st_mode = 0%o  S_ISREG = %d  （普通文件，权限 0444）\n",
               st.st_mode, S_ISREG(st.st_mode) ? 1 : 0);
        printf("  st_size = %lld   st_blocks = %lld   st_nlink = %lu\n",
               (long long) st.st_size, (long long) st.st_blocks,
               (unsigned long) st.st_nlink);
        printf("  st_ino  = %llu（内核编的，不是磁盘上的 inode 号）\n",
               (unsigned long long) st.st_ino);

        /* 再看 offset 100 处有没有「数据块」：/proc 文件是顺序生成的流 */
        int fd = open("/proc/version", O_RDONLY);
        if (fd != -1) {
            char buf[512];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            if (n == -1) {
                printf("  read 失败 errno=%d(%s)\n", errno, strerror(errno));
            } else {
                buf[n] = '\0';
                printf("  实际读出 %zd 字节（st_size 却报 0）\n", n);
                if (n && buf[n - 1] == '\n')
                    buf[n - 1] = '\0';
                printf("  内容: %s\n", buf);
            }
            close(fd);
        }
        printf("  ⚠️ 所以**不能**用 fstat 的 st_size 去 malloc 缓冲："
               "proc 文件的大小要「读到 EOF」才知道\n\n");

        /* 重复读同一个文件：内容每次重新生成 */
        printf("== ③ 同一个 /proc 文件反复打开，内容每次重新生成 ==\n");
        for (int i = 0; i < 2; i++) {
            /* 两次之间必须隔开一点时间，否则同一时钟节拍里读到的秒数一模一样，
               反而看不出「内容是按 read 时刻现算的」 */
            if (i == 1)
                usleep(200 * 1000);         /* 0.2 秒 */
            fd = open("/proc/uptime", O_RDONLY);
            if (fd != -1) {
                char b[128];
                ssize_t n = read(fd, b, sizeof(b) - 1);
                if (n > 0) {
                    b[n] = '\0';
                    printf("  第 %d 次 open(\"/proc/uptime\") + read → %s", i + 1, b);
                }
                close(fd);
            }
        }
        printf("  → 两次的秒数相差约 0.2（就是上面的 usleep）：/proc 文件并没有\n");
        printf("     「磁盘上那一份内容」，值由内核在 read 时按当前时刻现算\n\n");
    }

    /* ---------- ④ /proc/self 是魔术符号链接 ---------- */
    printf("== ④ /proc/self 不是目录，是内核造的符号链接 ==\n");
    char target[PATH_MAX];
    errno = 0;
    ssize_t n = readlink("/proc/self", target, sizeof(target) - 1);
    if (n == -1) {
        printf("  readlink(\"/proc/self\") 失败 errno=%d(%s)\n", errno, strerror(errno));
    } else {
        target[n] = '\0';
        printf("  readlink(\"/proc/self\") = \"%s\"   （本进程 getpid()=%d）\n",
               target, (int) getpid());
    }
    errno = 0;
    n = readlink("/proc/thread-self", target, sizeof(target) - 1);
    if (n != -1) {
        target[n] = '\0';
        printf("  readlink(\"/proc/thread-self\") = \"%s\"\n", target);
    }
    printf("  lstat 看它的类型：");
    struct stat lst;
    errno = 0;
    if (lstat("/proc/self", &lst) == 0)
        printf("S_ISLNK = %d\n", S_ISLNK(lst.st_mode) ? 1 : 0);
    else
        printf("lstat 失败 errno=%d(%s)\n", errno, strerror(errno));
    printf("  ⚠️ 但 open(\"/proc/self/status\") 照样能进 —— 链接由内核解析，"
           "不经过磁盘\n\n");

    /* ---------- ⑤ 顶层到底挂了些什么 ---------- */
    printf("== ⑤ /proc 顶层：数字目录 = 进程，其余 = 内核接口 ==\n");
    DIR *d = opendir("/proc");
    if (d == NULL) {
        printf("  opendir(\"/proc\") 失败 errno=%d(%s)\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }
    struct dirent *e;
    int npids = 0, nothers = 0;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.')
            continue;
        if (e->d_name[0] >= '0' && e->d_name[0] <= '9')
            npids++;
        else
            nothers++;
    }
    closedir(d);
    printf("  数字目录（进程）= %d 个\n", npids);
    printf("  其它条目（内核接口）= %d 个\n", nothers);
    printf("  ⚠️ 本容器里只有 %d 个进程，所以 /proc/PID 的样本极少；"
           "真机上是三位数\n", npids);
    return EXIT_SUCCESS;
}
