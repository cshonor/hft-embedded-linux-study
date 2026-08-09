/* Mutex-protected counter (TLPI-style thread_incr_mutex).
 * cc -Wall -Wextra -pthread -o thread_incr_mutex thread_incr_mutex.c
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 100000

static long counter;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static void *add(void *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < N; i++) {
        pthread_mutex_lock(&mtx);
        counter++;
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;
    int err;

    err = pthread_create(&t1, NULL, add, NULL);
    if (err) { fprintf(stderr, "%s\n", strerror(err)); return 1; }
    err = pthread_create(&t2, NULL, add, NULL);
    if (err) { fprintf(stderr, "%s\n", strerror(err)); return 1; }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("counter=%ld  expected=%d\n", counter, 2 * N);
    return (counter == 2L * N) ? 0 : 1;
}
