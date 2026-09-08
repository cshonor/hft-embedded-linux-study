#include <stdio.h>

void counter(void)
{
    static int c;
    int local = 0;
    c++;
    local++;
    printf("static=%d local=%d\n", c, local);
}

int main(void)
{
    counter();
    counter();
    counter();
    return 0;
}
