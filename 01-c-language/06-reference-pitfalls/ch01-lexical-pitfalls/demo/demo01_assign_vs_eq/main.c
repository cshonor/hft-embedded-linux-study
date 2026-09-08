#include <stdio.h>

int main(void)
{
    int x = 5;

    if (x = 0)
        printf("assign: branch taken (unexpected)\n");
    else
        printf("assign: x=%d after if(x=0)\n", x);

    x = 5;
    if (x == 0)
        printf("compare: zero\n");
    else
        printf("compare: x=%d (unchanged intent)\n", x);

    return 0;
}
