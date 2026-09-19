/* T4: input constraints — r / m / i / n / g / F
 *
 *  "r"  any general register (gcc picks)
 *  "m"  memory operand
 *  "i"  immediate constant known at compile time
 *  "n"  immediate constant (some arches restrict size)
 *  "g"  general (= r OR m OR i) — most permissive
 *  "F"  floating-point immediate
 *  "0".."9"  tie to another operand (must match constraint)
 *
 * Constraint modifiers after letter:
 *  "=" output, "+" read-write, "&" earlyclobber, "%" commutative, "!" prefer memory
 */
#include <stdio.h>

/* "r" : let gcc pick a register */
static int in_r(int a)
{
    int o;
    asm volatile ("mov %1, %0" : "=r"(o) : "r"(a));
    return o;
}

/* "m" : operand must be memory — asm does a load/store */
static int in_m(int a)
{
    int o;
    asm volatile ("movl %1, %0" : "=r"(o) : "m"(a));
    return o;
}

/* "i" : immediate constant — value baked into instruction */
static int in_i(void)
{
    int o;
    asm volatile ("mov %1, %0" : "=r"(o) : "i"(99));
    return o;
}

/* "n" : immediate with size constraint (often equivalent to "i" on x86) */
static int in_n(void)
{
    int o;
    asm volatile ("mov %1, %0" : "=r"(o) : "n"(77));
    return o;
}

/* "g" : general — register, memory, or immediate */
static int in_g(int a)
{
    int o;
    asm volatile ("mov %1, %0" : "=r"(o) : "g"(a));
    return o;
}

/* Tying: input "0" means "use the same constraint/operand as operand 0".
 * NOTE: gcc accepts this; clang 18 rejects "0" tied to a "+r" operand.
 * The idiom is redundant anyway (+r already provides the value) — shown
 * here to document the divergence. */
#if !defined(__clang__)
static int tied(int x)
{
    /* operand 0 is +r (rw), operand 1 is "0" -> same register as op0 */
    asm volatile ("add $5, %0" : "+r"(x) : "0"(x));
    return x;
}
#else
static int tied(int x)
{
    /* clang workaround: +r alone carries both read and write */
    asm volatile ("add $5, %0" : "+r"(x));
    return x;
}
#endif

int main(void)
{
    printf("in_r(11)  = %d\n", in_r(11));
    printf("in_m(22)  = %d\n", in_m(22));
    printf("in_i()    = %d\n", in_i());
    printf("in_n()    = %d\n", in_n());
    printf("in_g(33)  = %d\n", in_g(33));
    printf("tied(10)  = %d (should be 15)\n", tied(10));
    return 0;
}
