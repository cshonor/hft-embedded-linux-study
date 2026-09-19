/* T11: GCC vs Clang builtin support matrix - fixed */
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

int main(void)
{
    /* these should work on both gcc and clang */
    int v = __builtin_popcount(0xFFu);
    uint32_t b = __builtin_bswap32(0x12345678u);
    int c = __builtin_constant_p(5);
    long e = __builtin_expect(1, 1);

    /* overflow: gcc 5+, clang 3.5+ */
    int r;
    int ovf = __builtin_add_overflow(INT_MAX, 1, &r);

    /* types_compatible_p: gcc only? or clang too? */
    int t = __builtin_types_compatible_p(int, int);

    printf("popcount=%d bswap=%u constant_p=%d expect=%ld overflow=%d types=%d\n",
           v, b, c, e, ovf, t);

    /* __builtin_choose_expr: gcc extension, clang? */
    int x = 5;
    int y = __builtin_choose_expr(
        __builtin_constant_p(x),
        x * 2,
        -1
    );
    printf("choose_expr(var x) = %d\n", y);

    int y2 = __builtin_choose_expr(
        __builtin_constant_p(10),
        10 * 2,
        -1
    );
    printf("choose_expr(const 10) = %d\n", y2);

    /* __builtin_assume_aligned: both? */
    int *p = __builtin_assume_aligned((int*)0x1000, 16);
    (void)p;

    /* __builtin_unreachable: both */
    /* __builtin_trap: both */

    return 0;
}
