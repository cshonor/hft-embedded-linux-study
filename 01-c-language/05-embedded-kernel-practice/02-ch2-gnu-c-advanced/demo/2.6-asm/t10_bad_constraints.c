/* T10: constraint mistakes — compile error vs silent wrong code
 *
 * The whole danger of inline asm: the compiler can't check the instruction
 * matches the constraints. You describe operands with constraints, but the
 * asm template is opaque text. Mismatches are silent or late.
 */
#include <stdio.h>

/* (A) Compile-time error: constraint impossible to satisfy */
static int bad_impossible(void)
{
    int o;
    /* "i" requires a compile-time constant; a runtime var is not constant */
    int runtime = 5;
    asm volatile ("mov %1, %0" : "=r"(o) : "i"(runtime));
    return o;
}

/* (B) Assembler-time error: template uses wrong operand size.
 * `movl` is 32-bit but the operand is a 64-bit long — assembler rejects.
 * Compile with -DDEMO_BAD to see it. */
#if defined(DEMO_BAD)
static int bad_size(long v)
{
    long o;
    /* movl moves 32 bits but operand is 64-bit — assembler rejects */
    asm volatile ("movl %1, %0" : "=r"(o) : "r"(v));
    return o;
}
#else
static int bad_size(long v)
{
    long o;
    /* FIX: match sizes — movq for 64-bit, or let gcc pick with no suffix */
    asm volatile ("movq %1, %0" : "=r"(o) : "r"(v));
    return o;
}
#endif

/* (C) Silent wrong: =r but template never writes -> garbage output */
static int bad_nowrite(int a)
{
    int o;
    /* output "=r" but template only reads, never writes %0 */
    asm volatile ("mov %1, %1" : "=r"(o) : "r"(a));
    return o;   /* o is uninitialized garbage */
}

/* (D) Forgotten memory clobber: gcc caches a load across the asm */
static int bad_memclobber(int *p)
{
    int x = *p;
    asm volatile ("movl $99, %0" : "=m"(*p) :: );   /* writes *p via memory operand */
    int y = *p;
    /* gcc may have cached x==y and returned 2x; the "=m" tells it *p changed though */
    return x + y;
}

int main(void)
{
    /* (A) intentionally not compiled unless -DDEMO_BAD */
#if defined(DEMO_BAD)
    printf("bad_impossible = %d\n", bad_impossible());
#endif

    printf("bad_size(0x100000005) = 0x%lx (upper bits lost?)\n", (long)bad_size(0x100000005L));
    printf("bad_nowrite(7) = %d (garbage)\n", bad_nowrite(7));
    int v = 10;
    printf("bad_memclobber(&10) = %d (should be 109)\n", bad_memclobber(&v));
    return 0;
}
