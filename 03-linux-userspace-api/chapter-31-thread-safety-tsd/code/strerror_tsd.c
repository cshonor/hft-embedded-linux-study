/* Thread-safe message buffer via pthread_key (strerror-style pattern).
 * cc -Wall -Wextra -pthread -o strerror_tsd strerror_tsd.c && ./strerror_tsd
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_key_t key;

static void buf_destructor(void *p)
{
    free(p);
}

static void make_key(void)
{
    pthread_key_create(&key, buf_destructor);
}

/* Like a tiny strerror: returns pointer valid in *this* thread only */
static char *thread_msg(int code)
{
    char *buf;

    pthread_once(&once, make_key);
    buf = pthread_getspecific(key);
    if (buf == NULL) {
        buf = malloc(64);
        if (buf == NULL)
            return NULL;
        pthread_setspecific(key, buf);
    }
    snprintf(buf, 64, "thread-local msg for code=%d", code);
    return buf;
}

static void *worker(void *arg)
{
    int id = *(int *)arg;
    char *a = thread_msg(id);
    char *b = thread_msg(id + 100);
    /* a and b are the same buffer (overwrite) — still private per thread */
    printf("worker %d: %s\n", id, b);
    usleep(50000);
    printf("worker %d: still %s\n", id, thread_msg(id));
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;
    int a = 1, b = 2;

    pthread_create(&t1, NULL, worker, &a);
    pthread_create(&t2, NULL, worker, &b);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
