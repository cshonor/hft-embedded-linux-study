/* Listing 6-3 style: print addresses of different storage classes. */
#include <stdio.h>
#include <stdlib.h>

char glob_buf[65536];           /* BSS (uninitialized) */
int primes[] = {2, 3, 5, 7};    /* Data (initialized) */

static void f(void) {
    int local;                  /* stack */
    printf("local (stack)     %p\n", (void *)&local);
}

int main(void) {
    void *heap = malloc(100);

    printf("text (main)       %p\n", (void *)(void (*)(void))main);
    printf("initialized data  %p\n", (void *)primes);
    printf("BSS               %p\n", (void *)glob_buf);
    printf("heap              %p\n", heap);
    f();
    printf("expect roughly: text < data < bss < heap < stack\n");

    free(heap);
    return 0;
}
