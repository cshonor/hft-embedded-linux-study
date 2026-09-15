/* c14_2_disks_partitions.c —— TLPI §14.2 Disks and Partitions
 *
 *  演示「块设备 + 分区」这一层在容器里能看到多少：
 *    ① /proc/partitions：内核认到的块设备清单（major minor #blocks name）
 *    ② 把 #blocks（1 KiB 单位）换算成人类单位
 *    ③ stat("/app").st_dev → 反查它落在哪个分区上
 *    ④ 一个 major 底下都有谁（整盘与分区可能都在 259/blkext 上）
 *    ⑤ /proc/devices 的块设备段
 *    ⑥ 为什么这里找不到 /sys/dev/block（容器里 /sys 不是 sysfs）
 *
 *  ⚠️ 本章**做不了**的事（诚实标注）：
 *    容器里没有可读的裸块设备（/dev 下只有 4 个字符设备），
 *    所以 fsck / dumpe2fs / mkfs / fdisk 一类工具全都没法实测。
 *    这一节的可实测边界就在 /proc 的这几张表上。
 *
 *  编译：gcc -O0 -Wall -Wextra -o c14_2 c14_2_disks_partitions.c
 */
#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/sysmacros.h>
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

#define MAXP 256

static void human(unsigned long long kb, char *out, size_t cap)
{
    if (kb >= 1024ULL * 1024)
        snprintf(out, cap, "%.2f GiB", (double) kb / (1024.0 * 1024.0));
    else if (kb >= 1024)
        snprintf(out, cap, "%.2f MiB", (double) kb / 1024.0);
    else
        snprintf(out, cap, "%llu KiB", kb);
}

int main(void)
{
    /* ---------- ① /proc/partitions ---------- */
    sec("① /proc/partitions（内核认到的块设备）");
    {
        FILE *fp = fopen("/proc/partitions", "r");
        char line[256];
        int n = 0;

        if (fp == NULL) {
            printf("  errno=%d (%s)\n", errno, strerror(errno));
        } else {
            while (fgets(line, sizeof(line), fp) != NULL) {
                unsigned int maj, min;
                unsigned long long blk;
                char name[64];
                char h[32];

                if (sscanf(line, " %u %u %llu %63s", &maj, &min, &blk, name) != 4) {
                    printf("  %s", line);          /* 表头 */
                    continue;
                }
                human(blk, h, sizeof(h));
                printf("  %3u:%-4u %-14s %12llu KiB  (%s)\n", maj, min, name, blk, h);
                n++;
            }
            fclose(fp);
            printf("  共 %d 个块设备条目\n", n);
        }
    }

    /* ---------- ② 从设备号反查分区 ---------- */
    sec("② stat(\"/app\").st_dev → 落在哪个分区");
    {
        struct stat st;
        unsigned int want_maj, want_min;

        if (stat("/app", &st) == -1) {
            printf("  stat(/app) errno=%d (%s)\n", errno, strerror(errno));
        } else {
            want_maj = major(st.st_dev);
            want_min = minor(st.st_dev);
            printf("  st_dev = %u:%u（这是「文件系统实例」的设备号，不是分区号）\n",
                   want_maj, want_min);

            FILE *fp = fopen("/proc/partitions", "r");
            char line[256];
            int hit = 0;
            if (fp != NULL) {
                while (fgets(line, sizeof(line), fp) != NULL) {
                    unsigned int maj, min;
                    unsigned long long blk;
                    char name[64];
                    if (sscanf(line, " %u %u %llu %63s", &maj, &min, &blk, name) != 4)
                        continue;
                    if (maj == want_maj && min == want_min) {
                        printf("  /proc/partitions 里匹配到 -> %s（%llu KiB）\n", name, blk);
                        hit = 1;
                    }
                }
                fclose(fp);
            }
            if (!hit)
                printf("  /proc/partitions 里**没有** %u:%u 这一条 ——\n"
                       "  说明它是一个 bind mount 出来的视图，设备号来自宿主上的另一个实例。\n",
                       want_maj, want_min);
        }
    }

    /* ---------- ④ 按 major 分组：一个 major 底下都有谁 ---------- */
    sec("④ 一个 major 底下都有谁（/proc/partitions 按主设备号分组）");
    {
        char name[MAXP][64];
        unsigned int maj[MAXP], mino[MAXP];
        int np = 0;
        FILE *fp = fopen("/proc/partitions", "r");
        char line[256];

        if (fp == NULL) {
            printf("  /proc/partitions errno=%d (%s)\n", errno, strerror(errno));
        } else {
            while (np < MAXP && fgets(line, sizeof(line), fp) != NULL) {
                unsigned int m, mi;
                unsigned long long blk;
                char nm[64];
                if (sscanf(line, " %u %u %llu %63s", &m, &mi, &blk, nm) != 4)
                    continue;
                snprintf(name[np], sizeof(name[0]), "%s", nm);
                maj[np] = m;
                mino[np] = mi;
                np++;
            }
            fclose(fp);
            if (np == MAXP)
                printf("  ⚠ 只读了前 %d 条（缓冲区上限），下面的分组不完整\n", MAXP);

            unsigned int done[MAXP];
            int ndone = 0;

            for (int i = 0; i < np; i++) {
                int dup = 0;
                for (int k = 0; k < ndone; k++)
                    if (done[k] == maj[i])
                        dup = 1;
                if (dup)
                    continue;
                if (ndone < MAXP)
                    done[ndone++] = maj[i];

                int cnt = 0;
                for (int j = 0; j < np; j++)
                    if (maj[j] == maj[i])
                        cnt++;

                printf("  major %-4u 共 %d 个条目，次要号 {", maj[i], cnt);
                for (int j = 0, first = 1; j < np; j++) {
                    if (maj[j] != maj[i])
                        continue;
                    if (!first)
                        printf(",");
                    printf("%u", mino[j]);
                    first = 0;
                }
                printf("}\n");

                /* 每组最多列 8 条，并**显式**说明被省略了多少（不静默截断） */
                int shown = 0;
                for (int j = 0; j < np && shown < 8; j++) {
                    if (maj[j] != maj[i])
                        continue;
                    printf("      %3u:%-4u %-14s\n", maj[j], mino[j], name[j]);
                    shown++;
                }
                if (cnt > shown)
                    printf("      …… 还有 %d 条没列（要全部的话直接用 head /proc/partitions）\n",
                           cnt - shown);
            }
        }
    }
    printf("  ↑ 同一片盘上的「整盘」与「分区」**不一定共用一个 major**：\n"
           "    内核给分区号有两条路（block/partitions/core.c:359-367）——\n"
           "      /* in consecutive minor range? */\n"
           "      if (bdev->bd_partno < disk->minors)\n"
           "          devt = MKDEV(disk->major, disk->first_minor + bd_partno);\n"
           "      else\n"
           "          devt = MKDEV(BLOCK_EXT_MAJOR, blk_alloc_ext_minor());\n"
           "    驱动给了明确的 major + minors 数，就沿用它；没给（disk->major == 0，\n"
           "    整盘一个设备、也不要分区窗口）就只能从「扩展 dev_t 空间」里要号码\n"
           "    （block/genhd.c:418-424 的注释：otherwise just allocate the device\n"
           "    numbers ... from the extended dev_t space）。\n"
           "    这个扩展区的 major 就是 BLOCK_EXT_MAJOR —— block/genhd.c:885 用\n"
           "    register_blkdev(BLOCK_EXT_MAJOR, \"blkext\") 把它登记成 \"blkext\"，\n"
           "    /proc/devices 的块设备段里它显示成 259。minor 由 IDA 动态发号\n"
           "    （blk_alloc_ext_minor，block/genhd.c:307-315），所以同一片盘的\n"
           "    次要号看起来是「跳着的」，不是从 0 连续排下去。\n");

    /* ---------- ⑤ /proc/devices 的块设备段 ---------- */
    sec("⑤ /proc/devices 的块设备段");
    {
        FILE *fp = fopen("/proc/devices", "r");
        char line[256];
        int n = 0;

        if (fp != NULL) {
            while (fgets(line, sizeof(line), fp) != NULL) {
                if (strstr(line, "Block devices:") != NULL) {
                    printf("  %s", line);
                    /* 不设上限：块设备段很短，且上限会造成「静默截断」 */
                    while (fgets(line, sizeof(line), fp) != NULL) {
                        printf("  %s", line);
                        n++;
                    }
                    break;
                }
            }
            fclose(fp);
            printf("  共 %d 个已注册的块设备主设备号\n", n);
        }
    }

    /* ---------- ⑥ 为什么没有 /sys 可看 ---------- */
    sec("⑥ /sys/dev/block 为什么不存在");
    {
        struct stat st;
        if (stat("/sys", &st) == -1)
            printf("  stat(/sys) errno=%d (%s)\n", errno, strerror(errno));
        else
            printf("  /sys 存在，st_dev=%u:%u\n", major(st.st_dev), minor(st.st_dev));

        if (stat("/sys/dev", &st) == -1)
            printf("  stat(/sys/dev) -> errno=%d (%s)  ← 没有挂 sysfs，/sys 只是个空目录\n",
                   errno, strerror(errno));
        else
            printf("  /sys/dev 存在\n");

        if (stat("/sys/block", &st) == -1)
            printf("  stat(/sys/block) -> errno=%d (%s)\n", errno, strerror(errno));
        else
            printf("  /sys/block 存在\n");
    }
    printf("  ↑ 路径「存在」不等于「那个文件系统已挂载」——\n"
           "    /sys 目录在，但 sysfs 没挂上去，于是 /sys/dev/block 这条路走不通。\n"
           "    判据只能是 /proc/self/mountinfo，不是目录名。\n");

    printf("\n=== c14_2 done ===\n");
    return 0;
}
