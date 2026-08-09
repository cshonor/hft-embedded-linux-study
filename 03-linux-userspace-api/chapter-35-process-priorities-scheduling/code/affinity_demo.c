/* Print and optionally pin to CPU 0.
 * cc -Wall -Wextra -D_GNU_SOURCE -o affinity_demo affinity_demo.c
 * ./affinity_demo
 * ./affinity_demo pin
 */
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void show_affinity(void)
{
    cpu_set_t set;
    int i, n;

    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(set), &set) == -1) {
        perror("sched_getaffinity");
        return;
    }
    printf("pid=%ld CPUs:", (long)getpid());
    n = (int)sysconf(_SC_NPROCESSORS_ONLN);
    for (i = 0; i < n; i++)
        if (CPU_ISSET(i, &set))
            printf(" %d", i);
    printf("\n");
}

int main(int argc, char *argv[])
{
    show_affinity();

    if (argc > 1 && strcmp(argv[1], "pin") == 0) {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(0, &set);
        if (sched_setaffinity(0, sizeof(set), &set) == -1) {
            perror("sched_setaffinity");
            return 1;
        }
        printf("pinned to CPU 0\n");
        show_affinity();
    }
    return 0;
}
