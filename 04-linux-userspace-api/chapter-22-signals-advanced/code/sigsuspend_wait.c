/* Safe wait: block SIGINT, then sigsuspend with mask that unblocks it.
 * cc -Wall -Wextra -o sigsuspend_wait sigsuspend_wait.c && ./sigsuspend_wait
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t got = 0;

static void on_int(int sig)
{
    (void)sig;
    got = 1;
}

int main(void)
{
    struct sigaction sa;
    sigset_t block, prev, wait_mask;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_int;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    sigemptyset(&block);
    sigaddset(&block, SIGINT);
    if (sigprocmask(SIG_BLOCK, &block, &prev) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    wait_mask = prev;
    sigdelset(&wait_mask, SIGINT);

    printf("pid=%ld  waiting with sigsuspend — press Ctrl+C\n", (long)getpid());
    while (!got)
        sigsuspend(&wait_mask); /* returns -1/EINTR after handler */

    sigprocmask(SIG_SETMASK, &prev, NULL);
    printf("got SIGINT safely\n");
    return 0;
}
