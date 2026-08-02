/* getpriority / setpriority for current process.
 * cc -Wall -Wextra -o t_nice t_nice.c && ./t_nice
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

int main(void)
{
    int nice_val;

    errno = 0;
    nice_val = getpriority(PRIO_PROCESS, 0);
    if (nice_val == -1 && errno != 0) {
        perror("getpriority");
        return 1;
    }
    printf("pid=%ld nice=%d\n", (long)getpid(), nice_val);

    /* Lower priority (raise nice) — usually allowed for unprivileged */
    if (setpriority(PRIO_PROCESS, 0, nice_val + 1) == -1) {
        perror("setpriority(+1)");
    } else {
        errno = 0;
        nice_val = getpriority(PRIO_PROCESS, 0);
        printf("after setpriority(+1): nice=%d\n", nice_val);
    }

    /* Try raise priority (lower nice) — often EPERM for normal user */
    if (setpriority(PRIO_PROCESS, 0, -1) == -1)
        printf("setpriority(-1): %s (expected for non-root)\n",
               strerror(errno));
    else
        printf("setpriority(-1) succeeded (privileged)\n");

    return 0;
}
