/* pthread_barrier: all threads rendezvous before continuing.
 * cc -Wall -Wextra -pthread -o pthread_barrier_demo pthread_barrier_demo.c
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define N 4

static pthread_barrier_t barrier;

static void *worker(void *arg)
{
    int id = *(int *)arg;
    int rc;

    printf("thread %d before barrier\n", id);
    usleep((useconds_t)(id + 1) * 50000);

    rc = pthread_barrier_wait(&barrier);
    if (rc == PTHREAD_BARRIER_SERIAL_THREAD)
        printf("thread %d is SERIAL (one of them)\n", id);
    else if (rc != 0)
        fprintf(stderr, "barrier: %s\n", strerror(rc));

    printf("thread %d after barrier\n", id);
    return NULL;
}

int main(void)
{
    pthread_t t[N];
    int ids[N];
    int i;

    pthread_barrier_init(&barrier, NULL, N);
    for (i = 0; i < N; i++) {
        ids[i] = i;
        pthread_create(&t[i], NULL, worker, &ids[i]);
    }
    for (i = 0; i < N; i++)
        pthread_join(t[i], NULL);
    pthread_barrier_destroy(&barrier);
    return 0;
}
