#include <stdio.h>

void counter(void)
{
    int auto_cnt = 0;
    static int static_cnt;

    auto_cnt++;
    static_cnt++;
    printf("auto_cnt=%d static_cnt=%d\n", auto_cnt, static_cnt);
}

int main(void)
{
    counter();
    counter();
    counter();
    return 0;
}
