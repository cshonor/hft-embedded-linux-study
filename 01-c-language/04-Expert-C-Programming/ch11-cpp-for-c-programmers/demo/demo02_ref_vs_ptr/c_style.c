#include <stdio.h>

static void set_via_double_ptr(int **pp)
{
    **pp = 100;
}

int main(void)
{
    int x = 0;
    int *px = &x;
    set_via_double_ptr(&px);
    printf("C double ptr: x=%d\n", x);
    return 0;
}
