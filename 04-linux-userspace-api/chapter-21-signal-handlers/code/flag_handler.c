/* Canonical pattern: handler only sets volatile sig_atomic_t.
 * Also blocks SIGTERM while handling SIGINT (sa_mask demo).
 *
 * cc -Wall -Wextra -o flag_handler flag_handler.c && ./flag_handler
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t got_sigint = 0;

static void on_int(int sig)
{
    (void)sig;
    got_sigint = 1;
}

int main(void)
{
    struct sigaction sa;
    int count = 0;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_int;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM); /* extra block only during handler */
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    printf("pid=%ld  press Ctrl+C (exits after 3 deliveries)\n", (long)getpid());
    while (count < 3) {
        if (got_sigint) {
            got_sigint = 0;
            count++;
            printf("main: saw SIGINT #%d\n", count);
        }
        pause(); /* wake on signal; ok here for demo */
    }
    return 0;
}
