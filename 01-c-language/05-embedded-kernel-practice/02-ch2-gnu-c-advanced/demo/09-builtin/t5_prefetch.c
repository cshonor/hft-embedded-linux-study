/* T5: __builtin_prefetch - cache hints */
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define ELEMS 1024

struct node {
    int val;
    struct node *next;
};

/* prefetch next node while processing current */
__attribute__((noinline))
int sum_with_prefetch(struct node *head)
{
    int sum = 0;
    struct node *p = head;
    while (p) {
        __builtin_prefetch(p->next, 0, 3); /* read, high locality */
        sum += p->val;
        p = p->next;
    }
    return sum;
}

/* same without prefetch */
__attribute__((noinline))
int sum_no_prefetch(struct node *head)
{
    int sum = 0;
    struct node *p = head;
    while (p) {
        sum += p->val;
        p = p->next;
    }
    return sum;
}

/* prefetch variants: write vs read, locality levels */
__attribute__((noinline))
void demo_prefetch_args(int *p)
{
    __builtin_prefetch(p,     0, 0);  /* read, no temporal locality */
    __builtin_prefetch(p,     0, 1);  /* read, low temporal locality */
    __builtin_prefetch(p,     0, 2);  /* read, medium temporal */
    __builtin_prefetch(p,     0, 3);  /* read, high temporal */
    __builtin_prefetch(p + 1, 1, 3);  /* write, high temporal */
}

/* does prefetch generate any code at -O0? */
__attribute__((noinline))
void prefetch_at_O0(int *p)
{
    __builtin_prefetch(p, 0, 3);
}

int main(void)
{
    /* build a linked list */
    static struct node nodes[ELEMS];
    for (int i = 0; i < ELEMS - 1; i++) {
        nodes[i].val = i;
        nodes[i].next = &nodes[i + 1];
    }
    nodes[ELEMS - 1].val = ELEMS - 1;
    nodes[ELEMS - 1].next = NULL;

    printf("sum_with_prefetch = %d\n", sum_with_prefetch(&nodes[0]));
    printf("sum_no_prefetch   = %d\n", sum_no_prefetch(&nodes[0]));

    int x = 42;
    demo_prefetch_args(&x);
    prefetch_at_O0(&x);

    /* what does prefetch(NULL) do? UB but let's see */
    /* __builtin_prefetch(NULL, 0, 3);  -- don't run, just compile */

    return 0;
}
