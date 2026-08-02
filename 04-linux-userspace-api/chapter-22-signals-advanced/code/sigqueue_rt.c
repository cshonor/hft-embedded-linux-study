/* Queue a realtime signal to self with an int payload.
 * cc -Wall -Wextra -o sigqueue_rt sigqueue_rt.c && ./sigqueue_rt
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t got;
static volatile sig_atomic_t payload;

static void on_rt(int sig, siginfo_t *info, void *ucontext)
{
    (void)sig;
    (void)ucontext;
    if (info != NULL)
        payload = (sig_atomic_t)info->si_value.sival_int;
    got = 1;
}

int main(void)
{
    struct sigaction sa;
    union sigval sv;
    int rtsig = SIGRTMIN + 1;
    sigset_t block, wait_mask, prev;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = on_rt;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(rtsig, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    sigemptyset(&block);
    sigaddset(&block, rtsig);
    if (sigprocmask(SIG_BLOCK, &block, &prev) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    sv.sival_int = 42;
    if (sigqueue(getpid(), rtsig, sv) == -1) {
        perror("sigqueue");
        return EXIT_FAILURE;
    }
    printf("queued RT signal %d with sival_int=42\n", rtsig);

    wait_mask = prev;
    sigdelset(&wait_mask, rtsig);
    while (!got)
        sigsuspend(&wait_mask);

    printf("handler saw payload=%d\n", (int)payload);
    sigprocmask(SIG_SETMASK, &prev, NULL);
    return 0;
}
