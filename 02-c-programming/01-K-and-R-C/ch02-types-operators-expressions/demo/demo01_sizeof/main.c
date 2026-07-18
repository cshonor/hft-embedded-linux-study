#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    printf("=== basic types (bytes) ===\n");
    printf("char=%zu short=%zu int=%zu long=%zu long long=%zu\n",
           sizeof(char), sizeof(short), sizeof(int),
           sizeof(long), sizeof(long long));
    printf("float=%zu double=%zu void*=%zu\n",
           sizeof(float), sizeof(double), sizeof(void *));

    printf("\n=== stdint fixed-width ===\n");
    printf("uint8_t=%zu int32_t=%zu int64_t=%zu\n",
           sizeof(uint8_t), sizeof(int32_t), sizeof(int64_t));

    printf("\nlong size: %zu (platform-dependent)\n", sizeof(long));

    return 0;
}
