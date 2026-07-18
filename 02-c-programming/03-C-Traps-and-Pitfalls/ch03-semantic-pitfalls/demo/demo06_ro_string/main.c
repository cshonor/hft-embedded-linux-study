#include <stdio.h>

int main(void)
{
    char *p = "hello";
    char arr[] = "hello";

    printf("literal p=%p arr=%p\n", (void *)p, (void *)arr);
    arr[0] = 'H';
    printf("arr after modify: %s\n", arr);
    /* p[0]='H'; 取消注释 → 段错误 / UB */
    return 0;
}
