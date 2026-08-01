#include <stdio.h>
#include <string.h>

static void overflow_demo(void)
{
    volatile int guard = 0;
    char buf[4];

    printf("before: guard=%d\n", guard);
    strcpy(buf, "overflow"); /* no length check */
    printf("after:  guard=%d (adjacent stack corrupted)\n", guard);
}

int main(void)
{
    overflow_demo();
    return 0;
}
