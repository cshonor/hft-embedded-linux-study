/* T1: format attribute basics -- index semantics, too few / too many args */
#include <stdio.h>
#include <stdarg.h>

void log_ok(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_ok(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
}

/* no attribute at all -> should stay silent */
void log_bad(const char *fmt, ...);
void log_bad(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
}

/* fixed prefix: level + tag before fmt */
void log_lvl(int level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void log_lvl(int level, const char *fmt, ...)
{
    (void)level;
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
}

int main(void)
{
    log_ok("%d %s\n", "hello", 42);   /* A: two type mismatches */
    log_bad("%d %s\n", "hello", 42);  /* B: no attribute -> silence */
    log_ok("%d\n");                   /* C: too few args */
    log_ok("%d %d\n", 1);             /* D: too few args */
    log_ok("%d\n", 1, 2);             /* E: too many args */
    log_ok("%Q\n", 1);                /* F: invalid conversion */
    log_lvl(3, "%s\n", 42);           /* G: mismatch, index 2/3 */
    log_lvl(3, "%d\n", "x");          /* H: reverse mismatch */
    return 0;
}
