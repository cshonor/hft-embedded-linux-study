#include <stdio.h>
#include <stdlib.h>

static void use_after_check(int *p)
{
    if (!p) {
        puts("malloc failed, skip dereference");
        return;
    }
    *p = 42;
    printf("*p=%d\n", *p);
}

int main(void)
{
    int *p = malloc(sizeof(int));
    use_after_check(p);
    free(p);
    free(NULL); /* always safe */
    puts("free(NULL) OK");

    /* double free — run: make -C demo04_malloc double_free */
    return 0;
}
