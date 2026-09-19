/* T7: volatile semantics — the biggest misconception
 *
 * What asm volatile actually means (gcc manual, "Assembler Instructions"):
 *   1. Do NOT delete the asm even if its outputs are unused.
 *   2. Do NOT move it across jumps (in particular, do not hoist/sink).
 * What it does NOT mean:
 *   - It is NOT a CPU memory barrier. It does not stop the CPU reordering.
 *   - It does not stop gcc reordering other asm volatiles relative to each
 *     other in an unpredictable order (gcc keeps their *relative* order but
 *     only with respect to jumps, not other memory ops).
 *   - It does not emit any fence instruction.
 *
 * Without volatile:
 *   - gcc may delete the asm if outputs unused AND no side effects detected.
 *   - gcc may move it around freely.
 *
 * The kernel's barrier() = asm volatile("" ::: "memory"):
 *   - the "" produces no instruction
 *   - volatile keeps the (empty) asm in place
 *   - "memory" clobber is the real fence: reloads all live memory
 *   => compiler barrier, NOT hardware barrier. dmb/lfence/mfence needed too.
 */
#include <stdio.h>

/* (A) non-volatile asm with unused output — gcc may delete it */
static int dead_asm(int a)
{
    int unused;
    asm (                /* no volatile */
        "mov %1, %0"
        : "=r"(unused)
        : "r"(a)
    );
    (void)unused;
    return a;             /* unused not used -> whole asm may vanish at -O2 */
}

/* (B) volatile: kept even if output unused */
static int kept_asm(int a)
{
    int unused;
    asm volatile (
        "mov %1, %0"
        : "=r"(unused)
        : "r"(a)
    );
    (void)unused;
    return a;
}

/* (C) the empty-volatile idiom = no-op but blocks some motion */
static int empty_volatile(int a)
{
    asm volatile ("");
    return a;
}

/* (D) barrier(): compiler fence */
#define barrier() asm volatile("" ::: "memory")

static int with_barrier(int *p, int *q)
{
    int x = *p;
    barrier();           /* forces reload of everything, blocks reordering of loads */
    int y = *q;
    return x + y;
}

int main(void)
{
    int v = 7;
    printf("dead_asm(7)   = %d\n", dead_asm(v));
    printf("kept_asm(7)   = %d\n", kept_asm(v));
    printf("empty_volatile(7) = %d\n", empty_volatile(v));
    int a = 1, b = 2;
    printf("with_barrier(&a,&b) = %d\n", with_barrier(&a, &b));
    return 0;
}
