#include <stdio.h>

int g_arr[10];

void test(int arr[])
{
    printf("函数内 sizeof(arr)=%zu\n", (size_t)sizeof(arr));
}

int main(void)
{
    int local[10];

    printf("全局 sizeof(g_arr)=%zu\n", sizeof(g_arr));
    printf("局部 sizeof(local)=%zu\n", sizeof(local));

    printf("local=%p  &local=%p\n", (void *)local, (void *)&local);
    printf("local+1=%p  &local+1=%p\n",
           (void *)(local + 1), (void *)(&local + 1));

    test(local);
    return 0;
}
