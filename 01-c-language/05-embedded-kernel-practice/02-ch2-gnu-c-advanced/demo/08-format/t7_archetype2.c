/* T7: which archetypes does each compiler actually know? */
#include <stddef.h>

void a1(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void a2(const char *fmt, ...) __attribute__((format(gnu_printf, 1, 2)));
void a3(const char *fmt, ...) __attribute__((format(scanf, 1, 2)));
void a4(const char *fmt, ...) __attribute__((format(gnu_scanf, 1, 2)));
void a5(char *b, size_t n, const char *fmt, ...) __attribute__((format(strftime, 3, 0)));
void a6(const char *fmt, ...) __attribute__((format(strfmon, 1, 2)));
void a7(const char *fmt, ...) __attribute__((format(ms_printf, 1, 2)));
void a8(const char *fmt, ...) __attribute__((format(syslog, 1, 2)));

void a1(const char *fmt, ...) { (void)fmt; }
void a2(const char *fmt, ...) { (void)fmt; }
void a3(const char *fmt, ...) { (void)fmt; }
void a4(const char *fmt, ...) { (void)fmt; }
void a5(char *b, size_t n, const char *fmt, ...) { (void)b; (void)n; (void)fmt; }
void a6(const char *fmt, ...) { (void)fmt; }
void a7(const char *fmt, ...) { (void)fmt; }
void a8(const char *fmt, ...) { (void)fmt; }

int main(void) { return 0; }
