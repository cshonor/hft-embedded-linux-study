#ifndef DEMO_LOG_H
#define DEMO_LOG_H

#include <stdio.h>

#define LOG(fmt, ...) \
    printf("[LOG %s:%d %s] " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define MAX(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a > _b ? _a : _b; \
})

#endif
