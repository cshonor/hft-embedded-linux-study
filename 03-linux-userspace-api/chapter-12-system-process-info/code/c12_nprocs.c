/* c12_nprocs.c — Ch12 延伸：CPU 个数有几条路，各走哪条路
 *
 * ⚠️ 原书 Ch12 没有这一节（正文只有 §12.1 /proc 与 §12.2 uname）。本文件属于
 *    本笔记的延伸，对照 `man 3 get_nprocs` / `man 3 sysconf` 与 glibc 源码。
 *
 * 讲的是一件事：**「有几个 CPU」这个问题有四个入口，但底层只有
 * 三条数据来源，而且其中两条可能不存在**。本程序把四条路全打出来并互相
 * 对账，还顺带钉住一个实战大坑：这些函数失败时会**留下脏 errno**。
 *
 * 四条入口（glibc 源码坐标见文件尾）：
 *   get_nprocs()                   → __get_nprocs()
 *   get_nprocs_conf()              → __get_nprocs_conf()
 *   sysconf(_SC_NPROCESSORS_ONLN)  → 就是 __get_nprocs()      （同一个函数！）
 *   sysconf(_SC_NPROCESSORS_CONF)  → 就是 __get_nprocs_conf() （同一个函数！）
 *
 * 三条底层来源（按 __get_nprocs() 的尝试顺序）：
 *   1) /sys/devices/system/cpu/online        ← sysfs，不是所有环境都挂
 *   2) /proc/stat 里 "cpuN" 行数              ← 退路
 *   3) sched_getaffinity(0) 的 mask 位数     ← 再退一步
 *   （__get_nprocs_conf() 的第 1 条换成 /sys/devices/system/cpu/possible）
 *
 * 编译： gcc -O0 -Wall -Wextra -o c12_nprocs c12_nprocs.c
 * 取材： man-pages 6.19 get_nprocs(3)（"returns the number of processors currently
 *         online" / conf 版是 "configured"）
 *       Linux v6.6 fs/proc/stat.c（show_stat：只输出 online CPU 的 cpuN 行）
 *       glibc 2.39 sysdeps/unix/sysv/linux/getsysstats.c:214-222（__get_nprocs）
 *                      sysdeps/unix/sysv/linux/getsysstats.c:229-237（__get_nprocs_conf）
 *                      sysdeps/unix/sysv/linux/getsysstats.c:109-135（get_nproc_stat）
 *                      sysdeps/unix/sysv/linux/getsysstats.c:137-190（read_sysfs_file
 *                        —— 解析 "0-3,8" 这种范围的语法就在这儿）
 *                      sysdeps/unix/sysv/linux/getsysstats.c:192-211（fallback 链）
 *                      sysdeps/posix/sysconf.c:629-639（sysconf 只是转发）
 * 实测：本 CE 沙箱里 /sys/devices/system/cpu 整棵**不存在**，于是四条入口全部
 *       退到 /proc/stat，返回 2，并在 errno 里留下 ENOENT(2)。
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>

/* 探测一个路径是文件还是目录还是不存在 */
static const char *what(const char *path)
{
    FILE *fp;
    char b[256];

    fp = fopen(path, "r");
    if (fp != NULL) {
        int got = (fgets(b, sizeof b, fp) != NULL);
        fclose(fp);
        static char out[300];
        char *nl;
        if (!got)
            snprintf(out, sizeof out, "存在但读不到内容");
        else {
            nl = strchr(b, '\n');
            if (nl)
                *nl = '\0';
            snprintf(out, sizeof out, "存在，内容 = [%s]", b);
        }
        return out;
    }
    /* 打不开：分不清「不存在」和「是目录」时才再看一眼 errno */
    if (errno == ENOENT)
        return "不存在 (ENOENT)";
    if (errno == EISDIR)
        return "是目录";
    return "打不开（其它 errno）";
}

/* 自己数 /proc/stat 里 "cpuN" 的条数 */
static int count_proc_stat_cpus(void)
{
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL)
        return -1;
    char line[512];
    int n = 0;
    while (fgets(line, sizeof line, fp)) {
        if (strncmp(line, "cpu", 3) != 0)
            break;                          /* cpuN 行都在最前面，见 show_stat() */
        if (line[3] >= '0' && line[3] <= '9')
            n++;
    }
    fclose(fp);
    return n;
}

int main(void)
{
    /* ---------- ① 四条入口 ---------- */
    printf("== ① 四个入口的返回值 ==\n");
    /* ⚠️ 关键写法：调用与读 errno **必须分成两条语句**。
       printf("...%d...%d", f(), errno) 里参数求值顺序未定义，
       errno 很可能在 f() 之前就被读走 —— 那样量到的是「上一次的脏值」。*/
    errno = 0;
    int a = get_nprocs();
    int ea = errno;

    errno = 0;
    int b = get_nprocs_conf();
    int eb = errno;

    errno = 0;
    long c = sysconf(_SC_NPROCESSORS_ONLN);
    int ec = errno;

    errno = 0;
    long d = sysconf(_SC_NPROCESSORS_CONF);
    int ed = errno;

    printf("  get_nprocs()                  = %d   errno=%d(%s)\n", a, ea, strerror(ea));
    printf("  get_nprocs_conf()             = %d   errno=%d(%s)\n", b, eb, strerror(eb));
    printf("  sysconf(_SC_NPROCESSORS_ONLN) = %ld   errno=%d(%s)\n", c, ec, strerror(ec));
    printf("  sysconf(_SC_NPROCESSORS_CONF) = %ld   errno=%d(%s)\n", d, ed, strerror(ed));
    printf("  → 两条 sysconf 就是转发到两个 get_nprocs*，值当然一样。\n");
    printf("  ⚠️ 注意 errno 全都不是 0 —— 但**调用是成功的**。见第 ⑤ 段。\n\n");

    /* ---------- ② 底层第 1 条路：sysfs ---------- */
    printf("== ② 底层第 1 条路：/sys/devices/system/cpu/* ==\n");
    printf("  online                     : %s\n", what("/sys/devices/system/cpu/online"));
    printf("  possible                   : %s\n", what("/sys/devices/system/cpu/possible"));
    errno = 0;
    printf("  present                    : %s\n", what("/sys/devices/system/cpu/present"));
    printf("  （__get_nprocs 读 online，__get_nprocs_conf 读 possible）\n");
    {
        DIR *dir = opendir("/sys/devices/system/cpu");
        printf("  opendir(/sys/devices/system/cpu) → %s\n", dir ? "成功" : "失败（整棵不存在）");
        if (dir)
            closedir(dir);
    }
    printf("  ⚠️ 这条路的格式是**逗号分隔的范围**，例如 \"0-3,8,10-11\"；\n");
    printf("     glibc 的 read_sysfs_file() 里那段 strtoul + '-' 解析就是为它写的。\n");
    printf("     直接用 atoi() 读这个文件，遇到 \"0-3,8\" 会得到 0 —— 很常见的低级错误。\n\n");

    /* ---------- ③ 底层第 2 条路：/proc/stat ---------- */
    printf("== ③ 底层第 2 条路：数 /proc/stat 的 cpuN 行 ==\n");
    {
        int n = count_proc_stat_cpus();
        printf("  自己数出来的 cpuN 行数 = %d\n", n);
        printf("  get_nprocs() = %d  → %s\n", a, (n == a) ? "一致" : "不一致（可能中途 CPU 上下线）");
        printf("  /proc/stat 的头两行：\n");
        FILE *fp = fopen("/proc/stat", "r");
        if (fp) {
            char line[512];
            for (int i = 0; i < 3 && fgets(line, sizeof line, fp); i++) {
                char *nl = strchr(line, '\n');
                if (nl)
                    *nl = '\0';
                printf("    [%d] %s\n", i, line);
            }
            fclose(fp);
        }
        printf("  ⚠️ 第一行是汇总行 \"cpu \"（cpu 后面是空格，不是数字），**不算一个 CPU**；\n");
        printf("     这也是 glibc 要判 isdigit(l[3]) 的原因。\n\n");
    }

    /* ---------- ④ 底层第 3 条路：sched_getaffinity ---------- */
    printf("== ④ 底层第 3 条路：sched_getaffinity(0) 的 mask ==\n");
    {
        cpu_set_t set;
        CPU_ZERO(&set);
        errno = 0;
        int r = sched_getaffinity(0, sizeof(set), &set);
        int e = errno;
        if (r == -1) {
            printf("  sched_getaffinity 失败 errno=%d(%s)\n", e, strerror(e));
        } else {
            printf("  sched_getaffinity(0) = %d   CPU_COUNT = %d   errno=%d\n",
                   r, CPU_COUNT(&set), e);
            printf("  允许的 CPU 列表 = ");
            for (int i = 0; i < CPU_SETSIZE; i++)
                if (CPU_ISSET(i, &set))
                    printf("%d ", i);
            printf("\n");
            printf("  ⚠️ 这条路是**进程视角**的可用 CPU（受 cgroup cpuset 与 taskset 限制），\n");
            printf("     和「机器有几个核」不是一回事 —— 同一台机上不同进程可能拿到不同答案。\n");
            printf("     绑核的性能测试里要报的是这个数，不是 get_nprocs()。\n");
        }
        printf("\n");
    }

    /* ---------- ⑤ 脏 errno ---------- */
    printf("== ⑤ 脏 errno：调用成功不代表 errno 干净 ==\n");
    printf("  errno 的规则是「只在出错时有意义、成功时**不保证被清**」。\n");
    printf("  这里的脏值正是第 ② 段那次 open(\"/sys/devices/system/cpu/online\") 失败留下的：\n");
    printf("  getsysstats.c:216 `read_sysfs_file(\"/sys/devices/system/cpu/online\")`\n");
    printf("     → :148 `__open_nocancel(fname, O_RDONLY|O_CLOEXEC)` 返回 -1，errno = ENOENT\n");
    printf("     → :151 `if (fd != -1)` 不成立，函数直接 `return 0`（**不清 errno**）\n");
    printf("     → :217 `if (result != 0)` 不成立，继续 :221 `get_nprocs_fallback()`\n");
    printf("     → :198 `get_nproc_stat()` 成功返回 2 → 函数返回 2，errno 仍是 ENOENT\n");
    printf("  ⚠️ 所以 get_nprocs() 的**返回值是对的**（2），**errno 是脏的**（ENOENT）。\n");
    printf("     如果把 errno 当判据：`if (get_nprocs() < 0 || errno) 报警` → 永远误报。\n");
    printf("     正确写法就是本程序第 ① 段那样：先 errno=0，调用，**立即**取 errno 判断。\n\n");

    /* ---------- ⑥ 容器 / cgroup 的坑 ---------- */
    printf("== ⑥ 容器里 get_nprocs() 可能报的是**宿主机**核数 ==\n");
    printf("  看 glibc 那三条来源：/sys/devices/system/cpu/online、/proc/stat、\n");
    printf("  sched_getaffinity —— **没有一条读 cgroup 的 CPU 配额**（cgroup v2 的\n");
    printf("  /sys/fs/cgroup/cpu.max 或 v1 的 cpu.cfs_quota_us）。\n");
    printf("  所以 `docker run --cpus=2` 的容器里 get_nprocs() 完全可能返回宿主的 64。\n");
    printf("  要拿真实可用并行度得自己读 cgroup，或直接看 sched_getaffinity。\n");
    printf("  本沙箱里相关文件：\n");
    printf("    /sys/fs/cgroup/cpu.max          : %s\n", what("/sys/fs/cgroup/cpu.max"));
    printf("    /sys/fs/cgroup/cpu.max.burst    : %s\n", what("/sys/fs/cgroup/cpu.max.burst"));
    printf("\n");

    /* ---------- ⑦ 三源合账 ---------- */
    printf("== ⑦ 合账 ==\n");
    printf("  %-32s %s\n", "入口 / 来源", "值");
    printf("  %-32s %d\n", "get_nprocs()", a);
    printf("  %-32s %d\n", "get_nprocs_conf()", b);
    printf("  %-32s %d\n", "sysconf(_SC_NPROCESSORS_ONLN)", (int) c);
    printf("  %-32s %d\n", "sysconf(_SC_NPROCESSORS_CONF)", (int) d);
    printf("  %-32s %d\n", "数 /proc/stat 的 cpuN", count_proc_stat_cpus());
    {
        cpu_set_t set;
        CPU_ZERO(&set);
        if (sched_getaffinity(0, sizeof(set), &set) == 0)
            printf("  %-32s %d\n", "CPU_COUNT(sched_getaffinity)", CPU_COUNT(&set));
    }
    printf("  ⚠️ 六个数不代表「六个独立来源」—— 前四个只有两条底层实现，\n");
    printf("     第 5 条是它们的退路，第 6 条才是独立事实。做容量规划时别混着用。\n");
    return EXIT_SUCCESS;
}
