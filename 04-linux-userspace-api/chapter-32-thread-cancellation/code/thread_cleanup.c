/* cleanup handlers run on cancel / pthread_exit, not on plain return.
 * cc -Wall -Wextra -pthread -o thread_cleanup thread_cleanup.c
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void free_buf(void *arg)
{
    printf("cleanup: free %p\n", arg);
    free(arg);
}

static void *worker(void *arg)
{
    char *buf;
    (void)arg;

    buf = malloc(64);
    if (buf == NULL)
        return NULL;
    strcpy(buf, "owned-by-thread");

    pthread_cleanup_push(free_buf, buf);
    for (;;)
        pause(); /* cancel here → cleanup runs */
    /* not reached */
    pthread_cleanup_pop(1);
    return NULL;
}

int main(void)
{
    pthread_t t;
    void *res;

    pthread_create(&t, NULL, worker, NULL);
    sleep(1);
    pthread_cancel(t);
    pthread_join(t, &res);
    printf("join: %s\n", res == PTHREAD_CANCELED ? "PTHREAD_CANCELED" : "?");
    return 0;
}
