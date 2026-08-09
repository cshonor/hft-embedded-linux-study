#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void *worker(void *arg)
{
    (void)arg;
    uint64_t t0 = now_ns();
    /* stub “sensor” work */
    for (volatile int i = 0; i < 100000; i++) {
    }
    uint64_t dt = now_ns() - t0;
    printf("worker latency stub: %llu ns\n", (unsigned long long)dt);
    return NULL;
}

int main(void)
{
    pthread_t th;
    pthread_create(&th, NULL, worker, NULL);
    pthread_join(th, NULL);
    return 0;
}
