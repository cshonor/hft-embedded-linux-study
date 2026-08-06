#include <stdio.h>

#define SQUARE(x) ((x) * (x))
#define BAD_SQUARE(x) (x * x)

int main(void)
{
    int a = 3;
    printf("SQUARE(a+1)=%d\n", SQUARE(a + 1));       /* 16 */
    printf("BAD_SQUARE(a+1)=%d\n", BAD_SQUARE(a + 1)); /* 7: 展开为 a+1*a+1 */
    return 0;
}
