#include <stdio.h>

int main(void)
{
    int* a, b;   /* trap style: only a is a pointer */
    int  *c, *d; /* clear: both pointers */

    a = &b;
    b = 7;
    *a = 42;

    printf("sizeof(a)=%zu sizeof(b)=%zu\n", sizeof(a), sizeof(b));
    printf("a points to b; b=%d *a=%d\n", b, *a);

    c = &b;
    d = &b;
    printf("c and d both pointers: *c=%d *d=%d\n", *c, *d);
    return 0;
}
