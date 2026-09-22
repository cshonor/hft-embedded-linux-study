#ifndef MOD_ERR_H
#define MOD_ERR_H

typedef enum {
    ERR_OK       = 0,
    ERR_INVAL    = -1,
    ERR_NOMEM    = -2,
    ERR_IO       = -3,
    ERR_TIMEOUT  = -4,
} err_t;

const char *err_str(err_t e);

#endif
