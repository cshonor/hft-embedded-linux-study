#include "foo.h"
#include <stdio.h>

int main(void)
{
    printf("%s\n", foo_version());
    printf("foo_add(2,3)=%d\n", foo_add(2, 3));
    return 0;
}
