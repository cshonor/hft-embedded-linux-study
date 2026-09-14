/* TLPI 第 2 章 §2.18 —— 实时：调度策略、内存锁定、CPU 亲和，逐项实测
 *
 * 编译：gcc -O0 -Wall -Wextra c2_18_realtime.c -o c2_18
 * 运行：./c2_18
 *
 * 本节要钉死的事实：
 *   ① 实时性 = 可预测的**最坏**延迟，不是平均快。
 *   ② 三个最常被一起使用的开关：sched_setscheduler（抢占）、mlockall（防缺页）、
 *      sched_setaffinity（防迁移）。它们的权限要求各不相同。
 *   ③ 每一个都有对应的 capability：CAP_SYS_NICE（调度）、CAP_IPC_LOCK（锁内存）。
 *      euid=0 不等于拥有它们 —— 用 /proc/self/status 的 CapEff 验证。
 *   ④ PREEMPT_RT 解决的是「内核自己不可抢占」的问题，是另一层的事。
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>

#define CAP_SYS_NICE_BIT 23           /* include/uapi/linux/capability.h */
#define CAP_IPC_LOCK_BIT 14

static unsigned long long read_hex_line(const char *key)
{
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return 0;
    char line[512];
    unsigned long long v = 0;
    size_t klen = strlen(key);
    while (fgets(line, sizeof(line), f))
        if (strncmp(line, key, klen) == 0) { sscanf(line + klen, "%llx", &v); break; }
    fclose(f);
    return v;
}

static const char *policy_name(int p)
{
    switch (p) {
    case SCHED_OTHER: return "SCHED_OTHER";
    case SCHED_FIFO:  return "SCHED_FIFO";
    case SCHED_RR:    return "SCHED_RR";
    case SCHED_BATCH: return "SCHED_BATCH";
    case SCHED_IDLE:  return "SCHED_IDLE";
    default:          return "?";
    }
}

static void try_sched(int policy, int prio)
{
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = prio;
    errno = 0;
    int rc = sched_setscheduler(0, policy, &sp);
    if (rc == 0)
        printf("  sched_setscheduler(%s, prio=%d) -> OK，当前策略现在是 %s\n",
               policy_name(policy), prio, policy_name(sched_getscheduler(0)));
    else
        printf("  sched_setscheduler(%s, prio=%d) -> 失败 errno=%d (%s)\n",
               policy_name(policy), prio, errno, strerror(errno));
    errno = 0;
}

int main(void)
{
    printf("=== ① 先看家底：有没有那两个 capability ===\n");
    unsigned long long capEff = read_hex_line("CapEff:");
    printf("  CapEff = 0x%016llx\n", capEff);
    printf("  CAP_SYS_NICE (bit %d) = %s\n", CAP_SYS_NICE_BIT,
           (capEff >> CAP_SYS_NICE_BIT) & 1 ? "有" : "没有");
    printf("  CAP_IPC_LOCK (bit %d) = %s\n", CAP_IPC_LOCK_BIT,
           (capEff >> CAP_IPC_LOCK_BIT) & 1 ? "有" : "没有");
    printf("  -> 记住这两个开关的状态，下面每一步的成败都能对上。\n");

    printf("\n=== ② 调度策略：实时策略要 CAP_SYS_NICE ===\n");
    printf("  当前策略 = %s\n", policy_name(sched_getscheduler(0)));
    printf("  SCHED_FIFO 优先级范围 = [%d, %d]\n",
           sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO));
    printf("  SCHED_RR   优先级范围 = [%d, %d]\n",
           sched_get_priority_min(SCHED_RR), sched_get_priority_max(SCHED_RR));
    printf("  SCHED_FIFO 与 SCHED_RR 的区别：同优先级下 FIFO 一直跑到主动让出，\n");
    printf("  RR 会按时间片轮转。二者都抢占 SCHED_OTHER 的任务。\n\n");

    try_sched(SCHED_FIFO, 80);
    try_sched(SCHED_FIFO, 1);
    try_sched(SCHED_OTHER, 0);

    printf("\n  -> 上面 FIFO 两次都失败、OTHER 成功，说明本进程没有 CAP_SYS_NICE。\n");
    printf("     注意 EPERM(%d) 与 EINVAL(%d) 的区别：\n", EPERM, EINVAL);
    printf("       EINVAL = 参数不合法（比如 prio 超出范围）；\n");
    printf("       EPERM  = 参数没问题，是权限不够。\n");
    errno = 0;
    try_sched(SCHED_FIFO, 9999);       /* 故意给个越界优先级，看错误码 */
    printf("      ↑ 这一条就是 EINVAL：优先级越界比权限检查更早。\n");

    printf("\n=== ③ CPU 亲和：把进程钉在某个核上 ===\n");
    cpu_set_t before;
    if (sched_getaffinity(0, sizeof(before), &before) == 0) {
        printf("  当前亲和的 CPU：");
        for (int i = 0; i < CPU_SETSIZE; i++) if (CPU_ISSET(i, &before)) printf("%d ", i);
        printf("\n");
    }
    cpu_set_t one;
    CPU_ZERO(&one);
    CPU_SET(0, &one);
    errno = 0;
    if (sched_setaffinity(0, sizeof(one), &one) == 0) {
        cpu_set_t after;
        sched_getaffinity(0, sizeof(after), &after);
        printf("  sched_setaffinity(只留 CPU 0) -> OK，现在亲和集：");
        for (int i = 0; i < CPU_SETSIZE; i++) if (CPU_ISSET(i, &after)) printf("%d ", i);
        printf("\n");
    } else {
        printf("  sched_setaffinity 失败 errno=%d (%s)\n", errno, strerror(errno));
    }
    /* 恢复原样，别影响后面的实验 */
    sched_setaffinity(0, sizeof(before), &before);
    errno = 0;

    printf("\n=== ④ 内存锁定：消除缺页带来的毛刺 ===\n");
    struct rlimit rl;
    if (getrlimit(RLIMIT_MEMLOCK, &rl) == 0)
        printf("  RLIMIT_MEMLOCK: soft=%lld  hard=%lld 字节\n",
               (long long)rl.rlim_cur, (long long)rl.rlim_max);
    errno = 0;
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == 0) {
        printf("  mlockall(MCL_CURRENT|MCL_FUTURE) -> OK\n");
        printf("    MCL_CURRENT = 锁住现在已映射的页\n");
        printf("    MCL_FUTURE  = 之后新分配的页也自动锁住\n");
    } else {
        printf("  mlockall -> 失败 errno=%d (%s)\n", errno, strerror(errno));
    }
    errno = 0;

    /* 单个 mlock 能不能锁一段？实测一下容量感受 */
    size_t want = 64 * 1024 * 1024;              /* 64 MiB */
    void *p = mmap(NULL, want, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p != MAP_FAILED) {
        errno = 0;
        int rc = mlock(p, want);
        printf("  mlock(64MiB) -> %s", rc == 0 ? "OK\n" : "失败 ");
        if (rc != 0) printf("errno=%d (%s)\n", errno, strerror(errno));
        munmap(p, want);
    }
    errno = 0;

    printf("\n=== ⑤ 实时性的层次：从「调参数」到「换内核」 ===\n");
    printf("  %-30s %s\n", "层次", "解决什么");
    printf("  %-30s %s\n", "------------------------------", "--------------------------");
    printf("  %-30s %s\n", "SCHED_FIFO / SCHED_RR", "让本任务抢占普通任务");
    printf("  %-30s %s\n", "mlockall", "消除缺页导致的微秒级停顿");
    printf("  %-30s %s\n", "sched_setaffinity", "消除跨核迁移 / 缓存失效");
    printf("  %-30s %s\n", "隔离 CPU（isolcpus / cpuset）", "让别的任务别来抢这个核");
    printf("  %-30s %s\n", "关闭 C-state / 锁频", "消除降频唤醒延迟");
    printf("  %-30s %s\n", "PREEMPT_RT 内核", "让内核本身也可抢占（中断线程化）");
    printf("  %-30s %s\n", "busy-poll / 无锁队列", "把「等」变成「直接查」");
    printf("\n  → 前三条是应用层能做的，本 demo 全试了；\n");
    printf("     后三条要动内核/固件/硬件，属于 §2.18 里说的「另一层」。\n");

    printf("\n=== ⑥ 硬实时 vs 软实时：先问清需求 ===\n");
    printf("  硬实时：错过截止时间 = 系统失败（飞控、心脏起搏器、工业伺服）\n");
    printf("  软实时：偶尔错过只是质量下降（音视频、行情推送、交易回报）\n");
    printf("  HFT 大多落在「软实时的极端版」：不要求 0 抖动，只要求抖动小到\n");
    printf("  对手吃不到你的价。所以工程重点在「把长尾砍掉」，不是追平均值。\n");
    return 0;
}
