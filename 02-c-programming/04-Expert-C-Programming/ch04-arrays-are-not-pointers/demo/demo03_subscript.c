#include <stdio.h>

int main(void)
{
    int arr[] = {10, 20, 30};
    int *p = arr;

    printf("arr[1]=%d  1[arr]=%d  p[1]=%d\n", arr[1], 1[arr], p[1]);
    printf("*(arr+1)=%d\n", *(arr + 1));
    return 0;
}
