#include <stdio.h>

extern char msg[];

int main(void)
{
    printf("correct extern char msg[]: [%s]\n", msg);
    return 0;
}
