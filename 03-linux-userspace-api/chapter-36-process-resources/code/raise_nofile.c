/* Raise RLIMIT_NOFILE soft limit up to current hard (service-style).
 * cc -Wall -Wextra -o raise_nofile raise_nofile.c && ./raise_nofile
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

int main(void)
{
    struct rlimit r;

    if (getrlimit(RLIMIT_NOFILE, &r) == -1) {
        perror("getrlimit");
        return 1;
    }
    printf("before: soft=%llu hard=%llu\n",
           (unsigned long long)r.rlim_cur,
           (unsigned long long)r.rlim_max);

    if (r.rlim_cur < r.rlim_max) {
        r.rlim_cur = r.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &r) == -1) {
            fprintf(stderr, "setrlimit: %s\n", strerror(errno));
            return 1;
        }
    }

    if (getrlimit(RLIMIT_NOFILE, &r) == -1) {
        perror("getrlimit");
        return 1;
    }
    printf("after:  soft=%llu hard=%llu\n",
           (unsigned long long)r.rlim_cur,
           (unsigned long long)r.rlim_max);

    /* Non-privileged: raising hard should fail */
    r.rlim_max++;
    r.rlim_cur = r.rlim_max;
    if (setrlimit(RLIMIT_NOFILE, &r) == -1)
        printf("raise hard: %s (expected for non-root)\n", strerror(errno));
    else
        printf("raise hard succeeded (privileged)\n");

    return 0;
}
