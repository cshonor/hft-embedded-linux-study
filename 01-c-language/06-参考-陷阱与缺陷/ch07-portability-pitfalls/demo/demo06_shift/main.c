#include <stdio.h>
#include <stdint.h>

int main(void)
{
    int sx = -1;
    int sr = sx >> 1;

    uint32_t ux = (uint32_t)(int32_t)sx;
    ux >>= 1;

    printf("signed:  -1 >> 1 = %d (implementation-defined sign extension)\n", sr);
    printf("unsigned path: (uint32_t)(int32_t)-1 >> 1 = 0x%08x\n", ux);
    printf("portable bit ops: prefer unsigned types\n");

    return 0;
}
