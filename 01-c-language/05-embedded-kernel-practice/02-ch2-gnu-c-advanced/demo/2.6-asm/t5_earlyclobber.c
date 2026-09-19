/* T5: earlyclobber & and commutative %
 *
 *  "&" (earlyclobber): this output is written BEFORE all inputs are consumed.
 *     Without &, gcc may assign the same register to an input and this output,
 *     corrupting the input before the asm reads it.
 *
 *  "%" (commutative): operands are interchangeable; gcc may swap to ease alloc.
 *
 * Classic bug: asm writes output early, then reads a later input.
 * If gcc reuses the output reg for the input, the input is clobbered.
 */
#include <stdio.h>

/* BUG: no & — gcc may put out and b in the same register.
 * NOTE: x86 add is 2-operand; we use mov+add to be x86-runnable. */
static int bug_no_amp(int a, int b)
{
    int out;
    asm volatile (
        "mov %2, %0\n\t"    /* write out = b   (early!) */
        "add %1, %0"        /* out += a */
        : "=r"(out)
        : "r"(a), "r"(b)
    );
    return out;
}

/* FIX: = &r — output register must not alias any input */
static int fix_amp(int a, int b)
{
    int out;
    asm volatile (
        "mov %2, %0\n\t"
        "add %1, %0"
        : "=&r"(out)
        : "r"(a), "r"(b)
    );
    return out;
}

/* commutative %: tells gcc a,b can swap — gives register allocator freedom.
 * x86 `lea (base,index), dst` computes base+index in one insn, symmetric. */
static int commutative(int a, int b)
{
    int out;
    asm volatile (
        "lea (%1, %2), %0"   /* out = a + b */
        : "=r"(out)
        : "%r"(a), "r"(b)    /* % marks a as commutative with the next operand */
    );
    return out;
}

/* Real earlyclobber scenario: two outputs, one input shared */
static int real_case(int *p, int v)
{
    int x, y;
    asm volatile (
        "movl %2, %0\n\t"   /* x = v   (write early) */
        "movl %2, %1"       /* y = v   */
        : "=&r"(x), "=r"(y)
        : "r"(v)
    );
    *p = x;
    return y;
}

int main(void)
{
    printf("bug_no_amp(3,5) = %d (may be wrong)\n", bug_no_amp(3, 5));
    printf("fix_amp(3,5)    = %d (should be 8)\n", fix_amp(3, 5));
    printf("commutative(3,5)= %d (should be 8)\n", commutative(3, 5));
    int p;
    int y = real_case(&p, 42);
    printf("real_case: x=%d y=%d\n", p, y);
    return 0;
}
