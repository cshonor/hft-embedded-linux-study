#include "demo_log.h"

int main(void)
{
    LOG("boot ok");
    LOG("value=%d", 42);
    LOG("max=%d", MAX(3, 7));
    return 0;
}
