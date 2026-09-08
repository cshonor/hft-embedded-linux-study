#include <stdio.h>

int main(void)
{
    char c = '0';

    printf("c=%d ('0' ASCII)\n", c);
    printf("c=='0': %d\n", c == '0');
    printf("c==\"0\": %d (pointer compare, usually 0)\n", c == "0");

    return 0;
}
