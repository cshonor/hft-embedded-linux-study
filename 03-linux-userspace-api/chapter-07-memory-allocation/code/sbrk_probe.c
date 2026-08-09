/* Minimal demo: sbrk(0) query and grow/shrink program break. */
#include <stdio.h>
#include <unistd.h>

int main(void) {
    void *before = sbrk(0);
    printf("break before: %p\n", before);

    void *old = sbrk(4096);
    if (old == (void *)-1) {
        perror("sbrk(+4096)");
        return 1;
    }
    printf("sbrk(+4096) returned old break: %p\n", old);
    printf("break after grow: %p\n", sbrk(0));

    if (sbrk(-4096) == (void *)-1) {
        perror("sbrk(-4096)");
        return 1;
    }
    printf("break after shrink: %p\n", sbrk(0));
    printf("(do not use brk/sbrk in application code — malloc instead)\n");
    return 0;
}
