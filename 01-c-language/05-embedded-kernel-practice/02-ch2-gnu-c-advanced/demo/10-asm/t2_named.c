/* T2: four-part structure + named operands %[name] vs %0/%1
 *
 * Full form:
 *   asm [volatile] ( "template"
 *       : output_operands   // comma-separated, "=r"(x), [name]"=r"(x)
 *       : input_operands
 *       : clobbers
 *       : labels            // asm goto only
 *   );
 *
 * Operand references: %0, %1, ... (positional) or %[name] (symbolic).
 * Symbolic is robust to reordering; kernel uses both.
 */
#include <stdio.h>

/* positional: %0 = out, %1 = in_a, %2 = in_b.
 * NOTE: ARM `add dst,s1,s2` is 3-operand; x86 is 2-operand (add src,dst).
 * Here we use mov+add to keep it x86-runnable; ARM code would use one insn. */
static int pos_add(int a, int b)
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

/* named: %[sum], %[x], %[y] — reorder operands without rewriting template */
static int named_add(int a, int b)
{
    int s;
    asm volatile (
        "mov %[x], %[sum]\n\t"
        "add %[y], %[sum]"
        : [sum] "=r" (s)
        : [x] "r" (a), [y] "r" (b)
    );
    return s;
}

/* Compile with -DDEMO_BAD to see the mixed positional/named diagnostic */
#if defined(DEMO_BAD)
static int mix_try(int a)
{
    int s;
    /* intentionally wrong: positional %0 + named [x] in same asm */
    asm volatile (
        "add %0, %0, %1"
        : "=r" (s)
        : [x] "r" (a)
    );
    return s;
}
#endif

int main(void)
{
    printf("pos_add(3,5)  = %d\n", pos_add(3, 5));
    printf("named_add(3,5)= %d\n", named_add(3, 5));
#if defined(DEMO_BAD)
    printf("mix_try(7)    = %d\n", mix_try(7));
#endif
    return 0;
}
