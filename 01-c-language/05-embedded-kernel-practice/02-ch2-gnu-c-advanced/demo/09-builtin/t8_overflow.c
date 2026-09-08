/* T8: overflow builtins (GCC 5+, Clang 3.5+) */
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

/* checked addition */
__attribute__((noinline))
int safe_add(int a, int b, int *result)
{
    return __builtin_add_overflow(a, b, result);
}

/* checked multiplication */
__attribute__((noinline))
int safe_mul(int a, int b, int *result)
{
    return __builtin_mul_overflow(a, b, result);
}

/* unsigned wrap: does it flag? */
__attribute__((noinline))
int safe_add_u(uint32_t a, uint32_t b, uint32_t *r)
{
    return __builtin_add_overflow(a, b, r);
}

/* all overflow variants */
__attribute__((noinline))
void test_all_overflow(void)
{
    int r;
    uint32_t ru;

    /* signed: INT_MAX + 1 */
    printf("INT_MAX+1 overflow? %d (result=%d)\n",
           __builtin_add_overflow(INT_MAX, 1, &r), r);

    /* signed: INT_MIN - 1 */
    printf("INT_MIN-1 overflow? %d (result=%d)\n",
           __builtin_sub_overflow(INT_MIN, 1, &r), r);

    /* unsigned: UINT_MAX + 1 */
    printf("UINT_MAX+1 overflow? %d (result=%u)\n",
           __builtin_add_overflow(UINT_MAX, 1u, &ru), ru);

    /* mul: big numbers */
    printf("1000000*1000000 overflow? %d\n",
           __builtin_mul_overflow(1000000, 1000000, &r));

    printf("2000000000*2 overflow? %d (result=%d)\n",
           __builtin_mul_overflow(2000000000, 2, &r), r);

    /* different result type: what happens? */
    uint8_t r8;
    printf("300+300 -> uint8 overflow? %d (result=%u)\n",
           __builtin_add_overflow(300, 300, &r8), (unsigned)r8);

    /* does it work with 64-bit? */
    int64_t r64;
    printf("INT64_MAX+1 overflow? %d\n",
           __builtin_add_overflow((int64_t)INT64_MAX, (int64_t)1, &r64));
}

int main(void)
{
    int r;
    printf("safe_add(2,3) overflow=%d result=%d\n", safe_add(2, 3, &r), r);
    printf("safe_add(INT_MAX,1) overflow=%d result=%d\n",
           safe_add(INT_MAX, 1, &r), r);
    printf("safe_mul(INT_MAX,2) overflow=%d result=%d\n",
           safe_mul(INT_MAX, 2, &r), r);

    uint32_t ru;
    printf("safe_add_u(0xFFFFFFFF,1) overflow=%d result=%u\n",
           safe_add_u(0xFFFFFFFFu, 1u, &ru), ru);

    test_all_overflow();
    return 0;
}
