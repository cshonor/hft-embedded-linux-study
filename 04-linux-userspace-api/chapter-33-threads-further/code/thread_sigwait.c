/* Block signals in all threads; one thread sigwait()s (TLPI-recommended).
 * cc -Wall -Wextra -pthread -o thread_sigwait thread_sigwait.c
 * ./thread_sigwait   then: kill -USR1 <pid> / kill -TERM <pid>
 */
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile int running = 1;

static void *sig_thread(void *arg)
{
    sigset_t *set = arg;
    int sig;

    while (running) {
        if (sigwait(set, &sig) != 0) {
            perror("sigwait");
            break;
        }
        printf("sigwait got %d\n", sig);
        if (sig == SIGTERM) {
            running = 0;
            break;
        }
    }
    return NULL;
}

static void *worker(void *arg)
{
    (void)arg;
    while (running)
        sleep(1);
    return NULL;
}

int main(void)
{
    sigset_t set;
    pthread_t ts, tw;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGTERM);

    /* Block before creating any other threads — they inherit the mask */
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("pthread_sigmask");
        return 1;
    }

    printf("pid=%ld  kill -USR1 / -TERM this process\n", (long)getpid());
    pthread_create(&ts, NULL, sig_thread, &set);
    pthread_create(&tw, NULL, worker, NULL);

    pthread_join(ts, NULL);
    pthread_join(tw, NULL);
    return 0;
}
