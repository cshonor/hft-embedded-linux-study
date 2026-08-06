#include "common.h"

int shared_val = 0;

void bump(void)
{
    shared_val++;
}
