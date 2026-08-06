#include "../rtos/tcb.h"
#include <stdio.h>

static void task_low(void)
{
    for (int i = 0; i < 3; i++) {
        printf("[prio1 %s] sp=%p step=%d\n",
               current->name, current->sp, i);
        task_yield();
    }
}

static void task_high(void)
{
    for (int i = 0; i < 3; i++) {
        printf("[prio3 %s] sp=%p step=%d\n",
               current->name, current->sp, i);
        task_yield();
    }
}

int main(void)
{
    sched_init();
    task_create("worker", 1, task_low);
    task_create("urgent", 3, task_high);

    printf("TCB count=%d, independent stacks (cooperative setjmp switch)\n",
           task_count_get());
    for (int i = 0; i < task_count_get(); i++) {
        tcb_t *t = task_get(i);
        printf("  %s stack=[%p..%p) sp=%p prio=%u\n",
               t->name, (void *)t->stack,
               (void *)(t->stack + TASK_STACK_SIZE), t->sp, t->prio);
    }

    sched_start();
    puts("all tasks done");
    return 0;
}
