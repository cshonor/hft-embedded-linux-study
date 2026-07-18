#include <stdio.h>

static void wrong_style(int a, int b)
{
    if (a > 0)
        if (b > 0)
            puts("  inner: a>0 and b>0");
        else
            puts("  inner-else: bound to if(b>0) -> b<=0");
}

static void fixed_style(int a, int b)
{
    if (a > 0) {
        if (b > 0)
            puts("  fixed: a>0 and b>0");
    } else {
        puts("  fixed-else: a<=0");
    }
}

int main(void)
{
    puts("a=1 b=-1 (looks like outer else case):");
    wrong_style(1, -1);

    puts("a=-1 b=1:");
    fixed_style(-1, 1);
    return 0;
}
