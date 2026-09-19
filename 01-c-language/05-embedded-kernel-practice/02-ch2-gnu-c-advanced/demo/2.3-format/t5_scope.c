/* T5: where must the attribute live to be honoured? */
#include <stdio.h>

/* A: attribute on a forward declaration, definition has none */
void a_decl(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void a_decl(const char *fmt, ...) { (void)fmt; }

/* B: no forward declaration, attribute sits on the definition (before use) */
void b_def(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void b_def(const char *fmt, ...) { (void)fmt; }

/* C: attribute on first declaration only, second declaration drops it */
void c_two(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void c_two(const char *fmt, ...);
void c_two(const char *fmt, ...) { (void)fmt; }

/* D: attribute on the definition only, no separate declaration */
void d_defonly(const char *fmt, ...) __attribute__((format(printf, 1, 2))) { (void)fmt; }

/* E: reached through a macro wrapper */
#define LOG(fmt, ...) a_decl(fmt, ##__VA_ARGS__)

/* F: through a function pointer carrying the attribute */
void (*fp)(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

int main(void)
{
    a_decl("%d\n", "x");   /* A */
    b_def("%d\n", "x");    /* B */
    c_two("%d\n", "x");    /* C */
    d_defonly("%d\n", "x");/* D */
    LOG("%d\n", "y");      /* E */
    fp = a_decl;
    fp("%d\n", "z");       /* F */
    return 0;
}
