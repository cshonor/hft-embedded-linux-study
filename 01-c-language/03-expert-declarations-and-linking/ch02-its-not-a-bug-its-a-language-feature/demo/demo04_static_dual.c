#include <stdio.h>

/* static 双重语义演示 */
static int file_scope = 10;

static void counter(void)
{
    static int calls;
    calls++;
    printf("static local calls=%d\n", calls);
}

int main(void)
{
    counter();
    counter();
    printf("file static=%d\n", file_scope);
    return 0;
}
