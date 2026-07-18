#include <stdlib.h>

/* 故意泄漏：供 valgrind 检测 */
int main(void)
{
    void *p = malloc(1024);
    (void)p;
    return 0;
}
