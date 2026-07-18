#include <stdio.h>

int main(void)
{
    int flag = 0;
    int a = 0, b = 0;

    if (flag)
        a = 100;
    b = 200;

    printf("flag=0: a=%d b=%d (b always 200)\n", a, b);

    flag = 1;
    a = b = 0;
    if (flag) {
        a = 100;
        b = 200;
    }
    printf("flag=1 with braces: a=%d b=%d\n", a, b);
    return 0;
}
