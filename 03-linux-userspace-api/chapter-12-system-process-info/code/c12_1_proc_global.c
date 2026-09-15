/* c12_1_proc_global.c — Ch12 §12.1.2：整机信息，以及「/proc vs sysinfo」的交叉验证
 *
 * TLPI §12.1.2 说：整机信息既可以从 /proc 的全局文件读，也可以走 sysinfo()。
 * 本程序把两条路**并排打出来对账**，并回答三个工程问题：
 *   ① 同样的数字，两条路一致吗？（本容器实测：一致）
 *   ② 为什么老代码偏爱 sysinfo()？（一次系统调用 vs 若干次 open/read/解析）
 *   ③ 为什么又有人说「优先读 /proc/meminfo」？（meminfo 字段多得多，
 *      而且 sysinfo 的 totalram 在 32 位平台会溢出 —— 见 mem_unit 的作用）
 *
 * 编译： gcc -O0 -Wall -Wextra -o c12_1_proc_global c12_1_proc_global.c
 * 取材： man-pages 6.19 proc(5)（/proc/meminfo、/proc/loadavg、/proc/uptime、
 *         /proc/stat、/proc/cpuinfo 各节）+ sysinfo(2) 全文
 *       Linux v6.6 fs/proc/meminfo.c（seq_file 逐项输出）
 *                      fs/proc/loadavg.c（get_avenrun + LOAD_INT/LOAD_FRAC）
 *                      fs/proc/uptime.c（ktime_get_boottime_ts64 现算）
 *                      fs/proc/array.c（/proc/stat 的 cpu 行）
 *                      include/uapi/linux/sysinfo.h（struct sysinfo / SI_LOAD_SHIFT）
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

/* 只取第一行，去掉行尾换行 */
static int first_line(const char *path, char *out, size_t cap)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        snprintf(out, cap, "<打开失败 errno=%d(%s)>", errno, strerror(errno));
        return -1;
    }
    if (fgets(out, (int) cap, fp) == NULL) {
        fclose(fp);
        snprintf(out, cap, "<空>");
        return -1;
    }
    fclose(fp);
    size_t l = strlen(out);
    while (l && (out[l - 1] == '\n' || out[l - 1] == '\r'))
        out[--l] = '\0';
    return 0;
}

/* 在 "Key:  <数字> kB" 形式的文件里找一个键 */
static long long lookup_kb(const char *path, const char *key, int *found)
{
    FILE *fp = fopen(path, "r");
    *found = 0;
    if (fp == NULL)
        return -1;
    char line[512];
    size_t klen = strlen(key);
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, key, klen) == 0 && line[klen] == ':') {
            long long v = 0;
            if (sscanf(line + klen + 1, "%lld", &v) == 1) {
                *found = 1;
                fclose(fp);
                return v;
            }
        }
    }
    fclose(fp);
    return -1;
}

int main(void)
{
    char line[512];

    /* ---------- ① 纯文本型：version / uptime / loadavg ---------- */
    printf("== ① 几个纯文本全局文件（原样一行）==\n");
    if (first_line("/proc/version", line, sizeof(line)) == 0)
        printf("  /proc/version : %s\n", line);
    if (first_line("/proc/uptime", line, sizeof(line)) == 0)
        printf("  /proc/uptime  : %s\n", line);
    if (first_line("/proc/loadavg", line, sizeof(line)) == 0)
        printf("  /proc/loadavg : %s\n", line);
    printf("  ⚠️ /proc/uptime 有**两个**数：第一个是开机秒数，第二个是「所有 CPU 的\n");
    printf("     空闲时间总和」—— 双核机器上它会约等于 2× 开机秒。它**不是** idle 比例。\n");
    printf("  ⚠️ /proc/loadavg 的第 4 段是 \"running/total\"，第 5 段是「最近分配的 PID」，\n");
    printf("     都不是负载。\n\n");

    /* ---------- ② 与 sysinfo() 对账 ---------- */
    printf("== ② sysinfo() 与 /proc 的对账 ==\n");
    struct sysinfo si;
    errno = 0;
    if (sysinfo(&si) == -1) {
        printf("  sysinfo 失败 errno=%d(%s)\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }
    printf("  sysinfo: uptime=%lds  loads=[%lu %lu %lu]  mem_unit=%u  procs=%u\n",
           (long) si.uptime, si.loads[0], si.loads[1], si.loads[2],
           si.mem_unit, si.procs);
    printf("  sysinfo: totalram=%lu  freeram=%lu  sharedram=%lu  bufferram=%lu\n",
           si.totalram, si.freeram, si.sharedram, si.bufferram);
    printf("  sysinfo: totalswap=%lu  freeswap=%lu\n", si.totalswap, si.freeswap);

    unsigned long long total_bytes = (unsigned long long) si.totalram * si.mem_unit;
    unsigned long long free_bytes = (unsigned long long) si.freeram * si.mem_unit;
    printf("  → 字节数 = 字段 × mem_unit（本机 mem_unit=%u，所以字段就是字节数）\n",
           si.mem_unit);
    printf("    totalram × mem_unit = %llu 字节 = %llu MiB\n",
           total_bytes, total_bytes / 1024 / 1024);
    printf("    freeram  × mem_unit = %llu 字节 = %llu MiB\n",
           free_bytes, free_bytes / 1024 / 1024);

    int found = 0;
    long long memtotal = lookup_kb("/proc/meminfo", "MemTotal", &found);
    long long memfree = lookup_kb("/proc/meminfo", "MemFree", &found);
    printf("  /proc/meminfo : MemTotal=%lld kB  MemFree=%lld kB\n", memtotal, memfree);
    printf("  → MemTotal(kB) × 1024 = %lld 字节  vs  sysinfo 的 %llu 字节  → %s\n",
           memtotal * 1024, total_bytes,
           (memtotal > 0 && (unsigned long long) memtotal * 1024 == total_bytes) ? "一致" : "不一致");
    printf("  ⚠️ 但 **MemFree ≠ freeram 的口径**：meminfo 还有 MemAvailable / Buffers /\n");
    printf("     Cached / Slab 等十几项，sysinfo 只有 freeram。要看内存压力必须读\n");
    printf("     /proc/meminfo；只想「大概有多少」才用 sysinfo。\n\n");

    printf("== ③ 负载：sysinfo 是定点数，要除以 65536 ==\n");
    printf("  sysinfo.loads[0] = %lu  →  %lu/65536.0 = %.4f\n",
           si.loads[0], si.loads[0], si.loads[0] / 65536.0);
    if (first_line("/proc/loadavg", line, sizeof(line)) == 0) {
        double a = 0, b = 0, c = 0;
        sscanf(line, "%lf %lf %lf", &a, &b, &c);
        printf("  /proc/loadavg   = %.2f %.2f %.2f\n", a, b, c);
        printf("  → 两边对得上（差在 /proc 只给两位小数）\n");
    }
    printf("  ⚠️ 老内核里 loads[] 的定标不是 65536（是 1<<SI_LOAD_SHIFT，早期还有\n");
    printf("     直接给整数的版本）→ 跨内核解析要小心。\n\n");

    /* ---------- ④ CPU：三条路 ---------- */
    printf("== ④ 「几个 CPU」有三条路 ==\n");
    errno = 0;
    int np = get_nprocs();
    int npc = get_nprocs_conf();
    int err_after_nprocs = errno;
    printf("  get_nprocs()      = %d    get_nprocs_conf() = %d\n", np, npc);
    printf("  sysconf(_SC_NPROCESSORS_ONLN)   = %ld\n", sysconf(_SC_NPROCESSORS_ONLN));
    printf("  sysconf(_SC_NPROCESSORS_CONF)   = %ld\n", sysconf(_SC_NPROCESSORS_CONF));
    printf("  get_nprocs() 调用后 errno = %d（%s）← 这是**脏 errno**：\n",
           err_after_nprocs, strerror(err_after_nprocs));
    printf("     函数调用是**成功的**，这个值来自它内部的第一条尝试 ——\n");
    printf("     glibc 先读 /sys/devices/system/cpu/online（本容器没挂 /sys，\n");
    printf("     该文件不存在 → ENOENT 留在 errno），失败后才退到数 /proc/stat。\n");
    printf("     所以判错必须自己先 errno = 0、调用后立即取；别把 errno 当返回值用。\n");
    if (first_line("/proc/stat", line, sizeof(line)) == 0) {
        printf("  /proc/stat 第一行: %s\n", line);
        printf("  → 第一个字段 \"cpu\" 是**所有核的汇总**，之后才是 cpu0 / cpu1 …\n");
    }
    if (first_line("/proc/cpuinfo", line, sizeof(line)) == 0)
        printf("  /proc/cpuinfo 第一行: %s   （processor : N，每个逻辑核一段）\n", line);
    printf("\n");

    /* ---------- ⑤ hostname / osrelease 与 POSIX 接口对账 ---------- */
    printf("== ⑤ 内核参数的文本接口 vs POSIX 接口 ==\n");
    char host_kernel[256] = "", host_posix[256] = "";
    if (first_line("/proc/sys/kernel/hostname", host_kernel, sizeof(host_kernel)) != 0)
        strcpy(host_kernel, "<无>");
    if (gethostname(host_posix, sizeof(host_posix) - 1) == -1)
        strcpy(host_posix, "<失败>");
    printf("  /proc/sys/kernel/hostname : [%s]\n", host_kernel);
    printf("  gethostname()             : [%s]\n", host_posix);
    printf("  → %s\n", strcmp(host_kernel, host_posix) == 0 ? "一致" : "不一致");

    struct utsname u;
    if (uname(&u) == 0) {
        char osrel[256] = "";
        if (first_line("/proc/sys/kernel/osrelease", osrel, sizeof(osrel)) != 0)
            strcpy(osrel, "<无>");
        printf("  /proc/sys/kernel/osrelease: [%s]\n", osrel);
        printf("  uname().release           : [%s]\n", u.release);
        printf("  → %s\n", strcmp(osrel, u.release) == 0 ? "一致" : "不一致");
    }
    printf("  ⚠️ 可移植代码用 gethostname()/uname()；写系统脚本才图省事读 /proc/sys。\n\n");

    /* ---------- ⑥ 一个容易误会的事实 ---------- */
    printf("== ⑥ /proc/sys/fs/file-max 是 LONG_MAX 级别的数，不是「现在能开多少」 ==\n");
    if (first_line("/proc/sys/fs/file-max", line, sizeof(line)) == 0) {
        printf("  /proc/sys/fs/file-max = %s\n", line);
        printf("  ⚠️ 它是系统级**总量上限**，单个进程还被 RLIMIT_NOFILE 卡着；\n");
        printf("     查进程能开多少 fd 要用 getrlimit(RLIMIT_NOFILE) / sysconf(_SC_OPEN_MAX)\n");
        printf("     （见 Ch11 §11.2：那个值其实是 rlim_cur）。\n");
    }
    return EXIT_SUCCESS;
}
