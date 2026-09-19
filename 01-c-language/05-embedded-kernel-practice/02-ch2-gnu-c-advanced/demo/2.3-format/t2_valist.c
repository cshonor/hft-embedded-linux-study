/* T2: first-to-check = 0 -> the va_list flavour */
#include <stdio.h>
#include <stdarg.h>

/* correct: this function consumes a va_list, so first-to-check is 0 */
void vlog(const char *fmt, va_list ap) __attribute__((format(printf, 1, 0)));
void vlog(const char *fmt, va_list ap) { vprintf(fmt, ap); }

/* wrong on purpose: declared as if it took '...' but really takes va_list */
void vlog_wrong(const char *fmt, va_list ap) __attribute__((format(printf, 1, 2)));
void vlog_wrong(const char *fmt, va_list ap) { vprintf(fmt, ap); }

/* '...' wrappers that forward into the two variants above */
void wrap(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void wrap(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); vlog(fmt, ap); va_end(ap);
}

void wrap_wrong(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void wrap_wrong(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); vlog_wrong(fmt, ap); va_end(ap);
}

int main(void)
{
    wrap("%d\n", "x");        /* A: mismatch -- warning expected */
    wrap_wrong("%d\n", "x");  /* B: mismatch + maybe extra va_list note */
    return 0;
}
