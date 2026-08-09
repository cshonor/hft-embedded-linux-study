/* Bounded buffer: mutex + condition variables (while-predicate).
 * cc -Wall -Wextra -pthread -o prod_condvar prod_condvar.c && ./prod_condvar
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CAP 4
#define ITEMS 20

static int buf[CAP];
static int count;
static int head, tail;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

static void *producer(void *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < ITEMS; i++) {
        pthread_mutex_lock(&mtx);
        while (count == CAP)
            pthread_cond_wait(&not_full, &mtx);
        buf[tail] = i;
        tail = (tail + 1) % CAP;
        count++;
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

static void *consumer(void *arg)
{
    int i, v;
    long sum = 0;
    (void)arg;
    for (i = 0; i < ITEMS; i++) {
        pthread_mutex_lock(&mtx);
        while (count == 0)
            pthread_cond_wait(&not_empty, &mtx);
        v = buf[head];
        head = (head + 1) % CAP;
        count--;
        sum += v;
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mtx);
    }
    printf("consumer sum=%ld (expect %d)\n", sum, (ITEMS - 1) * ITEMS / 2);
    return NULL;
}

int main(void)
{
    pthread_t tp, tc;
    int err;

    err = pthread_create(&tp, NULL, producer, NULL);
    if (err) { fprintf(stderr, "%s\n", strerror(err)); return 1; }
    err = pthread_create(&tc, NULL, consumer, NULL);
    if (err) { fprintf(stderr, "%s\n", strerror(err)); return 1; }

    pthread_join(tp, NULL);
    pthread_join(tc, NULL);
    return 0;
}
