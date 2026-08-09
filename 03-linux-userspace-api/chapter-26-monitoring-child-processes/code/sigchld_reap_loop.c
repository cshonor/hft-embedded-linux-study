/* SIGCHLD handler must loop waitpid(WNOHANG) — signal does not queue.
 * cc -Wall -Wextra -o sigchld_reap_loop sigchld_reap_loop.c && ./sigchld_reap_loop
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t living = 0;

static void on_chld(int sig)
{
    int st;
    pid_t pid;

    (void)sig;
    while ((pid = waitpid(-1, &st, WNOHANG)) > 0) {
        if (living > 0)
            living--;
        /* keep handler tiny; avoid printf here in production */
    }
}

int main(void)
{
    struct sigaction sa;
    int i;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_chld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    for (i = 0; i < 5; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return EXIT_FAILURE;
        }
        if (pid == 0)
            _exit(i);
        living++;
    }

    printf("spawned 5 children; waiting for SIGCHLD reaps...\n");
    while (living > 0)
        pause();

    printf("all reaped (living=%d)\n", (int)living);
    return 0;
}
