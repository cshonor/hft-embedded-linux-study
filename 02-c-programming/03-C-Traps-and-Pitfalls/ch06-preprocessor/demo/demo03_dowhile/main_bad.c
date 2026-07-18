#include <stdio.h>

#define BAD_STEP() printf("step1\n"); printf("step2\n")

int main(void)
{
    int flag = 0;

    if (flag)
        BAD_STEP();
    else
        printf("else\n");

    return 0;
}
