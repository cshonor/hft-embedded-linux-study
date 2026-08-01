#include <stdio.h>

void test_sign_unsigned(void)
{
    unsigned int u = 5;
    int i = -1;

    printf("u=%u i=%d\n", u, i);
    if (u > i)
        puts("u > i");
    else
        puts("u <= i  (signed promoted to unsigned)");
}

int main(void)
{
    test_sign_unsigned();
    return 0;
}
