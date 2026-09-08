/* T6: control flow builtins - trap, unreachable, abort */
#include <stdio.h>
#include <stdlib.h>

/* __builtin_trap: generates ud2 (x86) / UDF (ARM) */
__attribute__((noinline))
void do_trap(void)
{
    printf("before trap\n");
    __builtin_trap();
    printf("after trap (unreachable)\n");  /* dead code */
}

/* __builtin_unreachable: tells compiler this path never taken */
__attribute__((noinline))
int do_unreachable(int x)
{
    switch (x) {
    case 0:  return 0;
    case 1:  return 1;
    case 2:  return 2;
    }
    __builtin_unreachable();  /* all cases covered, no default needed */
}

/* what if unreachable IS reached? */
__attribute__((noinline))
int lie_to_compiler(int x)
{
    if (x > 0) return x;
    __builtin_unreachable();  /* "x is always > 0" -- but what if not? */
}

/* __builtin_abort vs abort() */
__attribute__((noinline))
void do_builtin_abort(void)
{
    __builtin_abort();
}

int main(void)
{
    printf("unreachable(1) = %d\n", do_unreachable(1));
    printf("lie(5) = %d\n", lie_to_compiler(5));
    /* do_trap(); -- would crash */
    /* do_builtin_abort(); */
    return 0;
}
