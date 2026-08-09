/* SA_SIGINFO: print sender pid/uid for SIGUSR1.
 * cc -Wall -Wextra -o siginfo_demo siginfo_demo.c && ./siginfo_demo
 * Other shell: kill -USR1 <pid>
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t done = 0;
static volatile sig_atomic_t sender_pid;
static volatile sig_atomic_t sender_uid;

static void on_usr1(int sig, siginfo_t *info, void *ucontext)
{
    (void)sig;
    (void)ucontext;
    if (info != NULL) {
        sender_pid = (sig_atomic_t)info->si_pid;
        sender_uid = (sig_atomic_t)info->si_uid;
    }
    done = 1;
}

int main(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = on_usr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    printf("pid=%ld  waiting for SIGUSR1 (kill -USR1 %ld)\n",
           (long)getpid(), (long)getpid());
    while (!done)
        pause();

    printf("SIGUSR1 from pid=%ld uid=%ld\n",
           (long)sender_pid, (long)sender_uid);
    return 0;
}
