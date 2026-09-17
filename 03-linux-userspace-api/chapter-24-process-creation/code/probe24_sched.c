/* probe24_sched.c

   ⚠️ 本仓库自写 —— 探针，**不属于原书内容**。

   原书 §24.4 用一句话打发掉了「谁先跑」的可调性：
     "This default can be changed by assigning a nonzero value to the
      Linux-specific /proc/sys/kernel/sched_child_runs_first file."
   但这句话是在 2010 年（2.6.32 时代）写的。内核调度器后来换过代
   （Linux 6.6 起 CFS 被 EEVDF 取代），那个 `if` 分支还在不在、这个开关
   还灵不灵，书里当然不会告诉你。

   本探针只做一件事：把「书上说的那个文件」在实测沙箱里的**真实状态**问出来，
   好让笔记里的判断有实测依据，而不是只有源码推断。

   编译（自包含）：
     gcc -O0 -Wall -Wextra -o probe24_sched probe24_sched.c
*/

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

/* 读一个小文本文件并打印第一行；读不到就把 errno 原样报出来 */
static void showOne(const char *path)
{
    char buf[256];
    ssize_t n;
    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        printf("  %-44s  <打不开> errno=%d (%s)\n", path, errno, strerror(errno));
        return;
    }

    n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0)
        n = 0;
    buf[n] = '\0';
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
        buf[--n] = '\0';

    printf("  %-44s  \"%s\"\n", path, buf);
    close(fd);
}

int main(void)
{
    setbuf(stdout, NULL);               /* 沙箱 stdout 是全缓冲，探针必须立刻可见 */

    printf("[内核] ");
    showOne("/proc/version");

    printf("\n[①] 原书 §24.4 点名的那个开关\n");
    showOne("/proc/sys/kernel/sched_child_runs_first");

    printf("\n[②] 同一目录下其它 sched_* 开关（做对照：文件在不在、读不读得到）\n");
    showOne("/proc/sys/kernel/sched_autogroup_enabled");
    showOne("/proc/sys/kernel/sched_cfs_bandwidth_slice_us");
    showOne("/proc/sys/kernel/sched_rt_runtime_us");
    showOne("/proc/sys/kernel/sched_deadline_period_max_us");

    printf("\n[③] 调度器特性开关（debugfs；没挂载就说明沙箱里看不到）\n");
    showOne("/sys/kernel/debug/sched/features");
    showOne("/sys/kernel/debug/sched/debug");

    printf("\n[④] 本进程的调度策略与 nice 值（对照「谁先跑」的调度上下文）\n");
    {
        int policy = sched_getscheduler(0);
        struct sched_param sp;
        errno = 0;
        if (sched_getparam(0, &sp) == 0)
            printf("  sched_getscheduler(0) = %d (0=SCHED_OTHER)   "
                   "sched_priority = %d   nice = %d\n",
                   policy, sp.sched_priority, getpriority(PRIO_PROCESS, 0));
        else
            printf("  sched_getparam 失败 errno=%d (%s)\n", errno, strerror(errno));
    }

    return 0;
}
