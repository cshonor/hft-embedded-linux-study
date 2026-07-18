#include <stdio.h>

static void by_ptr(int *arr)
{
    printf("  in func: sizeof(arr)=%zu (pointer)\n", sizeof arr);
    (void)arr;
}

int main(void)
{
    int arr[10];
    int *p = arr;

    printf("sizeof(arr)=%zu\n", sizeof arr);
    printf("sizeof(&arr)=%zu\n", sizeof &arr);
    printf("sizeof(p)=%zu\n", sizeof p);
    by_ptr(arr);
    return 0;
}
