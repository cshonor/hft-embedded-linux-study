#include <stdio.h>

int main(void)
{
    unsigned x = 0x06; /* bit1、bit2 置位 */

    if (x & 0x02 == 2)
        printf("wrong: (x & 0x02==2) taken\n");
    else
        printf("wrong: (x & 0x02==2) NOT taken (mask==2 is false -> x&0)\n");

    if ((x & 0x02) == 0x02)
        printf("correct: bit1 set\n");
    else
        printf("correct: bit1 clear\n");

    return 0;
}
