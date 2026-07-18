#include "rtos/tcb.h"
#include "rtos/queue.h"
#include <stdio.h>

static mini_queue_t msg_q;

static void task_producer(void)
{
    for (int i = 1; i <= 4; i++) {
        queue_push(&msg_q, i * 10);
        printf("[producer] push %d\n", i * 10);
        task_yield();
    }
}

static void task_consumer(void)
{
    for (int i = 0; i < 4; i++) {
        int v;
        while (queue_pop(&msg_q, &v) != 0)
            task_yield();
        printf("[consumer] pop %d\n", v);
        task_yield();
    }
}

int main(void)
{
    queue_init(&msg_q);
    sched_init();
    task_create("producer", 2, task_producer);
    task_create("consumer", 2, task_consumer);
    puts("demo05: message queue producer/consumer (cooperative)");
    sched_start();
    return 0;
}
