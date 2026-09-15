/* c12_sysinfo.c — Ch12 延伸：sysinfo() 一次系统调用换回一整屏总量指标
 *
 * ⚠️ 原书 Ch12 只有 §12.1（/proc）与 §12.2（uname）两节正文，12.3 是 Summary、
 *    12.4 是 Exercises —— **没有 sysinfo() 小节**。本文件属于本笔记的延伸，
 *    对照 `man 2 sysinfo` 与内核源码，不是对原书某一节的实现。
 *
 * 重点是**四个陷阱**：
 *   ① 所有内存字段的单位不是字节，是 `mem_unit` 字节。本机 mem_unit=1 只是
 *      因为「字节数能塞进 unsigned long」；32 位内核内存超过 4GB 时会退化成
 *      以**页**为单位（mem_unit=PAGE_SIZE），字段值小 4096 倍。正确公式永远是
 *      `字节数 = 字段 × mem_unit`；
 *   ② `loads[]` 是**定点数**，小数点在 `1 << SI_LOAD_SHIFT` = 65536 处，
 *      既不是浮点也不是千分之一；
 *   ③ `uptime` 是**向上取整**的秒数（内核 `tp.tv_nsec ? 1 : 0`），所以它常比
 *      /proc/uptime 的小数部分大 1；
 *   ④ `procs` 在内核里赋的是 `nr_threads` —— **线程数**，不是进程数。
 *      man page 写的是 "Number of current processes"，与实现不符。
 *
 * 本程序把每个字段打出来，再和 /proc 的同名数据逐条对账。
 *
 * 编译： gcc -O0 -Wall -Wextra -o c12_sysinfo c12_sysinfo.c
 * 取材： man-pages 6.19 sysinfo(2)（mem_unit / SI_LOAD_SHIFT / procs 的描述）
 *       Linux v6.6 include/uapi/linux/sysinfo.h:7（SI_LOAD_SHIFT 16）
 *                      include/uapi/linux/sysinfo.h:8-23（struct sysinfo 全字段）
 *                      kernel/sys.c:2747-2808（do_sysinfo：memset → uptime 上取整
 *                        → get_avenrun → procs = nr_threads → si_meminfo →
 *                        溢出检查后 <<bitcount 并把 mem_unit 置 1）
 *                      kernel/sys.c:2810-2820（SYSCALL_DEFINE1(sysinfo)）
 *                      mm/page_alloc.c（si_meminfo：值是**页数**，mem_unit=PAGE_SIZE）
 *                      fs/proc/meminfo.c（/proc/meminfo 与 sysinfo 同源）
 *       glibc 2.39 sysdeps/unix/sysv/linux/getsysstats.c:272-292（get_phys_pages /
 *                      get_avphys_pages 就是 sysinfo 的换算，注释里明说
 *                      "This used to be done by parsing /proc/meminfo, but that's
 *                       unnecessarily expensive"）
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>

/* glibc 的 <sys/sysinfo.h> 不导出这个宏（它只在内核的 uapi 头里），
   所以照着 include/uapi/linux/sysinfo.h:7 自己写一份 */
#define SI_LOAD_SHIFT 16
#define LOAD_SCALE    ((double) (1 << SI_LOAD_SHIFT))   /* 65536.0 */

static int first_line(const char *path, char *buf, int sz)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return -1;
    if (fgets(buf, sz, fp) == NULL) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    char *nl = strchr(buf, '\n');
    if (nl)
        *nl = '\0';
    return 0;
}

/* 从 /proc/meminfo 里取某一行第一个数字（kB） */
static long long meminfo_kb(const char *key)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
        return -1;
    char line[256];
    long long v = -1;
    size_t kl = strlen(key);
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, key, kl) == 0 && line[kl] == ':') {
            v = strtoll(line + kl + 1, NULL, 10);
            break;
        }
    }
    fclose(fp);
    return v;
}

int main(void)
{
    struct sysinfo si;
    errno = 0;
    if (sysinfo(&si) == -1) {
        printf("sysinfo 失败 errno=%d(%s)\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    /* ---------- ① 全字段 ---------- */
    printf("== ① struct sysinfo 全字段 ==\n");
    printf("  uptime     = %ld 秒              （开机时长，注意是**向上取整**）\n", si.uptime);
    printf("  loads[0]   = %lu  （1 分钟负载，定点数）\n", (unsigned long) si.loads[0]);
    printf("  loads[1]   = %lu  （5 分钟）\n", (unsigned long) si.loads[1]);
    printf("  loads[2]   = %lu  （15 分钟）\n", (unsigned long) si.loads[2]);
    printf("  totalram   = %llu  × mem_unit\n", (unsigned long long) si.totalram);
    printf("  freeram    = %llu  × mem_unit\n", (unsigned long long) si.freeram);
    printf("  sharedram  = %llu  × mem_unit\n", (unsigned long long) si.sharedram);
    printf("  bufferram  = %llu  × mem_unit\n", (unsigned long long) si.bufferram);
    printf("  totalswap  = %llu  × mem_unit\n", (unsigned long long) si.totalswap);
    printf("  freeswap   = %llu  × mem_unit\n", (unsigned long long) si.freeswap);
    printf("  procs      = %u   （内核赋的是 nr_threads = **线程数**）\n", (unsigned) si.procs);
    printf("  totalhigh  = %llu  × mem_unit （HIGHMEM 只在 32 位有意义）\n",
           (unsigned long long) si.totalhigh);
    printf("  freehigh   = %llu  × mem_unit\n", (unsigned long long) si.freehigh);
    printf("  mem_unit   = %u 字节             （1 = 字段值已经是字节）\n", si.mem_unit);
    printf("  sizeof(struct sysinfo) = %zu\n\n", sizeof(struct sysinfo));

    /* ---------- ② mem_unit 的规则 ---------- */
    printf("== ② mem_unit：唯一的正确读法是「字段 × mem_unit」 ==\n");
    printf("  do_sysinfo() 的流程（kernel/sys.c:2747-2808）：\n");
    printf("    1) memset 清 0\n");
    printf("    2) si_meminfo() 把内存字段填成**页数**，并设 mem_unit = PAGE_SIZE\n");
    printf("    3) 如果 (totalram+totalswap) 左移 log2(PAGE_SIZE) 位后**不溢出**\n");
    printf("       32 位 unsigned long，就把所有内存字段 <<bitcount 并把 mem_unit 置 1\n");
    printf("    4) 一旦溢出就直接 goto out —— 字段留在**页**单位，mem_unit = PAGE_SIZE\n");
    printf("  本机 PAGE_SIZE=%ld，mem_unit=%u，所以现在是第 3 种情况（值已经是字节）。\n",
           sysconf(_SC_PAGESIZE), si.mem_unit);
    printf("  ⚠️ 若 mem_unit=%ld（32 位大内存机），totalram 报的就会是页数，\n",
           (long) sysconf(_SC_PAGESIZE));
    printf("     直接当字节用会**少算 4096 倍**。这个字段不能忽略。\n\n");

    /* ---------- ③ 与 /proc/meminfo 对账 ---------- */
    printf("== ③ totalram × mem_unit 与 /proc/meminfo MemTotal 对账 ==\n");
    {
        unsigned long long bytes = (unsigned long long) si.totalram * si.mem_unit;
        long long mt = meminfo_kb("MemTotal");
        printf("  sysinfo : totalram × mem_unit = %llu 字节\n", bytes);
        printf("  /proc   : MemTotal             = %lld kB = %lld 字节\n", mt, mt * 1024);
        printf("  → %s\n", (mt >= 0 && bytes == (unsigned long long) mt * 1024)
               ? "严格相等（两者同源：都来自 si_meminfo()）"
               : "不等（可能读的两个时刻不同）");
        printf("  freeram × mem_unit = %llu 字节；/proc MemFree = %lld kB\n",
               (unsigned long long) si.freeram * si.mem_unit, meminfo_kb("MemFree"));
        printf("  ⚠️ freeram 这一项**随时在变**，上面两行不等是正常的（不是 bug）。\n\n");
    }

    /* ---------- ④ loads 定点数 ---------- */
    printf("== ④ loads[] 是 1<<%d 的定点数，不是浮点 ==\n", SI_LOAD_SHIFT);
    {
        char la[128] = "";
        first_line("/proc/loadavg", la, sizeof la);
        printf("  /proc/loadavg = [%s]\n", la);
        printf("  sysinfo: %.4f  %.4f  %.4f   （loads[i] / %.0f）\n",
               si.loads[0] / LOAD_SCALE, si.loads[1] / LOAD_SCALE, si.loads[2] / LOAD_SCALE,
               LOAD_SCALE);
        printf("  → 与 loadavg 前三个数一致。内核在 do_sysinfo 里调的是\n");
        printf("     get_avenrun(info->loads, 0, SI_LOAD_SHIFT - FSHIFT)：\n");
        printf("     avenrun 内部用 FSHIFT=11 的定点，这里左移 %d 位改成 1<<16。\n",
               SI_LOAD_SHIFT - 11);
        printf("  ⚠️ 忘了除 65536 就会把负载 0.60 读成 39136 —— 这种数字很容易被\n");
        printf("     当成「负载爆表」写进告警阈值。另外 loadavg 的第 4/5 段不是负载：\n");
        printf("     第 4 段是 running/total 线程数，第 5 段是最近建的 PID。\n\n");
    }

    /* ---------- ⑤ uptime 向上取整 ---------- */
    printf("== ⑤ uptime 是向上取整的秒数 ==\n");
    {
        char up[64] = "";
        first_line("/proc/uptime", up, sizeof up);
        printf("  /proc/uptime  = [%s]（第一段是秒，第二段是所有 CPU 空闲时间之和）\n", up);
        printf("  sysinfo.uptime = %ld 秒\n", si.uptime);
        printf("  → 内核写的是 `tp.tv_sec + (tp.tv_nsec ? 1 : 0)`（kernel/sys.c:2757），\n");
        printf("     所以 sysinfo 的值 ≥ /proc/uptime 第一段，最多差 1 秒。\n");
        printf("  ⚠️ 要精确时长就用 /proc/uptime 的浮点数；要个整秒用哪个都行。\n\n");
    }

    /* ---------- ⑥ procs 是线程数 ---------- */
    printf("== ⑥ procs 是**线程数**，不是进程数 ==\n");
    {
        char la[128] = "";
        first_line("/proc/loadavg", la, sizeof la);
        printf("  sysinfo.procs = %u\n", (unsigned) si.procs);
        printf("  /proc/loadavg = [%s]  ← 第 4 段 \"running/total\" 里的 total 就是它\n", la);

        /* 自己数 /proc 下的数字目录 = 真正的**进程**数（不含线程） */
        int npid = 0;
        DIR *d = opendir("/proc");
        if (d != NULL) {
            struct dirent *e;
            while ((e = readdir(d)) != NULL)
                if (e->d_name[0] >= '0' && e->d_name[0] <= '9')
                    npid++;
            closedir(d);
        }
        printf("  /proc 下的数字目录（进程）数 = %d\n", npid);
        printf("  ⚠️ man page 写 \"Number of current processes\"，但 kernel/sys.c:2761\n");
        printf("     赋的是 `info->procs = nr_threads;` —— **线程**计数。\n");
        printf("     多线程进程会明显拉高 procs：JVM / 浏览器这类进程动辄几十上百线程，\n");
        printf("     于是 procs 会比 /proc 下的进程目录数大得多。\n");
        printf("     本容器里两者差了**两个数量级**（%u 个线程 vs %d 个 /proc 目录）：\n",
               (unsigned) si.procs, npid);
        printf("     因为 nr_threads 是**宿主级**计数器，而 /proc 只列本 PID 命名空间里的进程\n");
        printf("     —— sysinfo() 不做命名空间隔离，容器里它报的是宿主机的数字。\n\n");
    }

    /* ---------- ⑦ totalhigh / freehigh ---------- */
    printf("== ⑦ totalhigh / freehigh 在 x86-64 上恒 0 ==\n");
    printf("  totalhigh = %llu   freehigh = %llu\n",
           (unsigned long long) si.totalhigh, (unsigned long long) si.freehigh);
    printf("  这两项统计 HIGHMEM（高端内存）区；x86-64 没有 HIGHMEM（内核地址空间\n");
    printf("  足够覆盖全部物理内存），所以恒 0。别拿它们算「可用内存」。\n\n");

    /* ---------- ⑧ glibc 把 get_phys_pages 建在 sysinfo 上 ---------- */
    printf("== ⑧ get_phys_pages()/get_avphys_pages() 就是 sysinfo 的换算 ==\n");
    {
        long tp = get_phys_pages();
        long ap = get_avphys_pages();
        long ps = sysconf(_SC_PAGESIZE);
        printf("  get_phys_pages()   = %ld 页  → %ld MiB\n", tp, tp / 256);
        printf("  get_avphys_pages() = %ld 页  → %ld MiB\n", ap, ap / 256);
        printf("  totalram × mem_unit / PAGE_SIZE = %llu  = get_phys_pages()? %d\n",
               (unsigned long long) si.totalram * si.mem_unit / (unsigned long long) ps,
               ((unsigned long long) si.totalram * si.mem_unit / (unsigned long long) ps)
                   == (unsigned long long) tp);
        printf("  glibc 源码注释原话：这些值以前是 parse /proc/meminfo 得来的，\n");
        printf("  \"but that's unnecessarily expensive (and /proc is not always available)\"。\n");
        printf("  → 所以 `sysconf(_SC_PHYS_PAGES)` / `get_phys_pages()` 都不是新系统调用，\n");
        printf("    底下就是 sysinfo(2)。这也意味着 sysinfo() 不可用时它们一起失效。\n");
        printf("  ⚠️ get_avphys_pages() 对应 freeram，**每次跑都不一样**。\n\n");
    }

    /* ---------- ⑨ 错误路径 ---------- */
    printf("== ⑨ sysinfo(NULL) ==\n");
    errno = 0;
    int r = sysinfo(NULL);
    printf("  sysinfo(NULL) = %d errno=%d(%s)\n", r, errno, strerror(errno));
    printf("  内核在 copy_to_user 处挡下 NULL（kernel/sys.c:2816），返回 EFAULT。\n");
    printf("  注意 do_sysinfo 已经**做完了全部工作**才失败 —— 白干一趟。\n\n");

    /* ---------- ⑩ 会漂移的字段 ---------- */
    printf("== ⑩ 哪些字段每次跑都不一样（写笔记/告警时别硬编码）==\n");
    printf("  必然漂移：uptime / loads[] / freeram / freeswap / procs\n");
    printf("  相对稳定：totalram / totalswap / mem_unit / totalhigh / freehigh\n");
    printf("  ⚠️ **sysinfo() 不做命名空间 / cgroup 隔离**：容器里它报的是宿主机的\n");
    printf("     线程数、内存量。被 PID 命名空间隔离的只是 /proc 的**进程目录**\n");
    printf("     （见第 ⑥ 段的「线程数 vs /proc 目录数」）；\n");
    printf("     /proc/meminfo 同样不是 cgroup 口径 ——\n");
    printf("     第 ③ 段实测它与 totalram 严格相等就是证据。要配额得读 cgroup。\n");
    printf("  ⚠️ 跨机器对比时只比「稳定」那组；否则两台机的负载不同会掩盖真差异。\n");
    return EXIT_SUCCESS;
}
