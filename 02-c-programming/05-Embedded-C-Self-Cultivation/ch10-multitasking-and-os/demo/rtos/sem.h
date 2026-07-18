#ifndef MINI_SEM_H
#define MINI_SEM_H

typedef struct {
    volatile int count;
} mini_sem_t;

void mini_sem_init(mini_sem_t *s, int val);
void mini_sem_wait(mini_sem_t *s);
void mini_sem_post(mini_sem_t *s);

#endif
