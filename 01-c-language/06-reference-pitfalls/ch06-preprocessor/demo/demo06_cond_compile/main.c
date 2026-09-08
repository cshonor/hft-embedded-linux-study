#include <stdio.h>
#include "demo_cfg.h"

int main(void)
{
    printf("mode: %s\n", mode);
    printf("build with: make debug=1  vs  make debug=0\n");
    return 0;
}
