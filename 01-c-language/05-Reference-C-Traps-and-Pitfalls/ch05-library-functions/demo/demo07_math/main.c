#include <math.h>
#include <stdio.h>

int main(void)
{
    int x = (int)sqrt(4.0);
    double y = sqrt(2.0);

    printf("int x = (int)sqrt(4) => %d\n", x);
    printf("double y = sqrt(2) => %.17g\n", y);
    printf("truncation: (int)sqrt(2) => %d\n", (int)sqrt(2.0));

    return 0;
}
