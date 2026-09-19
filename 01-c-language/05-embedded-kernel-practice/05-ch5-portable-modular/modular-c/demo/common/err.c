#include "err.h"

const char *err_str(err_t e)
{
    switch (e) {
    case ERR_OK:      return "OK";
    case ERR_INVAL:   return "EINVAL";
    case ERR_NOMEM:   return "ENOMEM";
    case ERR_IO:      return "EIO";
    case ERR_TIMEOUT: return "ETIMEDOUT";
    default:          return "UNKNOWN";
    }
}
