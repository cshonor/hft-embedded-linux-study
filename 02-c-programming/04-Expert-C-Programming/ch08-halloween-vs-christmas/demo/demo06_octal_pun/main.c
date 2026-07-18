#include <stdio.h>

int main(void)
{
    int halloween_oct = 031; /* 八进制 31 = 十进制 25 */
    int christmas_dec = 25;

    printf("Halloween vs Christmas (book pun):\n");
    printf("  031 (oct) = %d\n", halloween_oct);
    printf("  25  (dec) = %d\n", christmas_dec);
    printf("  equal? %s\n", halloween_oct == christmas_dec ? "yes" : "no");
    return 0;
}
