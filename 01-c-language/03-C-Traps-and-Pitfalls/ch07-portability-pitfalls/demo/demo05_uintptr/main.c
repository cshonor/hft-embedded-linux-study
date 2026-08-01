#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    void *p = malloc(16);
    uintptr_t u = (uintptr_t)p;
    void *q = (void *)u;

    printf("sizeof(void*)=%zu sizeof(int)=%zu sizeof(uintptr_t)=%zu\n",
           sizeof(void *), sizeof(int), sizeof(uintptr_t));

    if (sizeof(void *) > sizeof(int))
        puts("never store pointers in int on 64-bit — use uintptr_t");

    printf("round-trip ptr %p -> 0x%zx -> %p\n", p, u, q);

    free(p);
    return 0;
}
