/* T8: -O0 vs -O2 register allocation for asm
 *
 * At -O0: every C variable lives in memory (stack slot). Operands with "r" get
 *         loaded into a fresh register just for the asm, then stored back.
 * At -O2: variables may stay in registers across statements. asm operands "r"
 *         may reuse the register the variable already lives in — no load/store.
 *
 * Consequence: the SAME asm can behave differently under different -O.
 * Especially: a missing clobber is "fine" at -O0 (var is on stack, untouched)
 * but CORRUPTS at -O2 (var lives in the clobbered register).
 */
#include <stdio.h>

static int demo(int a, int b)
{
    int x = a;            /* at -O2 x may live in a register */
    asm volatile (
        "movl %2, %%eax\n\t"   /* eax = b  (clobbers eax!) */
        "addl %1, %%eax\n\t"   /* eax += a */
        "movl %%eax, %0"       /* x = eax */
        : "=r"(x)
        : "r"(a), "r"(b)
        : "rax"
    );
    return x;
}

/* The bug version: forget rax clobber */
static int demo_bug(int a, int b)
{
    int x = a;
    asm volatile (
        "movl %2, %%eax\n\t"
        "addl %1, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(x)
        : "r"(a), "r"(b)
        /* missing rax clobber */
    );
    return x;
}

int main(void)
{
    printf("demo(3,5)    = %d (should be 8)\n", demo(3, 5));
    printf("demo_bug(3,5)= %d (may differ at -O2)\n", demo_bug(3, 5));
    return 0;
}
