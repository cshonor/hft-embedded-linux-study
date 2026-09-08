/* T8: the kernel no_printk trick -- keep the check, drop the code */
#include <stdio.h>

int real_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int real_log(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

/* kernel no_printk look-alike: format is still checked, code vanishes */
#define no_log(fmt, ...) ({ if (0) real_log(fmt, ##__VA_ARGS__); 0; })

/* dead macro: neither code nor check */
#define dead_log(fmt, ...) do { } while (0)

/* try to put the attribute inside a macro */
#define attr_log(fmt, ...) __attribute__((format(printf, 1, 2))) real_log(fmt, ##__VA_ARGS__)

int main(void)
{
    no_log("%d\n", "x");     /* A: still checked?  */
    dead_log("%d\n", "y");   /* B: dropped entirely */
    /* attr_log("%d\n", "z");  expands to a syntax error -- see notes */
    return 0;
}
