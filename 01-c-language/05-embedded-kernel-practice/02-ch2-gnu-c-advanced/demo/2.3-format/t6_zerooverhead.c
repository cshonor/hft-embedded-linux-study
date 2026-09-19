/* T6: can a logging call really cost zero? (HFT angle) */
#include <stdio.h>
#include <stdarg.h>

#define MAX_LEVEL 1
enum { L_ERR = 0, L_WARN = 1, L_INFO = 2, L_DEBUG = 3 };

int g_level = 1;

void log_at(int level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void log_at(int level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[%d] ", level);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

/* compile-time gate: MAX_LEVEL is a constant */
#define LOG_DBG(fmt, ...)                                   \
    do {                                                    \
        if (L_DEBUG <= MAX_LEVEL)                           \
            log_at(L_DEBUG, fmt, ##__VA_ARGS__);            \
    } while (0)

/* runtime gate: g_level is a variable */
#define LOG_RT(fmt, ...)                                    \
    do {                                                    \
        if (L_DEBUG <= g_level)                             \
            log_at(L_DEBUG, fmt, ##__VA_ARGS__);            \
    } while (0)

int main(void)
{
    LOG_DBG("dbg %d\n", 42);
    LOG_RT("rt %d\n", 7);
    return 0;
}
