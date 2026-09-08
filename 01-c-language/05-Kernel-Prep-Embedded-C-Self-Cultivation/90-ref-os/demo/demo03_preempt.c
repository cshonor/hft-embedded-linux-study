#include "../rtos/tcb.h"
#include <stdio.h>

/* demo03：与 demo02 相同协作调度，强调高优先级先运行 */
static void task_bg(void)
{
    puts("[bg] start");
    task_yield();
    puts("[bg] resume");
}

static void task_fg(void)
{
    puts("[fg] preempt-style: high prio runs first");
    task_yield();
    puts("[fg] done");
}

int main(void)
{
    sched_init();
    task_create("bg", 1, task_bg);
    task_create("fg", 5, task_fg);
    puts("demo03: priority-ready queue (host cooperative; ARM 用 PendSV 抢占)");
    sched_start();
    return 0;
}
