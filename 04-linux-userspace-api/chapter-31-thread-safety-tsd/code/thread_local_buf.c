/* Static TLS with __thread — preferred for known per-thread buffers.
 * cc -Wall -Wextra -pthread -o thread_local_buf thread_local_buf.c
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static __thread char tls_buf[64];

static void *worker(void *arg)
{
    int id = *(int *)arg;
    snprintf(tls_buf, sizeof(tls_buf), "hello from thread %d", id);
    usleep(20000);
    printf("%s (tls_buf addr=%p)\n", tls_buf, (void *)tls_buf);
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

    snprintf(tls_buf, sizeof(tls_buf), "main");
    printf("main: %s (addr=%p)\n", tls_buf, (void *)tls_buf);
    return 0;
}
