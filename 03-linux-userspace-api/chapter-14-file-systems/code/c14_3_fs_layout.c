/* c14_3_fs_layout.c —— TLPI §14.3 File Systems（传统布局：以 ext2/ext4 为范例）
 *
 *  §14.3 讲的是「块组」布局：boot block + superblock + group descriptor +
 *  block bitmap + inode bitmap + inode table + data blocks。要看这套东西
 *  通常得用 dumpe2fs 读裸设备 —— **容器里没有裸设备，做不了**。
 *
 *  所以本 demo 改成「用能拿到的东西反推」：
 *    ① statfs() 的字段就是 superblock 在用户态的那一面
 *       （块大小、总块数、总 inode 数……）
 *    ② /proc/fs/ext4/<dev>/ ：内核把每个已挂载 ext4 的运行时参数挂在这里，
 *       这才是「不读裸设备也能看 ext4」的入口
 *    ③ /proc/fs/jbd2/ ：ext4 的日志层（jbd2）的运行时接口
 *
 *  ⚠️ 诚实标注：本 demo **不能**验证块组的物理布局（那需要 dumpe2fs + 裸设备），
 *    只能验证「superblock 在用户态的投影」与「内核暴露的运行时参数」。
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_3 c14_3_fs_layout.c
 */
#define _GNU_SOURCE
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void sec(const char *t)
{
    printf("\n== %s ==\n", t);
}

/* 列出目录条目（最多 maxn 个），返回 0 表示成功 */
static int list_dir(const char *path, int maxn)
{
    DIR *d = opendir(path);
    struct dirent *e;
    int n = 0;

    if (d == NULL) {
        printf("  opendir(%s) -> errno=%d (%s)\n", path, errno, strerror(errno));
        return -1;
    }
    printf("  > %s：\n", path);
    while ((e = readdir(d)) != NULL && n < maxn) {
        if (e->d_name[0] == '.' &&
            (e->d_name[1] == '\0' || (e->d_name[1] == '.' && e->d_name[2] == '\0')))
            continue;
        printf("    %s\n", e->d_name);
        n++;
    }
    closedir(d);
    return 0;
}

/* 取目录里第一个非 . 条的条目名（用于找 /proc/fs/ext4 下的设备名）*/
static int first_entry(const char *path, char *out, size_t cap)
{
    DIR *d = opendir(path);
    struct dirent *e;

    out[0] = '\0';
    if (d == NULL)
        return -1;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.' &&
            (e->d_name[1] == '\0' || (e->d_name[1] == '.' && e->d_name[2] == '\0')))
            continue;
        snprintf(out, cap, "%s", e->d_name);
        break;
    }
    closedir(d);
    return out[0] == '\0' ? -1 : 0;
}

/* 打一个文件的前 maxlines 行 */
static void dump_file(const char *path, int maxlines)
{
    FILE *fp = fopen(path, "r");
    char line[512];
    int n = 0;

    if (fp == NULL) {
        printf("    %-40s 打不开（errno=%d）\n", path, errno);
        return;
    }
    printf("    --- %s ---\n", path);
    while (fgets(line, sizeof(line), fp) != NULL && n < maxlines) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';
        printf("    %s\n", line);
        n++;
    }
    fclose(fp);
}

int main(void)
{
    /* ---------- ① superblock 在用户态的投影 ---------- */
    sec("① statfs() 字段 = superblock 的用户态投影（ext4 示例）");
    {
        struct statfs sfs;

        if (statfs("/app", &sfs) == -1) {
            printf("  statfs(/app) errno=%d (%s)\n", errno, strerror(errno));
        } else {
            printf("  f_type   = %#-12lx  ← magic 0xef53 = ext2/3/4（superblock 第一眼）\n",
                   (unsigned long) sfs.f_type);
            printf("  f_bsize  = %-12lu  块大小（superblock 的核心参数，块组的粒度）\n",
                   (unsigned long) sfs.f_bsize);
            printf("  f_blocks = %-12llu  总块数 × 块大小 = 卷容量 %llu 字节\n",
                   (unsigned long long) sfs.f_blocks,
                   (unsigned long long) sfs.f_blocks * sfs.f_bsize);
            printf("  f_files  = %-12llu  inode 总数（inode table 的总容量）\n",
                   (unsigned long long) sfs.f_files);
            printf("  f_ffree  = %-12llu  空闲 inode\n",
                   (unsigned long long) sfs.f_ffree);
            printf("  f_namelen= %-12lu  单个文件名上限\n", (unsigned long) sfs.f_namelen);
            printf("  → 已用 inode %llu 个，占 %.3f%%\n",
                   (unsigned long long) (sfs.f_files - sfs.f_ffree),
                   100.0 * (double) (sfs.f_files - sfs.f_ffree) / (double) sfs.f_files);
        }
    }

    /* ---------- ② /proc/fs/ext4 ---------- */
    sec("② /proc/fs/ext4/<dev>/：不读裸设备也能看 ext4 的运行时参数");
    list_dir("/proc/fs/ext4", 8);
    {
        char dev[128];
        char path[512];

        if (first_entry("/proc/fs/ext4", dev, sizeof(dev)) == 0) {
            printf("  > 第一个 ext4 实例：%s\n", dev);
            snprintf(path, sizeof(path), "/proc/fs/ext4/%s", dev);
            list_dir(path, 20);
        } else {
            printf("  /proc/fs/ext4 里没有条目\n");
        }
    }

    /* ---------- ③ /proc/fs/jbd2 ---------- */
    sec("③ /proc/fs/jbd2/<dev>-<n>/：ext4 的日志层（jbd2）");
    list_dir("/proc/fs/jbd2", 8);
    {
        char dev[128];
        char path[512];

        if (first_entry("/proc/fs/jbd2", dev, sizeof(dev)) == 0) {
            snprintf(path, sizeof(path), "/proc/fs/jbd2/%s", dev);
            printf("  > 第一个 jbd2 实例：%s\n", dev);
            list_dir(path, 20);
            snprintf(path, sizeof(path), "/proc/fs/jbd2/%s/info", dev);
            dump_file(path, 14);
        }
    }

    /* ---------- ④ 块组结构只能在笔记里讲 ---------- */
    sec("④ 块组的物理布局——本容器无法实测（只有组数能算）");
    printf("  按 v6.6 自带文档 Documentation/filesystems/ext4/blockgroup.rst:6-28，\n"
           "  一个「标准块组」从上到下是这些段：\n"
           "    Group 0 Padding(1024 字节，只有 0 号组有) / ext4 Super Block /\n"
           "    Group Descriptors / Reserved GDT Blocks / Data Block Bitmap /\n"
           "    inode Bitmap / inode Table / Data Blocks\n"
           "  其中超级块与组描述符只在**部分**块组里有冗余副本\n"
           "  （blockgroup.rst:37-43）；位图与 inode 表的位置由\n"
           "  grp.bg_inode_table_* 给出，ext4 的 flex_bg 还可以把它们挪到别的组\n"
           "  （blockgroup.rst:50-60）。\n"
           "  要验证这些必须读裸设备（dumpe2fs 那种工具），而本容器 /dev 下只有\n"
           "  4 个字符设备、没有任何块设备 —— 所以这一段只有「概念正确性」而\n"
           "  没有本机实测数字。\n");
    {
        struct statfs sfs;
        if (statfs("/app", &sfs) == 0) {
            unsigned long long per_group = 8ULL * (unsigned long long) sfs.f_bsize;
            unsigned long long nblk = (unsigned long long) sfs.f_blocks;
            unsigned long long ng = (nblk + per_group - 1) / per_group;

            printf("  能算的只有**组数**：文档说组大小 = 8 × 块大小，4 KiB 块 =>\n"
                   "  32768 块/组 = 128 MiB（ext4/overview.rst:9-13）。\n");
            printf("    /app 的 f_bsize   = %ld 字节\n", (long) sfs.f_bsize);
            printf("    /app 的 f_blocks  = %llu 块（来自 statfs，见 ①）\n", nblk);
            printf("    每组的块数        = 8 × %ld = %llu 块 = %llu 字节\n",
                   (long) sfs.f_bsize, per_group, per_group * (unsigned long long) sfs.f_bsize);
            printf("    组数 ≈ %llu / %llu = %.2f  → 向上取整 %llu 组\n",
                   nblk, per_group, (double) nblk / (double) per_group, ng);
            printf("    ⚠ 这是拿**文档公式**推出来的，不是实测块组表；\n"
                   "      「这些组到底怎么排」只能读裸设备才知道。\n");
        } else {
            printf("  statfs(/app) errno=%d (%s)\n", errno, strerror(errno));
        }
    }

    printf("\n=== c14_3 done ===\n");
    return 0;
}
