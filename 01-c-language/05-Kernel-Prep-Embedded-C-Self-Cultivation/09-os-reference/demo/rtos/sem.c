#include "sem.h"
#ifndef NO_SCHED
#include "tcb.h"
#endif

void mini_sem_init(mini_sem_t *s, int val)
{
    if (s)
        s->count = val;
}

void mini_sem_wait(mini_sem_t *s)
{
    while (s && s->count <= 0) {
#ifndef NO_SCHED
        task_yield();
#else
        ;
#endif
    }
    if (s)
        s->count--;
}

void mini_sem_post(mini_sem_t *s)
{
    if (s)
        s->count++;
}
