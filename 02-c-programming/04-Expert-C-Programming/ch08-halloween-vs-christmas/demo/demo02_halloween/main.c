#include <stdio.h>

int main(void)
{
    int a, b = 5;

    if (a = b == 0) {
        printf("Halloween trap: b==0, a=%d\n", a);
    } else {
        printf("Halloween trap: b!=0, a=%d (a 被赋值为 b==0 的 0/1)\n", a);
    }

    if ((a = b) == 0) {
        printf("correct: a is 0\n");
    } else {
        printf("correct: a = b = %d\n", a);
    }
    return 0;
}
