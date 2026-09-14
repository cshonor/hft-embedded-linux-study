/* TLPI 第 10 章 §10.5 Updating the System Clock
 * 改系统时钟的四个接口在**非特权**进程下分别怎么失败；只读接口怎么读；
 * CAP_SYS_TIME 到底有没有
 *
 * 编译: gcc -O0 -Wall -Wextra -o c10_5_setclock c10_5_setclock.c
 * 运行: ./c10_5_setclock                      (无需参数；本容器非 root)
 *
 * 原书 §10.5 讲了三条改时钟的路（settimeofday / clock_settime / adjtime）
 * 和 NTP 的渐进校正。本 demo 只做一件事：**在容器里把每一路都试一遍，
 * 看它到底返回什么**。做不成的实验就如实打印「EPERM」，不假装成功
 * —— 这是本仓库 demo 的统一约定。
 *
 * 预期（非特权容器）：4 个写接口全部 -1/EPERM，2 个只读接口成功。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <time.h>
#include <unistd.h>

/* CAP_SYS_TIME 在 capability 位图里的位号（include/uapi/linux/capability.h） */
#define CAP_SYS_TIME_BIT 25

static int read_cap_sys_time(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    char line[512];
    unsigned long long eff = 0;

    if (f == NULL)
        return -1;
    while (fgets(line, sizeof line, f) != NULL) {
        if (strncmp(line, "CapEff:", 7) == 0) {
            sscanf(line + 7, "%llx", &eff);
            break;
        }
    }
    fclose(f);
    return (int) ((eff >> CAP_SYS_TIME_BIT) & 1ULL);
}

static void show_identity(void)
{
    int cap = read_cap_sys_time();

    printf("== 0. 先看有没有资格改时钟 ==\n");
    printf("uid=%ld  euid=%ld  egid=%ld\n",
           (long) getuid(), (long) geteuid(), (long) getegid());
    printf("CapEff 里 CAP_SYS_TIME(bit %d) = %s\n",
           CAP_SYS_TIME_BIT,
           cap < 0 ? "读不到 /proc/self/status"
                   : (cap ? "1 —— 有资格" : "0 —— 没资格，下面的写操作必然 EPERM"));
    printf("\n");
}

static void probe_writes(void)
{
    struct timespec ts = { 0, 0 };
    struct timeval tv = { 0, 0 };
    struct timeval delta = { 0, 1000 };     /* 想往前拨 1000 微秒 */
    struct timeval old = { 0, 0 };
    struct timex tx;
    int rc;

    printf("== 1. 四个「改时钟」接口逐个试 ==\n");

    errno = 0;
    ts.tv_sec = (time_t) 0;                 /* 想把时钟设成 1970-01-01 */
    rc = clock_settime(CLOCK_REALTIME, &ts);
    printf("  clock_settime(CLOCK_REALTIME, 1970-01-01) -> %d  errno=%d (%s)\n",
           rc, errno, strerror(errno));

    errno = 0;
    rc = settimeofday(&tv, NULL);
    printf("  settimeofday(&{0,0}, NULL)                  -> %d  errno=%d (%s)\n",
           rc, errno, strerror(errno));

    errno = 0;
    rc = adjtime(&delta, &old);
    printf("  adjtime(+1000us, &old)                      -> %d  errno=%d (%s)\n",
           rc, errno, strerror(errno));

    memset(&tx, 0, sizeof tx);
    tx.modes = ADJ_FREQUENCY;
    tx.freq = 100;
    errno = 0;
    rc = clock_adjtime(CLOCK_REALTIME, &tx);
    printf("  clock_adjtime(REALTIME, modes=ADJ_FREQUENCY) -> %d  errno=%d (%s)\n",
           rc, errno, strerror(errno));
    printf("（这四条在非特权进程下都必须是 EPERM —— 内核用\n"
           "  capable(CAP_SYS_TIME) 在真正动时钟之前就挡掉了）\n\n");
}

static void probe_reads(void)
{
    struct timeval old = { 0, 0 };
    struct timex tx;
    int rc;

    printf("== 2. 两个「只读」接口是允许的 ==\n");

    errno = 0;
    rc = adjtime(NULL, &old);
    printf("  adjtime(NULL, &old) 读待校正量 -> %d  剩余 %ld.%06ld s  errno=%d (%s)\n",
           rc, (long) old.tv_sec, (long) old.tv_usec, errno, strerror(errno));

    memset(&tx, 0, sizeof tx);
    errno = 0;
    rc = clock_adjtime(CLOCK_REALTIME, &tx);
    if (rc < 0) {
        printf("  clock_adjtime(REALTIME, modes=0) 只读 -> %d  errno=%d (%s)\n",
               rc, errno, strerror(errno));
    } else {
        printf("  clock_adjtime(REALTIME, modes=0) 只读 -> %d  (返回值即 status)\n", rc);
        printf("    freq      = %ld  (每 2^16 单位 = 1 ppm)\n", tx.freq);
        printf("    offset    = %ld us\n", tx.offset);
        printf("    esterror  = %ld us\n", tx.esterror);
        printf("    maxerror  = %ld us\n", tx.maxerror);
        printf("    status    = 0x%x  (STA_PLL=%d STA_UNSYNC=%d STA_NANO=%d)\n",
               tx.status, !!(tx.status & STA_PLL), !!(tx.status & STA_UNSYNC),
               !!(tx.status & STA_NANO));
        printf("    tai       = %d  (TAI - UTC 的闰秒补偿)\n", (int) tx.tai);
        /* tick 就是内核的 tick_usec：USER_HZ 一个 tick 的微秒数。
           它不是 2^-16 定标量（freq 才是）。见 kernel/time/ntp.c:816
           `txc->tick = tick_usec;` + :34 `tick_usec = USER_TICK_USEC`。 */
        printf("    tick      = %ld  (USER_HZ 下一个 tick 的微秒数 = 10 ms)\n", tx.tick);
    }
    printf("\n");
}

static void show_gettimeofday_timezone(void)
{
    struct timeval tv;
    struct timezone tz;
    int rc;

    printf("== 3. gettimeofday 的第二个参数（struct timezone） ==\n");
    memset(&tz, 0xAA, sizeof tz);           /* 先填成 0xAA，看内核会不会改写 */
    errno = 0;
    rc = gettimeofday(&tv, &tz);
    printf("  gettimeofday(&tv, &tz) -> %d  errno=%d (%s)\n",
           rc, errno, strerror(errno));
    printf("  tv = %ld.%06ld\n", (long) tv.tv_sec, (long) tv.tv_usec);
    printf("  tz = { tz_minuteswest=%d, tz_dsttime=%d }\n",
           tz.tz_minuteswest, tz.tz_dsttime);
    printf("  tz 仍是 0xAA 模式? %s   <- glibc 根本不填它\n",
           (tz.tz_minuteswest == (int) 0xAAAAAAAA) ? "是" : "否");
    printf("（POSIX 早就把 timezone 参数标成废弃：Linux 里时区完全由用户态\n"
           "  glibc + /etc/localtime 决定，内核手上没有时区信息）\n");
}

int main(void)
{
    show_identity();
    probe_writes();
    probe_reads();
    show_gettimeofday_timezone();

    printf("\n原书 §10.5 的要点，在这台机器上的对照：\n");
    printf("  settimeofday()  —— 废弃；要 CAP_SYS_TIME；精度只到微秒\n");
    printf("  clock_settime() —— 推荐替代；同样要 CAP_SYS_TIME；精度到纳秒；**会跳变**\n");
    printf("  adjtime()       —— 平滑推进：内核对 delta 按每秒约 %d us 的速率摊开\n",
           500);
    printf("  clock_adjtime() —— adjtimex(2) 的 POSIX 门面，能同时调 freq/offset/tick\n");
    return 0;
}
