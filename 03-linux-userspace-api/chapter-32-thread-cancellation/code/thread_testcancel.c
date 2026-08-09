/* Busy loop needs pthread_testcancel() or cancel never runs (deferred).
 * cc -Wall -Wextra -pthread -o thread_testcancel thread_testcancel.c
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *worker(void *arg)
{
    volatile unsigned long n = 0;
    (void)arg;
    for (;;) {
        n++;
        if ((n & 0xfffffUL) == 0)
            pthread_testcancel(); /* manual cancellation point */
    }
    return NULL;
}

int main(void)
{
    pthread_t t;
    void *res;

    pthread_create(&t, NULL, worker, NULL);
    usleep(200000);
    pthread_cancel(t);
    pthread_join(t, &res);
    printf("%s\n", res == PTHREAD_CANCELED ? "canceled via testcancel" : "?");
    return 0;
}
