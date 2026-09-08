#include <stdio.h>

int main(void)
{
    unsigned i;

    printf("unsigned i after i-- at 0 wraps to %u\n", 0u - 1);
    printf("simulating for(i=3; i>=0; i--) without infinite loop:\n");
    for (i = 3; i <= 3; i--) {
        printf("  i=%u\n", i);
        if (i == 0)
            break; /* 真实代码无 break → 死循环 */
    }
    printf("next i-- would wrap to %u → loop never ends\n", 0u - 1);
    return 0;
}
