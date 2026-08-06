#ifndef MOD_LOG_H
#define MOD_LOG_H

#include <stdio.h>

#ifndef LOG_ENABLE
#define LOG_ENABLE 1
#endif

#define LOG_INFO(fmt, ...) \
    do { if (LOG_ENABLE) printf("[INFO %s] " fmt "\n", __func__, ##__VA_ARGS__); } while (0)

#define LOG_ERR(fmt, ...) \
    do { if (LOG_ENABLE) fprintf(stderr, "[ERR  %s] " fmt "\n", __func__, ##__VA_ARGS__); } while (0)

#endif
