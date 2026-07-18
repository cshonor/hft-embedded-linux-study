#include <stdio.h>

static char *get_bad_ptr(void)
{
    char buf[8] = "123456";
    return buf; /* 栈帧销毁后失效 */
}

int main(void)
{
    char *p = get_bad_ptr();
    /* 未定义行为：可能乱码、段错误或「看似正常」 */
    printf("dangling: [%s]\n", p);
    return 0;
}
