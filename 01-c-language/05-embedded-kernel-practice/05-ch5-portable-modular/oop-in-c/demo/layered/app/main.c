#include "../abstract/device.h"

int app_run(void);

int main(void)
{
    return app_run() == (int)(sizeof("layered-arch") - 1) ? 0 : 1;
}
