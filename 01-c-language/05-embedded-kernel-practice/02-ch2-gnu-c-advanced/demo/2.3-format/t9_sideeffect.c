/* T9: when the log is compiled out, is the argument still evaluated? */
#include <stdio.h>

int g_level = 0;
int counter = 0;

int side(void) { return ++counter; }

int real_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int real_log(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

/* kernel no_printk style: compile-time dead */
#define no_log(fmt, ...) ({ if (0) real_log(fmt, ##__VA_ARGS__); 0; })

/* runtime gate */
#define rt_log(fmt, ...) do { if (g_level) real_log(fmt, ##__VA_ARGS__); } while (0)

int main(void)
{
    no_log("%d\n", side());   /* A: arguments vanish?  */
    rt_log("%d\n", side());   /* B: evaluated at runtime */
    return 0;
}
