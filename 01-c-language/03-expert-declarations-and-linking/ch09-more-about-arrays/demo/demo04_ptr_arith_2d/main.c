#include <stdio.h>

int main(void)
{
    int arr[3][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12},
    };

    int (*row_ptr)[4] = arr;   /* 数组指针：指向一行 int[4] */
    int *elem_ptr = arr[0];    /* 指向首元素，等价 &arr[0][0] */

    printf("=== 层级地址 ===\n");
    printf("arr       %p  type int[3][4]\n", (void *)arr);
    printf("arr[0]    %p  type int*\n", (void *)arr[0]);
    printf("arr[0][1] %p  value %d\n", (void *)&arr[0][1], arr[0][1]);

    printf("\n=== row_ptr+1 跳过整行 ===\n");
    printf("row_ptr   %p\n", (void *)row_ptr);
    printf("row_ptr+1 %p  step=%td\n",
           (void *)(row_ptr + 1),
           (char *)(row_ptr + 1) - (char *)row_ptr);

    printf("\n=== elem_ptr+1 跳过一个 int ===\n");
    printf("elem_ptr   %p arr[0][1]=%d\n", (void *)elem_ptr, elem_ptr[1]);
    printf("elem_ptr+1 %p arr[0][2]=%d\n", (void *)(elem_ptr + 1), elem_ptr[1]);

    printf("\n=== &arr vs arr ===\n");
    printf("&arr+1 step=%td (=sizeof whole 2D)\n",
           (char *)(&arr + 1) - (char *)&arr);

    return 0;
}
