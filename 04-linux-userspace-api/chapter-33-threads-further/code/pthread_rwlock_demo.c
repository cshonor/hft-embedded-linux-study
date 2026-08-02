/* Simple rwlock: concurrent readers, exclusive writer.
 * cc -Wall -Wextra -pthread -o pthread_rwlock_demo pthread_rwlock_demo.c
 */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;
static int shared;

static void *reader(void *arg)
{
    int id = *(int *)arg;
    int i, v;

    for (i = 0; i < 3; i++) {
        pthread_rwlock_rdlock(&rw);
        v = shared;
        printf("reader %d saw %d\n", id, v);
        usleep(30000);
        pthread_rwlock_unlock(&rw);
        usleep(10000);
    }
    return NULL;
}

static void *writer(void *arg)
{
    int i;
    (void)arg;
    for (i = 1; i <= 3; i++) {
        pthread_rwlock_wrlock(&rw);
        shared = i * 10;
        printf("writer set %d\n", shared);
        usleep(20000);
        pthread_rwlock_unlock(&rw);
        usleep(40000);
    }
    return NULL;
}

int main(void)
{
    pthread_t r1, r2, w;
    int a = 1, b = 2;

    pthread_create(&r1, NULL, reader, &a);
    pthread_create(&r2, NULL, reader, &b);
    pthread_create(&w, NULL, writer, NULL);
    pthread_join(r1, NULL);
    pthread_join(r2, NULL);
    pthread_join(w, NULL);
    return 0;
}
