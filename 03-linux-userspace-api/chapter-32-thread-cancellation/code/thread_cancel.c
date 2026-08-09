/* Basic pthread_cancel; join sees PTHREAD_CANCELED.
 * cc -Wall -Wextra -pthread -o thread_cancel thread_cancel.c
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *worker(void *arg)
{
    (void)arg;
    for (;;) {
        pause(); /* cancellation point */
    }
    return NULL;
}

int main(void)
{
    pthread_t t;
    void *res;
    int err;

    err = pthread_create(&t, NULL, worker, NULL);
    if (err) {
        fprintf(stderr, "create: %s\n", strerror(err));
        return 1;
    }

    sleep(1);
    err = pthread_cancel(t);
    if (err) {
        fprintf(stderr, "cancel: %s\n", strerror(err));
        return 1;
    }

    err = pthread_join(t, &res);
    if (err) {
        fprintf(stderr, "join: %s\n", strerror(err));
        return 1;
    }

    if (res == PTHREAD_CANCELED)
        printf("thread was canceled\n");
    else
        printf("unexpected retval\n");
    return 0;
}
