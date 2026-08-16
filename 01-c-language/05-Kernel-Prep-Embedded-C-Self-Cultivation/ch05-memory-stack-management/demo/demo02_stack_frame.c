#include <stdio.h>

static int depth;

void recurse(int n)
{
    int frame = n;
    depth = n;
    printf("depth=%d frame@%p sp nearby\n", n, (void *)&frame);
    if (n > 0)
        recurse(n - 1);
}

int main(void)
{
    recurse(4);
    return 0;
}
