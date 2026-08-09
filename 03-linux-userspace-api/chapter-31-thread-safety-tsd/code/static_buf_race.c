/* Shared static buffer — classic thread-unsafe pattern (like old strtok).
 * cc -Wall -Wextra -pthread -o static_buf_race static_buf_race.c && ./static_buf_race
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char shared[64];

static char *fill(const char *s)
{
    /* NOT thread-safe */
    strncpy(shared, s, sizeof(shared) - 1);
    shared[sizeof(shared) - 1] = '\0';
    usleep(100000); /* widen race window */
    return shared;
}

static void *worker(void *arg)
{
    const char *name = arg;
    char *p = fill(name);
    printf("worker saw: %s (wanted %s)\n", p, name);
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, worker, "AAAAAAAA");
    pthread_create(&t2, NULL, worker, "BBBBBBBB");
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
