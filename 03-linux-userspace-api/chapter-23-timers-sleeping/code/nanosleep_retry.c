/* nanosleep with EINTR retry (does not use SIGALRM).
 * cc -Wall -Wextra -o nanosleep_retry nanosleep_retry.c && ./nanosleep_retry
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t interrupts;

static void on_int(int sig)
{
    (void)sig;
    interrupts++;
}

static int sleep_full(const struct timespec *total)
{
    struct timespec req = *total;

    while (nanosleep(&req, &req) == -1) {
        if (errno != EINTR)
            return -1;
        /* req already holds remaining time */
    }
    return 0;
}

int main(void)
{
    struct sigaction sa;
    struct timespec two_sec = { .tv_sec = 2, .tv_nsec = 0 };

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_int;
    sigemptyset(&sa.sa_mask);
    /* no SA_RESTART — we want nanosleep to surface EINTR for the demo */
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    printf("sleeping 2s (Ctrl+C may interrupt; we retry)...\n");
    if (sleep_full(&two_sec) == -1) {
        perror("nanosleep");
        return EXIT_FAILURE;
    }
    printf("done (EINTR count seen by handler: %d)\n", (int)interrupts);
    return 0;
}
