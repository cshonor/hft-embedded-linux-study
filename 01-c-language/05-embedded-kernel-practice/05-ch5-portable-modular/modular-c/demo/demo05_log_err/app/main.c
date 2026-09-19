#include "../../common/err.h"
#include "../../common/log.h"
#include <stdio.h>

int main(void)
{
    err_t codes[] = { ERR_OK, ERR_INVAL, ERR_NOMEM, ERR_IO };

    for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
        if (codes[i] == ERR_OK)
            LOG_INFO("status %s", err_str(codes[i]));
        else
            LOG_ERR("status %s", err_str(codes[i]));
    }
    return 0;
}
