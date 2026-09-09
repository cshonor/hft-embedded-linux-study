#include <stdio.h>

static void print_row(int arr[][4], int row)
{
    for (int j = 0; j < 4; j++)
        printf("%d ", arr[row][j]);
    printf("\n");
}

int main(void)
{
    int m[2][4] = {{1, 2, 3, 4}, {5, 6, 7, 8}};
    int (*p)[4] = m;

    printf("=== 二维数组传参 ===\n");
    print_row(m, 0);
    print_row(m, 1);

    printf("\n=== 指针运算步长 ===\n");
    printf("m=%p m[0]=%p m[1]=%p\n", (void *)m, (void *)m[0], (void *)m[1]);
    printf("p=%p p+1=%p  步长=%td bytes (=4*sizeof(int))\n",
           (void *)p, (void *)(p + 1),
           (char *)(p + 1) - (char *)p);

    printf("\n=== sizeof ===\n");
    printf("sizeof(m)=%zu sizeof(m[0])=%zu sizeof(p)=%zu\n",
           sizeof m, sizeof m[0], sizeof p);

    return 0;
}
