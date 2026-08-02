/* Synchronous signal receive: block USR1/USR2, sigwaitinfo (no handler).
 * cc -Wall -Wextra -o sigwaitinfo_loop sigwaitinfo_loop.c && ./sigwaitinfo_loop
 * Other shell: kill -USR1 <pid> ; kill -USR2 <pid>
 * Exit: kill -TERM <pid> (also synced)
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    sigset_t set;
    siginfo_t info;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    printf("pid=%ld  sigwaitinfo loop (USR1/USR2/TERM)\n", (long)getpid());
    for (;;) {
        int sig = sigwaitinfo(&set, &info);
        if (sig == -1) {
            perror("sigwaitinfo");
            return EXIT_FAILURE;
        }
        /* Ordinary thread code — printf/malloc OK here */
        printf("got signal %d from pid=%ld\n", sig, (long)info.si_pid);
        if (sig == SIGTERM) {
            printf("TERM — exit\n");
            break;
        }
    }
    return 0;
}
