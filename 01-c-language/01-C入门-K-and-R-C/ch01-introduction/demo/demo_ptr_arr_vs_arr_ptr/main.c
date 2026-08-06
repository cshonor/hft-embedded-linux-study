#include <stdio.h>

int main(void)
{
    /* 指针数组：元素是指针 */
    const char *arr[] = { "A", "B", "C" };
    printf("pointer array: arr[0]=%s arr[1]=%s\n", arr[0], arr[1]);

    /* 数组指针：指向整块 char[16] */
    char buf[16] = "test";
    char (*ptr)[16] = &buf;
    printf("array pointer: *ptr=%s  (*ptr)[0]=%c\n", *ptr, (*ptr)[0]);

    printf("sizeof(arr)=%zu (3 pointers)\n", sizeof(arr));
    printf("sizeof(ptr)=%zu (one pointer)\n", sizeof(ptr));
    printf("sizeof(*ptr)=%zu (whole char[16])\n", sizeof(*ptr));
    return 0;
}
