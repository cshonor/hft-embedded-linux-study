#include "rtos/sem.h"
#include <stdio.h>

int main(void)
{
    mini_sem_t rx;

    mini_sem_init(&rx, 0);
    puts("[task] blocked on binary semaphore (count=0)");

    puts("[ISR] uart rx interrupt");
    mini_sem_post(&rx);

    mini_sem_wait(&rx);
    puts("[task] woke up, print buffer");

    puts("demo04: ISR sem_post -> task sem_wait sync");
    return 0;
}
