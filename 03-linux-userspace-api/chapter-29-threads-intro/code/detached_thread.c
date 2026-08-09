/* Detached thread: no join; resources reclaimed on exit.
 * cc -Wall -Wextra -pthread -o detached_thread detached_thread.c
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *worker(void *arg)
{
    (void)arg;
    printf("detached worker running\n");
    /* brief work */
    usleep(100000);
    printf("detached worker done\n");
    return NULL;
}

int main(void)
{
    pthread_t t;
    int err;

    err = pthread_create(&t, NULL, worker, NULL);
    if (err != 0) {
        fprintf(stderr, "create: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_detach(t);
    if (err != 0) {
        fprintf(stderr, "detach: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    /* Cannot join; give worker time to finish before process exits */
    usleep(300000);
    printf("main exiting (detached thread should have finished)\n");
    return 0;
}
