/* T3: archetype dialects -- printf vs gnu_printf vs ms_printf vs syslog */
#include <stdio.h>

void f_std(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void f_gnu(const char *fmt, ...) __attribute__((format(gnu_printf, 1, 2)));
void f_ms (const char *fmt, ...) __attribute__((format(ms_printf, 1, 2)));
void f_sys(const char *fmt, ...) __attribute__((format(syslog, 1, 2)));
void f_sft(char *buf, size_t n, const char *fmt, ...) __attribute__((format(strftime, 3, 0)));

int main(void)
{
    char *p = 0;
    char buf[64];

    f_std("%m\n");                    /* A: glibc %m under plain printf  */
    f_gnu("%m\n");                    /* B: glibc %m under gnu_printf    */
    f_sys("%m\n");                    /* C: %m is *standard* for syslog  */

    f_std("%ms\n", &p);               /* D: glibc 'm' allocation flag    */
    f_gnu("%ms\n", &p);               /* E: ditto under gnu_printf       */

    f_std("%'d\n", 1234567);          /* F: thousands grouping flag      */
    f_gnu("%'d\n", 1234567);          /* G: ditto                        */

    f_ms("%I64d\n", (long long)1);    /* H: MS 64-bit length modifier    */
    f_std("%I64d\n", (long long)1);   /* I: same under plain printf      */

    f_sft(buf, sizeof buf, "%Q\n");   /* J: strftime rejects unknown     */
    f_sft(buf, sizeof buf, "%Y\n");   /* K: strftime accepts %Y          */
    return 0;
}
