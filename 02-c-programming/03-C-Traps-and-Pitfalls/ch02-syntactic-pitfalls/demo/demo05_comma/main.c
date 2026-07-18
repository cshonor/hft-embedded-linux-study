#include <stdio.h>

int main(void)
{
    int x = (1, 2, 3);
    printf("comma op: x = (1,2,3) -> %d\n", x);

    int a = 0, b = 5;
    if (a++, b < 100)
        printf("if (a++, b<100): a=%d b=%d -> true\n", a, b);

    for (int i = 0, j = 3; i < j; i++, j--)
        printf("for comma init/step: i=%d j=%d\n", i, j);

    return 0;
}
