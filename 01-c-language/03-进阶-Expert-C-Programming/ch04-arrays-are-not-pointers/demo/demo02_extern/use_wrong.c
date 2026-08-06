#include <stdio.h>

extern char *arr; /* 错误：与 char arr[] 定义不匹配 */

int main(void)
{
    puts("demo02_wrong: may SIGSEGV — treats string bytes as pointer");
    printf("arr as pointer=%p\n", (void *)arr);
    printf("*arr=%c\n", *arr);
    return 0;
}
