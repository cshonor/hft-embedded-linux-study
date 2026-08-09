/* Basic pthread_create + pthread_join.
 * cc -Wall -Wextra -pthread -o simple_thread simple_thread.c && ./simple_thread
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *worker(void *arg)
{
    const char *name = arg;
    printf("worker: hello from %s (self ok)\n", name);
    return NULL;
}

int main(void)
{
    pthread_t t;
    int err;

    err = pthread_create(&t, NULL, worker, "tid-A");
    if (err != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_join(t, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(err));
        return EXIT_FAILURE;
    }
    printf("main: joined\n");
    return 0;
}
