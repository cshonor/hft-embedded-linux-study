/* Print selected resource soft/hard limits.
 * cc -Wall -Wextra -o print_rlimit print_rlimit.c && ./print_rlimit
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

static void show(const char *name, int resource)
{
    struct rlimit r;

    if (getrlimit(resource, &r) == -1) {
        perror(name);
        return;
    }
    printf("%-16s soft=", name);
    if (r.rlim_cur == RLIM_INFINITY)
        printf("inf");
    else
        printf("%llu", (unsigned long long)r.rlim_cur);
    printf("  hard=");
    if (r.rlim_max == RLIM_INFINITY)
        printf("inf\n");
    else
        printf("%llu\n", (unsigned long long)r.rlim_max);
}

int main(void)
{
    show("NOFILE", RLIMIT_NOFILE);
    show("STACK", RLIMIT_STACK);
    show("AS", RLIMIT_AS);
    show("CORE", RLIMIT_CORE);
    show("NPROC", RLIMIT_NPROC);
    show("CPU", RLIMIT_CPU);
    show("FSIZE", RLIMIT_FSIZE);
    return 0;
}
