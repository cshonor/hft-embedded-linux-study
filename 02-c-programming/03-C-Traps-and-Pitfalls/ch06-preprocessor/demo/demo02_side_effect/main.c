#include <stdio.h>

#define SQUARE(x) ((x) * (x))

static inline int square_inline(int x)
{
    return x * x;
}

int main(void)
{
    int i = 2;

    int r_macro = SQUARE(i++);
    printf("after SQUARE(i++): i=%d res=%d (i incremented twice)\n", i, r_macro);

    i = 2;
    int r_inline = square_inline(i++);
    printf("after square_inline(i++): i=%d res=%d\n", i, r_inline);

    return 0;
}
