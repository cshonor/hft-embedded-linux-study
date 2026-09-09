#include <stdio.h>

int main(void)
{
    unsigned val = 0x0F;
    unsigned mask = 0x03;

    /* 错误：== 高于 & → val & (mask == 1) */
    if (val & mask == 1)
        printf("wrong grouping: val & mask == 1 为真\n");
    else
        printf("wrong grouping: val & mask == 1 为假\n");

    /* 正确 */
    if ((val & mask) == 1)
        printf("correct: (val & mask) == 1 → %u\n", val & mask);
    else
        printf("correct: (val & mask) == %u\n", val & mask);

    return 0;
}
