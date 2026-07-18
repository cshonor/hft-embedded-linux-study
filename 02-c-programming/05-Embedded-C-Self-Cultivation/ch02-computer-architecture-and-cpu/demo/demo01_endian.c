#include <stdio.h>
#include <stdint.h>

static int is_little_endian(void)
{
    uint32_t x = 0x01020304;
    return *(uint8_t *)&x == 0x04;
}

int main(void)
{
    uint32_t v = 0x12345678;
    printf("CPU endian: %s\n", is_little_endian() ? "little" : "big");
    printf("&v = %p\n", (void *)&v);
    printf("Use gdb: x/4xb &v\n");
    return 0;
}
