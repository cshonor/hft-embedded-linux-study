/* T3: output constraints — =r / =m / +r / +m
 *
 *  "="  write-only: operand written by asm, input value not provided.
 *  "+"  read-write: operand both read and written.
 *  "=&r" earlyclobber: written before all inputs consumed (see T5).
 *  "=r" register, "=m" memory, "=g" general (reg or mem).
 *
 * Common bug: using "=" on something you also read -> wrong value.
 */
#include <stdio.h>

/* =r : write-only output to a register */
static int out_reg(int in)
{
    int o;
    asm volatile ("mov %1, %0" : "=r"(o) : "r"(in));
    return o;
}

/* =m : output to memory (force a store) */
static long out_mem(long in)
{
    long o;
    asm volatile ("mov %1, %0" : "=m"(o) : "r"(in));
    return o;
}

/* +r : read-write register — the input value matters AND we write back */
static int rw_reg(int x)
{
    asm volatile ("add $10, %0" : "+r"(x));
    return x;
}

/* BUG demo: using =r but the template also reads %0's input value.
 * Compiler thinks %0 is write-only, may alias it with an input. */
static int bug_alias(int a, int b)
{
    int o;
    /* template reads %0 (the old value) AND writes it — should be +r */
    asm volatile ("add %2, %0" : "=r"(o) : "0"(a), "r"(b));
    /* "0"(a) ties input to operand 0, but = says write-only... */
    return o;
}

int main(void)
{
    printf("out_reg(42)   = %d\n", out_reg(42));
    printf("out_mem(100) = %ld\n", out_mem(100));
    printf("rw_reg(5)    = %d\n", rw_reg(5));
    printf("bug_alias(3,4) = %d (may be wrong: should be 7)\n", bug_alias(3, 4));
    return 0;
}
