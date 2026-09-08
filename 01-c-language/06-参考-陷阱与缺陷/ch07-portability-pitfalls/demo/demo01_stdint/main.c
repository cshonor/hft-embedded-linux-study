#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    printf("sizeof char=%zu short=%zu int=%zu long=%zu long long=%zu void*=%zu\n",
           sizeof(char), sizeof(short), sizeof(int), sizeof(long),
           sizeof(long long), sizeof(void *));

    int64_t ts = 1700000000000LL;
    printf("int64_t timestamp=%" PRId64 "\n", ts);

    printf("use uint64_t/int64_t for wire format, not long\n");

    return 0;
}
