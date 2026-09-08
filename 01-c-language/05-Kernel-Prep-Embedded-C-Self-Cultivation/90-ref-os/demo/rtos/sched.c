#include "tcb.h"
#include <stdio.h>
#include <string.h>

static tcb_t tasks[MAX_TASKS];
static int task_count;
static int current_idx = -1;
static jmp_buf main_env;
static int sched_running;

tcb_t *current;

static void task_trampoline(void)
{
    if (current && current->entry)
        current->entry();
    if (current)
        current->state = TASK_DEAD;
    task_yield();
}

void sched_init(void)
{
    memset(tasks, 0, sizeof(tasks));
    task_count = 0;
    current_idx = -1;
    current = NULL;
    sched_running = 0;
}

int task_create(const char *name, uint8_t prio, void (*entry)(void))
{
    if (task_count >= MAX_TASKS || !entry)
        return -1;
    tcb_t *t = &tasks[task_count];
    memset(t, 0, sizeof(*t));
    strncpy(t->name, name, sizeof(t->name) - 1);
    t->prio = prio;
    t->entry = entry;
    t->state = TASK_READY;
    t->sp = t->stack + TASK_STACK_SIZE;
    return task_count++;
}

static int pick_next(void)
{
    int best = -1;
    uint8_t bp = 0;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].state == TASK_READY && tasks[i].prio >= bp) {
            bp = tasks[i].prio;
            best = i;
        }
    }
    return best;
}

void task_yield(void)
{
    if (!sched_running || current_idx < 0)
        return;

    if (setjmp(tasks[current_idx].ctx) == 0) {
        int next = pick_next();
        if (next < 0) {
            sched_running = 0;
            longjmp(main_env, 1);
        }
        tasks[current_idx].state = TASK_READY;
        current_idx = next;
        current = &tasks[current_idx];
        current->state = TASK_RUNNING;
        longjmp(tasks[current_idx].ctx, 1);
    }
}

void sched_start(void)
{
    if (setjmp(main_env) == 0) {
        sched_running = 1;
        int first = pick_next();
        if (first < 0)
            return;
        current_idx = first;
        current = &tasks[current_idx];
        current->state = TASK_RUNNING;
        if (setjmp(current->ctx) == 0)
            task_trampoline();
    }
}

tcb_t *task_get(int idx)
{
    if (idx < 0 || idx >= task_count)
        return NULL;
    return &tasks[idx];
}

int task_count_get(void) { return task_count; }
