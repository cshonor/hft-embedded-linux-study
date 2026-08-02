/* After fork: signal mask inherited; pending set cleared in child.
 * cc -Wall -Wextra -o fork_signal_state fork_signal_state.c && ./fork_signal_state
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void on_usr1(int sig)
{
    (void)sig;
    /* unused — we keep SIGUSR1 blocked so it stays pending in parent */
}

static void show_sets(const char *who)
{
    sigset_t mask, pend;

    if (sigprocmask(SIG_SETMASK, NULL, &mask) == -1 ||
        sigpending(&pend) == -1) {
        perror("sig*");
        _exit(1);
    }
    printf("%s: SIGUSR1 blocked=%s pending=%s\n",
           who,
           sigismember(&mask, SIGUSR1) ? "yes" : "no",
           sigismember(&pend, SIGUSR1) ? "yes" : "no");
}

int main(void)
{
    struct sigaction sa;
    sigset_t block;
    pid_t pid;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_usr1;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &block, NULL) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    /* Queue a pending SIGUSR1 in the parent */
    if (raise(SIGUSR1) == -1) {
        perror("raise");
        return EXIT_FAILURE;
    }

    show_sets("parent-before-fork");
    fflush(stdout);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        show_sets("child");
        /* Expect: blocked=yes, pending=no */
        _exit(0);
    }

    waitpid(pid, NULL, 0);
    show_sets("parent-after-fork");
    /* Parent still has pending until unblocked */
    return 0;
}
