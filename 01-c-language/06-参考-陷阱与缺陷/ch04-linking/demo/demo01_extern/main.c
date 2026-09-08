#include <stdio.h>
#include "common.h"

int main(void)
{
    bump();
    bump();
    printf("shared_val=%d (one definition in def.c)\n", shared_val);
    return 0;
}
