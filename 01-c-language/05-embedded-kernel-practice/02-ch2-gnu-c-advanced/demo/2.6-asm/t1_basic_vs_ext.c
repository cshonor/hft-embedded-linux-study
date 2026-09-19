/* T1: basic asm vs extended asm — the % trap
 *
 * basic asm:   asm("...");            — no operands, % is literal
 * extended asm: asm("..." : out : in : clobber);  — % starts an operand ref
 *
 * gcc doc: in basic asm, to emit a literal % you write %.
 *          in extended asm, to emit a literal % you write %%.
 */
#include <stdio.h>

/* (A) basic asm: % is literal, no operands allowed.
 * In basic asm (no colon-separated operands), gcc treats % as a literal
 * character — it is NOT an operand reference. (In extended asm, %0 is.) */
static void basic_demo(void)
{
    asm("nop");                 /* simplest basic asm */
    /* To emit a literal % in extended asm you write %%; in basic asm, just %.
     * We don't emit % here to avoid injecting bytes into .text. */
    printf("basic asm ran (%% is literal in basic, %% is literal %% in ext)\n");
}

/* (B) extended asm: %0 refers to operand 0; %% is a literal %.
 * NOTE: ARM `add dst, src1, src2` is 3-operand; x86 `add` is 2-operand
 *       (add src, dst => dst += src). We use mov+add to emulate. */
static int add_extended(int a, int b)
{
    int s;
    asm volatile (
        "mov %1, %0\n\t"
        "add %2, %0"
        : "=r" (s)
        : "r" (a), "r" (b)
    );
    return s;
}

/* (C) the classic trap: basic-asm-looking but actually extended, %0 vs % */
static int literal_percent(void)
{
    int x = 0;
    /* Want to emit "mov $42, %0" where %0 is operand.
       But if we also need a literal % in the asm (rare), use %% */
    asm volatile (
        "mov $42, %0"        /* %0 = operand 0 */
        : "=r" (x)
    );
    return x;
}

int main(void)
{
    basic_demo();
    printf("add_extended(3,5) = %d\n", add_extended(3, 5));
    printf("literal_percent() = %d\n", literal_percent());
    return 0;
}
