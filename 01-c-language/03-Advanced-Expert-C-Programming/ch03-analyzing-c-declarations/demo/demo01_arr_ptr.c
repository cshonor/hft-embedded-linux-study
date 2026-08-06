#include <stdio.h>

typedef char (*Arr5)[5];

int main(void)
{
    char buf[5] = "abc";
    Arr5 p = &buf;
    printf("(*p)[0]=%c\n", (*p)[0]);
    return 0;
}
