/* Unsynchronized increments — data race (undefined; often loses counts).
 * Fix belongs in Ch30 (mutex). Compile without -O2 to make races likelier.
 *
 * cc -Wall -Wextra -pthread -o thread_race thread_race.c && ./thread_race
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 100000

static long counter;

static void *add(void *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < N; i++)
        counter++; /* NOT atomic / not locked */
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
    return 0;
}
