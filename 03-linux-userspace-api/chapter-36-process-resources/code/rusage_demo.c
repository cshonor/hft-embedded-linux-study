/* getrusage SELF vs CHILDREN (children only after wait).
 * cc -Wall -Wextra -o rusage_demo rusage_demo.c && ./rusage_demo
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

static void print_usage(const char *tag, const struct rusage *u)
{
    printf("%s: utime=%ld.%06ld  stime=%ld.%06ld  maxrss=%ld KB  "
           "nvcsw=%ld nivcsw=%ld\n",
           tag,
           (long)u->ru_utime.tv_sec, (long)u->ru_utime.tv_usec,
           (long)u->ru_stime.tv_sec, (long)u->ru_stime.tv_usec,
           u->ru_maxrss, u->ru_nvcsw, u->ru_nivcsw);
}

int main(void)
{
    struct rusage u;
    pid_t pid;
    volatile unsigned long i;

    /* burn a little user time */
    for (i = 0; i < 50000000UL; i++)
        ;

    if (getrusage(RUSAGE_SELF, &u) == -1) {
        perror("getrusage SELF");
        return 1;
    }
    print_usage("SELF", &u);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        volatile unsigned long j;
        for (j = 0; j < 30000000UL; j++)
            ;
        _exit(0);
    }

    /* Before wait: CHILDREN typically still zero / unchanged for that child */
    if (getrusage(RUSAGE_CHILDREN, &u) == 0)
        print_usage("CHILDREN-before-wait", &u);

    waitpid(pid, NULL, 0);

    if (getrusage(RUSAGE_CHILDREN, &u) == -1) {
        perror("getrusage CHILDREN");
        return 1;
    }
    print_usage("CHILDREN-after-wait", &u);
    return 0;
}
