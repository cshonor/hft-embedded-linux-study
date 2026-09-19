/* T13: popcount with -mpopcnt vs without */
#include <stdio.h>
#include <stdint.h>

__attribute__((noinline))
int do_popcount_default(uint32_t v) { return __builtin_popcount(v); }

__attribute__((noinline))
int do_popcount_popcnt(uint32_t v) {
    /* Use explicit popcnt if available */
#ifdef __POPCNT__
    return __builtin_popcount(v);
#else
    return __builtin_popcount(v);
#endif
}

int main(void)
{
    uint32_t v = 0xF0F0F0F0u;
    printf("popcount(0x%08X) = %d\n", v, do_popcount_default(v));
    return 0;
}
