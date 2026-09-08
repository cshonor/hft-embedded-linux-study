#ifndef MINI_TCB_H
#define MINI_TCB_H

#include <stdint.h>
#include <setjmp.h>

#define TASK_STACK_SIZE 1024
#define MAX_TASKS       4

typedef enum {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_DEAD,
} task_state_t;

typedef struct tcb {
    jmp_buf  ctx;
    uint8_t  stack[TASK_STACK_SIZE];
    void    *sp;           /* 指向栈内某位置，教学用 */
    uint8_t  prio;
    task_state_t state;
    void   (*entry)(void);
    char     name[16];
} tcb_t;

void sched_init(void);
int  task_create(const char *name, uint8_t prio, void (*entry)(void));
void sched_start(void);
void task_yield(void);
tcb_t *task_get(int idx);
int task_count_get(void);

extern tcb_t *current;

#endif
