/* Reap all exited children in SIGCHLD handler via waitpid(WNOHANG) loop.
 * cc -Wall -Wextra -o sigchld_reap sigchld_reap.c && ./sigchld_reap
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t kids_left = 0;

static void on_chld(int sig)
{
    int status;
    pid_t pid;

    (void)sig;
    /* Loop: standard SIGCHLD does not queue — one delivery may cover many */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (kids_left > 0)
            kids_left--;
    }
}

int main(void)
{
    struct sigaction sa;
    int i;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_chld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    for (i = 0; i < 3; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return EXIT_FAILURE;
        }
        if (pid == 0) {
            _exit(10 + i);
        }
        kids_left++;
    }

    printf("spawned 3 children; waiting for reaps...\n");
    while (kids_left > 0)
        pause();

    printf("all children reaped (no zombies)\n");
    return 0;
}
