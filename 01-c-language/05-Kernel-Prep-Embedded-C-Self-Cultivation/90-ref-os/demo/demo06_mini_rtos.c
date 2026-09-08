#include "rtos/tcb.h"
#include "rtos/sem.h"
#include "rtos/queue.h"
#include <stdio.h>

/* demo06：ch09 分层 rtos/ 组件 + ch08 TCB 综合 */
static mini_queue_t evt_q;
static mini_sem_t   evt_sem;

static void task_sensor(void)
{
    queue_push(&evt_q, 25);
    mini_sem_post(&evt_sem);
    printf("[sensor] sample queued\n");
    task_yield();
}

static void task_logger(void)
{
    mini_sem_wait(&evt_sem);
    int temp;
    queue_pop(&evt_q, &temp);
    printf("[logger] temperature=%d C\n", temp);
}

int main(void)
{
    queue_init(&evt_q);
    mini_sem_init(&evt_sem, 0);
    sched_init();
    task_create("sensor", 3, task_sensor);
    task_create("logger", 2, task_logger);
    puts("demo06: mini RTOS (sched+sem+queue modules)");
    sched_start();
    return 0;
}
