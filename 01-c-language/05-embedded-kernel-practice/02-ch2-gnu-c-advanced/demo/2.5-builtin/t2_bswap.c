/* T2: byte swap builtins */
#include <stdio.h>
#include <stdint.h>

__attribute__((noinline)) uint16_t do_bswap16(uint16_t v) { return __builtin_bswap16(v); }
__attribute__((noinline)) uint32_t do_bswap32(uint32_t v) { return __builtin_bswap32(v); }
__attribute__((noinline)) uint64_t do_bswap64(uint64_t v) { return __builtin_bswap64(v); }

int main(void)
{
    printf("bswap16(0x1234) = 0x%04X\n", do_bswap16(0x1234));
    printf("bswap32(0x12345678) = 0x%08X\n", do_bswap32(0x12345678u));
    printf("bswap64(0x0123456789ABCDEF) = 0x%016llX\n",
           (unsigned long long)do_bswap64(0x0123456789ABCDEFULL));

    /* constant folding */
    printf("--- constant ---\n");
    printf("bswap32(0xAABBCCDD) = 0x%08X (compile-time)\n", __builtin_bswap32(0xAABBCCDDu));
    return 0;
}
