/* T9: asm goto — jump to a C label from inside asm
 *
 * Syntax (gcc >= 4.5, full form with outputs since gcc 14):
 *   asm goto ("..." :: input_operands : clobbers : labels);
 *
 * The labels are C labels in the same function. The asm may branch to them.
 * Classic use: lock acquire retry, feature detection with jump-on-condition.
 *
 * clang supports asm goto since clang 8 (no outputs), outputs since clang 18.
 *
 * No output operands allowed in the classic form (outputs need gcc 14+/clang 18).
 * The 6th colon-separated section is for labels.
 */
#include <stdio.h>
#include <stddef.h>

static int try_feature(void)
{
    int ok = 0;
    /* x86: try CPUID. Simplified: test a flag then branch. */
    asm goto (
        "bt $0, %%rax\n\t"      /* test bit 0 of rax (loaded below) */
        "jc %l[yes]"            /* if set, jump to C label "yes" */
        :: "a"(1)               /* rax = 1, bit 0 set */
        : "cc"
        : yes
    );
    /* fallthrough path */
    ok = 0;
    return ok;
yes:
    ok = 1;
    return ok;
}

/* Real kernel-style pattern: spinlock retry */
static int lock_attempt(int *lock_var)
{
    /* simplified: try to atomically swap 0->1; if old was 0 we got it */
    asm goto (
        "xor %%eax, %%eax\n\t"
        "mov $1, %%ebx\n\t"
        "lock cmpxchg %%ebx, %0\n\t"  /* if *lock==eax(0), *lock=ebx(1), ZF=1 */
        "jnz %l[retry]"
        :: "m"(*lock_var)
        : "eax", "ebx", "cc", "memory"
        : retry
    );
    return 1;   /* acquired */
retry:
    return 0;   /* contended */
}

int main(void)
{
    printf("try_feature() = %d\n", try_feature());
    int lock = 0;
    printf("lock_attempt(free) = %d\n", lock_attempt(&lock));
    lock = 1;
    printf("lock_attempt(held) = %d\n", lock_attempt(&lock));
    return 0;
}
