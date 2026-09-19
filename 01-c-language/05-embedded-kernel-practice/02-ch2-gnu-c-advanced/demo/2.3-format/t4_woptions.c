/* T4: the -Wformat family */
#include <stdio.h>

void logf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void logf(const char *fmt, ...) { (void)fmt; }

int main(void)
{
    const char *s = "%d\n";
    unsigned u = 1;
    int n = 0;
    char small[8];

    logf(s, 1);                    /* A: -Wformat-nonliteral            */
    printf("%d\n");                /* B: too few arguments              */
    printf("");                    /* C: -Wformat-zero-length           */
    printf("%d\n", u);             /* D: -Wformat-signedness            */
    printf("%n\n", &n);            /* E: %n -- security sensitive       */
    snprintf(small, sizeof small, "%s\n", "a very long string here"); /* F: truncation */
    sprintf(small, "%s\n", "a very long string here");               /* G: overflow   */
    return 0;
}
