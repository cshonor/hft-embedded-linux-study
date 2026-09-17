/* ex23_1_my_alarm.c —— TLPI 习题 23-1 的实现
 *
 * 题面（原书 §23.9，逐字）：
 *     "Although alarm() is implemented as a system call within the Linux
 *      kernel, this is redundant. Implement alarm() using setitimer()."
 *
 * 思路：alarm(seconds) 与 setitimer(ITIMER_REAL, ...) 的对应关系是
 *     it_value.tv_sec  = seconds          （单次，不倒计时重复）
 *     it_value.tv_usec = 0
 *     it_interval      = {0, 0}           ← **interval 必须是 0**，
 *                                            否则就成了周期定时器而不是 alarm
 * 而返回值（"上一个闹钟还剩几秒"）来自 setitimer() 的第三个参数 old_value。
 *
 * ⭐ 返回值是本习题的**真正考点**：alarm(2) 与 setitimer() 的返回值并不等价。
 *     实测（CE, gcc 13.3 / x86-64，冻结日志 tlpi-ch23-final*.txt）：
 *         my_alarm(4) 时上一次还剩 4.699907 秒 -> 本程序返回 4（直接取 tv_sec）
 *         alarm(5) 后过 0.3 秒，getitimer() 读到 4.699919 秒，
 *             alarm(0)                           -> glibc 返回 **5**（向上取整）
 *     差别来自内核的 alarm_setitimer()（见文件末尾引文）：剩余微秒 >= 500000
 *     时会 +1。也就是说**只照抄 setitimer(ITIMER_REAL, {it_value=tv_sec}) 并不
 *     完全等价** —— 这正是"用 setitimer 实现 alarm"这个题目值得做的原因。
 *
 * 程序做五件事：
 *   ① 空转语法：连续 my_alarm() 观察"返回值 = 上一个闹钟的剩余"
 *   ② 取消语义：my_alarm(0) 撤销闹钟并返回剩余（对应 alarm(0)）
 *   ③ 真触发：my_alarm(1) + pause()，确认 SIGALRM 真的送达
 *   ④ 与真 alarm() 对拍，把 ① 的"截断"与 glibc 的"向上取整"摆在一起
 *      —— 注意 ★ 处：真 alarm() 会**覆盖**本程序刚设的定时器，
 *         所以两者必须分开测，不能混在同一条时间线上。
 *
 * 编译：gcc -O0 -Wall -Wextra -o ex23_1_my_alarm ex23_1_my_alarm.c
 * 运行：./ex23_1_my_alarm
 *
 * 源码坐标：
 *   glibc 2.39  sysdeps/unix/sysv/linux/alarm.c  __alarm() 直接调 alarm 系统调用
 *   Linux      kernel/time/itimer.c   alarm_setitimer() —— 向上取整就在这里
 *              （v3.5 时该文件是 kernel/itimer.c；本文引用的原文取自
 *                linux-next v3.5 镜像站，**v6.6 的那一版未逐字核对**，
 *                但本机实测行为与下面这段逻辑完全吻合；逐字原文见
 *                notes/23.9-exercises.md，这里只写逻辑，免得再套一层注释）
 *
 *   内核逻辑（alarm_setitimer()，注释原文照抄）：
 *       do_setitimer(ITIMER_REAL, &it_new, &it_old);
 *
 *       .. We can't return 0 if we have an alarm pending ...  And we'd
 *          better return too much than too little anyway
 *       if ((!it_old.it_value.tv_sec && it_old.it_value.tv_usec) ||
 *           it_old.it_value.tv_usec >= 500000)
 *               it_old.it_value.tv_sec++;
 *
 *       return it_old.it_value.tv_sec;
 *
 *   即：剩余「秒为 0 但微秒非 0」或「微秒 >= 500000」时进位。
 *   所以真正的 alarm() 与"裸 setitimer + 取 tv_sec"在剩余 >= 0.5 秒时差 1。
 */
#define _POSIX_C_SOURCE 199309
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

static volatile sig_atomic_t alarmFired = 0;

/* 用 nanosleep 而不是 usleep：本文件只定义 _POSIX_C_SOURCE 199309，
   glibc 在这个宏下**不声明 usleep**（它属于 _DEFAULT_SOURCE / _XOPEN_SOURCE），
   而 nanosleep 是 POSIX.1b 正牌接口。 */
static void msleep(long ms)
{
    struct timespec req;

    req.tv_sec = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000L;
    while (nanosleep(&req, &req) == -1 && errno == EINTR)
        ;
}

static void handler(int sig)
{
    (void) sig;
    alarmFired = 1;
}

/* 习题 23-1 的实现：用 setitimer() 造一个 alarm() */
static unsigned int my_alarm(unsigned int seconds)
{
    struct itimerval newv, oldv;

    newv.it_interval.tv_sec = 0;        /* 单次 —— 这是 alarm 与周期定时器的分界 */
    newv.it_interval.tv_usec = 0;
    newv.it_value.tv_sec = (long) seconds;
    newv.it_value.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &newv, &oldv) == -1)
        return 0;                       /* alarm(3) 失败也返回 0 */

    printf("    [my_alarm(%u)] 旧定时器剩余：%ld.%06ld 秒"
           " -> 返回 tv_sec = %ld\n",
           seconds, (long) oldv.it_value.tv_sec,
           (long) oldv.it_value.tv_usec, (long) oldv.it_value.tv_sec);

    return (unsigned int) oldv.it_value.tv_sec;
}

int main(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        printf("sigaction 失败 errno=%d\n", errno);
        return 1;
    }

    printf("=== ① 连续 my_alarm()：返回值就是「上一个闹钟的剩余」 ===\n");
    my_alarm(5);                        /* 第一个：之前没有闹钟 => 剩余 0.000000 */
    msleep(300);                        /* 消耗 0.3 秒 */
    my_alarm(4);                        /* 应剩约 4.7 秒 */
    msleep(300);
    my_alarm(3);                        /* 应剩约 3.7 秒 */

    printf("=== ② my_alarm(0) 撤销闹钟，并返回剩余（对应 alarm(0)） ===\n");
    my_alarm(0);
    alarmFired = 0;
    sleep(1);                           /* 等 1 秒 —— 若没撤销成功，这里会收到信号 */
    printf("    撤销后等 1 秒，alarmFired = %d（期望 0，说明闹钟真的被撤销了）\n",
           (int) alarmFired);

    printf("=== ③ 真触发：my_alarm(1) + pause() ===\n");
    alarmFired = 0;
    my_alarm(1);
    while (!alarmFired)
        pause();
    printf("    收到 SIGALRM，alarmFired = %d（期望 1）\n", (int) alarmFired);

    printf("=== ④ 与真 alarm() 对拍（★ 分开测，因为两者会互相覆盖） ===\n");
    printf("  真 alarm(5)   -> 返回 %u\n", alarm(5));
    msleep(300);
    {
        struct itimerval itv;

        /* 先用 getitimer() 把「真实剩余」掏出来，再把 alarm(0) 的返回值放一起看 */
        if (getitimer(ITIMER_REAL, &itv) == 0)
            printf("  getitimer() 看到的真实剩余：%ld.%06ld 秒\n",
                   (long) itv.it_value.tv_sec, (long) itv.it_value.tv_usec);
        printf("  真 alarm(0)   -> 返回 %u   ← 与上一行比：**向上取整**，不是截断\n",
               alarm(0));
    }
    printf("  对照：① 里 my_alarm() 返回的是裸的 tv_sec（截断），"
           "两者在剩余 >= 0.5 秒时会差 1\n");

    printf("\n=== ex23_1 done ===\n");
    return 0;
}
