/* Block SIGINT, show pending, then unblock (delivery / default terminate).
 * cc -Wall -Wextra -o block_pending block_pending.c && ./block_pending
 *
 * While "blocked", press Ctrl+C once or twice; standard signals do not queue.
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void on_int(int sig)
{
    const char msg[] = "\n[handler] SIGINT delivered\n";
    (void)sig;
    /* write is async-signal-safe */
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main(void)
{
    sigset_t block, old, pending;
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_int;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    sigemptyset(&block);
    sigaddset(&block, SIGINT);
    if (sigprocmask(SIG_BLOCK, &block, &old) == -1) {
        perror("sigprocmask BLOCK");
        return EXIT_FAILURE;
    }

    printf("pid=%ld  SIGINT blocked for 8s — try Ctrl+C\n", (long)getpid());
    sleep(8);

    if (sigpending(&pending) == -1) {
        perror("sigpending");
        return EXIT_FAILURE;
    }
    printf("SIGINT pending? %s\n",
           sigismember(&pending, SIGINT) ? "YES" : "no");

    printf("unblocking SIGINT (pending one will deliver now)...\n");
    if (sigprocmask(SIG_SETMASK, &old, NULL) == -1) {
        perror("sigprocmask SETMASK");
        return EXIT_FAILURE;
    }

    /* If user never pressed Ctrl+C, just exit cleanly */
    sleep(1);
    printf("done\n");
    return 0;
}
