/* T6: clobber list — memory / register / "cc"
 *
 * Clobbers tell gcc "this asm will dirty these things beyond the outputs".
 *  "memory" — asm reads/writes memory gcc doesn't track; reload everything live.
 *  "cc"     — condition flags (x86: EFLAGS). On x86 "cc" is accepted but rarely needed.
 *  "rax" etc — a specific register is clobbered.
 *
 * If you forget a clobber, gcc may keep a value in that reg across the asm and
 * read a corrupted value. The classic: using a scratch reg without declaring it.
 */
#include <stdio.h>

/* clobber a specific register without declaring it as output */
static int use_scratch(int a)
{
    int out;
    asm volatile (
        "movl %1, %%eax\n\t"   /* eax = a   (eax is scratch, must clobber) */
        "addl $5, %%eax\n\t"   /* eax += 5 */
        "movl %%eax, %0"       /* out = eax */
        : "=r"(out)
        : "r"(a)
        : "rax"               /* MUST tell gcc eax is clobbered */
    );
    return out;
}

/* "memory" clobber: forces reload of all live memory across the asm.
 * This is what barrier() does — compiler fence, NOT cpu fence. */
static int mem_clobber(int *p)
{
    int x = *p;           /* load 1 */
    asm volatile ("" ::: "memory");   /* compiler barrier */
    int y = *p;           /* load 2 — gcc can no longer prove x==y, reloads */
    return x + y;
}

/* BUG: forget the clobber — gcc may keep something important in rax */
static int forget_clobber(int a)
{
    int important = a * 1000;   /* gcc might put this in rax */
    int out;
    asm volatile (
        "movl %1, %%eax\n\t"   /* clobbers rax! but we didn't tell gcc */
        "movl %%eax, %0"
        : "=r"(out)
        : "r"(a)
        /* missing: "rax" */
    );
    return out + important;    /* important might be read from a clobbered rax */
}

int main(void)
{
    int v = 10;
    printf("use_scratch(10) = %d (should be 15)\n", use_scratch(v));
    int p = 42;
    printf("mem_clobber(&42) = %d\n", mem_clobber(&p));
    printf("forget_clobber(3) = %d (may be wrong)\n", forget_clobber(3));
    return 0;
}
