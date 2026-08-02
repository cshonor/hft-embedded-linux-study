/* Without SA_RESTART, a signal during blocking read yields EINTR.
 * cc -Wall -Wextra -o eintr_read eintr_read.c && ./eintr_read
 * Press Ctrl+C while waiting on stdin.
 */
#include <errno.h>
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
    char buf[64];
    ssize_t n;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_int;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* deliberately NO SA_RESTART */

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    printf("blocking read(STDIN)... press Ctrl+C\n");
    n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n == -1) {
        if (errno == EINTR)
            printf("read interrupted: EINTR (got_flag=%d)\n", (int)got);
        else
            perror("read");
        return 0;
    }
    printf("read %zd bytes (no interrupt)\n", n);
    return 0;
}
