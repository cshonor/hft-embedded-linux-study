#include <stdio.h>

int main(void)
{
    printf("decimal 10 = %d\n", 10);
    printf("octal   010 = %d (not 10!)\n", 010);
    printf("hex   0x10 = %d\n", 0x10);
    printf("float .5 = %f, 5. = %f\n", .5, 5.);
    return 0;
}
