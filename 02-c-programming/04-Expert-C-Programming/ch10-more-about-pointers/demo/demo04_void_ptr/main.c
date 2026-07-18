#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int a = 42;
    void *vp = &a;

    /* *(int*)vp 强制转型后解引用 */
    printf("void*: val=%d\n", *(int *)vp);

    /* malloc 返回 void* */
    int *p = malloc(sizeof *p);
    if (p) {
        *p = 100;
        printf("malloc void* -> int*: %d\n", *p);
        free(p);
    }

    return 0;
}
