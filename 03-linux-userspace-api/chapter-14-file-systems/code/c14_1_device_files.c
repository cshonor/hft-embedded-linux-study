/* c14_1_device_files.c —— TLPI §14.1 设备特殊文件（Device Special Files）
 *
 *  演示「设备号三件套」：makedev() 造、major()/minor() 拆、st_rdev 读。
 *
 *    ① dev_t 的编码：Linux 用 32 位编 major/minor（12 + 20 位）
 *    ② st_dev（文件所在的文件系统）vs st_rdev（设备本身）—— 最容易被混淆的一对，
 *       并且用「/dev 与 /dev/null 的 st_dev 不同」坐实「单文件 bind 挂载」
 *    ③ 扫 /dev：**只看 st_mode 的类型位**，不看文件名
 *    ④ /proc/devices：主设备号 → 驱动名 的权威对照
 *    ⑤ mknod(2)：造 FIFO 免特权，造设备节点要 CAP_MKNOD
 *    ⑥ /dev/null 与 /dev/zero 的行为差异
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_1 c14_1_device_files.c
 */
#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void sec(const char *t)
{
    printf("\n== %s ==\n", t);
}

/* 打一个路径的设备信息：类型位是唯一判据，名字不算数 */
static void show_dev(const char *path)
{
    struct stat st;
    const char *kind;

    if (stat(path, &st) == -1) {
        printf("  %-20s stat 失败 errno=%d (%s)\n", path, errno, strerror(errno));
        return;
    }
    kind = S_ISCHR(st.st_mode) ? "chr" : S_ISBLK(st.st_mode) ? "blk" :
           S_ISDIR(st.st_mode) ? "dir" : S_ISREG(st.st_mode) ? "reg" :
           S_ISLNK(st.st_mode) ? "lnk" : S_ISSOCK(st.st_mode) ? "sock" : "???";
    printf("  %-20s %-4s mode=%07o st_dev=%u:%-4u", path, kind,
           (unsigned) st.st_mode & 077777, major(st.st_dev), minor(st.st_dev));
    if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode))
        printf(" st_rdev=%u:%u", major(st.st_rdev), minor(st.st_rdev));
    else
        printf(" st_rdev 无意义");
    printf("\n");
}

/* 在 /proc/devices 里找「主设备号 → 驱动名」*/
static void lookup_major(unsigned int want)
{
    FILE *fp = fopen("/proc/devices", "r");
    char line[256];

    if (fp == NULL) {
        printf("  /proc/devices: errno=%d (%s)\n", errno, strerror(errno));
        return;
    }
    while (fgets(line, sizeof(line), fp) != NULL) {
        unsigned int maj;
        char name[128];
        if (sscanf(line, " %u %127s", &maj, name) == 2 && maj == want) {
            printf("  /proc/devices: 主设备号 %u -> %s\n", maj, name);
            fclose(fp);
            return;
        }
    }
    fclose(fp);
    printf("  /proc/devices: 没找到主设备号 %u\n", want);
}

int main(void)
{
    /* ---------- ① dev_t 的编码 ---------- */
    sec("① dev_t 的编码：makedev() 与 major()/minor()");
    {
        dev_t d1 = makedev(1, 3);
        dev_t d2 = makedev(259, 1);
        dev_t d3 = makedev(0xfff, 0xfffff);

        printf("  makedev(1, 3)         = %-12llu 0x%llx -> %u:%u\n",
               (unsigned long long) d1, (unsigned long long) d1,
               major(d1), minor(d1));
        printf("  makedev(259, 1)       = %-12llu 0x%llx -> %u:%u\n",
               (unsigned long long) d2, (unsigned long long) d2,
               major(d2), minor(d2));
        printf("  makedev(0xfff, 0xfffff) = %-12llu -> %u:%u  （12 位 major + 20 位 minor 的上限）\n",
               (unsigned long long) d3, major(d3), minor(d3));
        printf("  sizeof(dev_t) = %zu（glibc 侧 64 位，内核编码只用低 32 位）\n",
               sizeof(dev_t));
    }

    /* ---------- ② st_dev vs st_rdev ---------- */
    sec("② st_dev（在哪个文件系统上）vs st_rdev（是哪个设备）");
    show_dev("/dev");
    show_dev("/dev/null");
    show_dev("/dev/zero");
    show_dev("/dev/random");
    show_dev("/dev/urandom");
    show_dev("/etc/passwd");
    show_dev("/app");
    printf("  ↑ /dev/null 的 st_dev 是它所在文件系统的设备号，st_rdev 才是字符设备 1:3\n");
    printf("  ↑ 把 /dev 与 /dev/null 的 st_dev 对一下：**不一样** ⇒ /dev/null 并不是\n");
    printf("    「/dev 这个目录里的一个成员」，而是**另一个文件系统上的文件被挂到这个名字上**\n");
    printf("    （mountinfo 里它是一条 `root=/null` 的独立记录 —— 见 §14.9.4 的单文件 bind）。\n");
    printf("    「同一个目录下的条目 st_dev 却不同」是纯靠 stat 就能看出来的硬证据。\n");

    /* ---------- ③ 扫 /dev ---------- */
    sec("③ 扫 /dev：类型位说了算，名字说了不算");
    {
        DIR *dir = opendir("/dev");
        struct dirent *e;
        int nchr = 0, nblk = 0, nother = 0;

        if (dir == NULL) {
            printf("  opendir(/dev) errno=%d (%s)\n", errno, strerror(errno));
        } else {
            while ((e = readdir(dir)) != NULL) {
                char p[512];
                struct stat st;
                const char *kind;

                if (e->d_name[0] == '.' && (e->d_name[1] == '\0' ||
                                            (e->d_name[1] == '.' && e->d_name[2] == '\0')))
                    continue;
                snprintf(p, sizeof(p), "/dev/%s", e->d_name);
                if (lstat(p, &st) == -1)
                    continue;
                kind = S_ISCHR(st.st_mode) ? "chr" : S_ISBLK(st.st_mode) ? "blk" : "oth";
                if (S_ISCHR(st.st_mode)) nchr++;
                else if (S_ISBLK(st.st_mode)) nblk++;
                else nother++;
                printf("  %-22s %-4s mode=%07o", p, kind, (unsigned) st.st_mode & 077777);
                if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode))
                    printf(" st_rdev=%u:%u", major(st.st_rdev), minor(st.st_rdev));
                printf("\n");
            }
            closedir(dir);
            printf("  小计：字符设备 %d 个、块设备 %d 个、其它 %d 个\n",
                   nchr, nblk, nother);
        }
    }

    /* ---------- ④ /proc/devices ---------- */
    sec("④ /proc/devices：主设备号 ↔ 驱动名");
    lookup_major(1);      /* mem：/dev/null, /dev/zero, /dev/random 都挂在这里 */
    lookup_major(5);      /* tty：/dev/tty, /dev/console, /dev/ptmx */
    lookup_major(259);    /* nvme 块设备 */

    /* ---------- ⑤ mknod(2) ---------- */
    sec("⑤ mknod(2)：造 FIFO 不需要特权，造设备节点要 CAP_MKNOD");
    {
        unlink("/app/c14_1_fifo");
        errno = 0;
        if (mknod("/app/c14_1_fifo", S_IFIFO | 0666, 0) == -1)
            printf("  mknod(FIFO)   -> -1 errno=%d (%s)\n", errno, strerror(errno));
        else
            printf("  mknod(FIFO)   -> 0  **成功**（FIFO 与普通文件不走设备号）\n");

        unlink("/app/c14_1_node");
        errno = 0;
        if (mknod("/app/c14_1_node", S_IFCHR | 0666, makedev(1, 3)) == -1)
            printf("  mknod(chr 1:3) -> -1 errno=%d (%s)  ← 缺 CAP_MKNOD\n",
                   errno, strerror(errno));
        else
            printf("  mknod(chr 1:3) -> 0  **成功**\n");

        unlink("/app/c14_1_fifo");
        unlink("/app/c14_1_node");
    }

    /* ---------- ⑥ /dev/null vs /dev/zero ---------- */
    sec("⑥ /dev/null（写丢弃、读 EOF）vs /dev/zero（写丢弃、读无穷个 0）");
    {
        char buf[16];
        ssize_t n;
        int fd;

        fd = open("/dev/null", O_RDWR);
        if (fd == -1) {
            printf("  open(/dev/null) errno=%d (%s)\n", errno, strerror(errno));
        } else {
            n = write(fd, "abc", 3);
            printf("  write(/dev/null, 3 字节)  = %zd\n", n);
            n = read(fd, buf, sizeof(buf));
            printf("  read(/dev/null, 16 字节)  = %zd  （0 = EOF）\n", n);
            close(fd);
        }

        fd = open("/dev/zero", O_RDONLY);
        if (fd == -1) {
            printf("  open(/dev/zero) errno=%d (%s)\n", errno, strerror(errno));
        } else {
            memset(buf, 0xAA, sizeof(buf));
            n = read(fd, buf, sizeof(buf));
            printf("  read(/dev/zero, 16 字节)  = %zd，首字节 = 0x%02x（永远读到 0，不会 EOF）\n",
                   n, (unsigned char) buf[0]);
            errno = 0;
            n = read(fd, buf, 0);
            printf("  read(/dev/zero, 0 字节)   = %zd  errno=%d\n", n, errno);
            close(fd);
        }
    }

    printf("\n=== c14_1 done ===\n");
    return 0;
}
