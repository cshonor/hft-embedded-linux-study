/* Listing 6-2 style: volatile vs non-volatile across setjmp/longjmp. */
#include <setjmp.h>
#include <stdio.h>

static jmp_buf env;

static void do_jump(int nvar, int rvar, int vvar) {
    printf("inside do_jump(): nvar=%d rvar=%d vvar=%d\n", nvar, rvar, vvar);
    longjmp(env, 1);
}

int main(void) {
    int nvar = 111;              /* may be optimized into register */
    register int rvar = 222;     /* register hint */
    volatile int vvar = 333;     /* must survive longjmp */

    if (setjmp(env) == 0) {
        nvar = 777;
        rvar = 888;
        vvar = 999;
        do_jump(nvar, rvar, vvar);
    } else {
        printf("after longjmp(): nvar=%d rvar=%d vvar=%d\n", nvar, rvar, vvar);
        printf("(nvar/rvar may revert under -O; vvar should stay 999)\n");
    }
    return 0;
}
