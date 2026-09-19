/* T7b: does -std= change what printf archetype accepts? (ties back to 6.1) */
#include <stddef.h>

void f(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void f(const char *fmt, ...) { (void)fmt; }

int main(void)
{
    f("%m\n");            /* glibc %m : GNU extension        */
    f("%'d\n", 1);        /* thousands grouping : GNU        */
    f("%lld\n", 1LL);     /* C99 long long                   */
    f("%zu\n", (size_t)1);/* C99 size_t                      */
    return 0;
}
