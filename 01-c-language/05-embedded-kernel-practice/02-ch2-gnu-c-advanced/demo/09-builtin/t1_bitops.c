/* T1: bit manipulation builtins - do they map to hardware instructions? */
#include <stdio.h>
#include <stdint.h>

/* prevent inlining so we can see the actual instruction */
__attribute__((noinline))
int do_popcount(uint32_t v)  { return __builtin_popcount(v); }

__attribute__((noinline))
int do_popcountll(uint64_t v) { return __builtin_popcountll(v); }

__attribute__((noinline))
int do_clz(uint32_t v)   { return __builtin_clz(v); }

__attribute__((noinline))
int do_ctz(uint32_t v)   { return __builtin_ctz(v); }

__attribute__((noinline))
int do_ffs(uint32_t v)   { return __builtin_ffs(v); }

__attribute__((noinline))
int do_parity(uint32_t v) { return __builtin_parity(v); }

__attribute__((noinline))
int do_clzl(unsigned long v) { return __builtin_clzl(v); }

int main(void)
{
    uint32_t v = 0xF0F0F0F0u;
    printf("popcount(0x%08X) = %d\n", v, do_popcount(v));
    printf("popcountll(0x%016llX) = %d\n", (unsigned long long)v, do_popcountll(v));
    printf("clz(0x%08X) = %d\n", v, do_clz(v));
    printf("ctz(0x%08X) = %d\n", v, do_ctz(v));
    printf("ffs(0x%08X) = %d\n", v, do_ffs(v));
    printf("parity(0x%08X) = %d\n", v, do_parity(v));
    printf("clzl(0x%08lX) = %d\n", (unsigned long)v, do_clzl(v));

    /* constant folding: compile-time evaluation */
    printf("--- constant folding ---\n");
    printf("popcount(0xFF)  = %d (compile-time)\n", __builtin_popcount(0xFF));
    printf("clz(1)          = %d (compile-time)\n", __builtin_clz(1u));
    printf("ctz(0x80)       = %d (compile-time)\n", __builtin_ctz(0x80u));
    printf("ffs(0)          = %d (UB!)\n", __builtin_ffs(0));
    return 0;
}
