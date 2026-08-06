/*
 * 伪共享演示：两线程各改一个 counter。
 * shared：两 counter 同缓存行 → 大量 cache line bounce。
 * padded：用 64B 对齐隔离 → 通常更快。
 *
 * 运行：make && ./main
 * 多次运行取趋势；CPU/核心数不同结果会有波动。
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CACHE_LINE 64
#define ITERS 50000000

typedef struct {
    volatile int64_t c1;
    volatile int64_t c2;
} shared_t;

typedef struct {
    volatile int64_t c1;
    char pad[CACHE_LINE - sizeof(int64_t)];
    volatile int64_t c2;
} padded_t;

static shared_t g_shared;
static padded_t g_padded;

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void *inc_c1(void *arg)
{
    (void)arg;
    for (int i = 0; i < ITERS; i++)
        g_shared.c1++;
    return NULL;
}

static void *inc_c2(void *arg)
{
    (void)arg;
    for (int i = 0; i < ITERS; i++)
        g_shared.c2++;
    return NULL;
}

static void *inc_p1(void *arg)
{
    (void)arg;
    for (int i = 0; i < ITERS; i++)
        g_padded.c1++;
    return NULL;
}

static void *inc_p2(void *arg)
{
    (void)arg;
    for (int i = 0; i < ITERS; i++)
        g_padded.c2++;
    return NULL;
}

static double bench(void *(*f1)(void *), void *(*f2)(void *))
{
    pthread_t t1, t2;
    double t0 = now_sec();
    if (pthread_create(&t1, NULL, f1, NULL) != 0 ||
        pthread_create(&t2, NULL, f2, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return now_sec() - t0;
}

int main(void)
{
    double t_shared = bench(inc_c1, inc_c2);
    double t_padded = bench(inc_p1, inc_p2);

    printf("false sharing demo (%d iters/thread)\n", ITERS);
    printf("  shared (same cache line): %.2f s\n", t_shared);
    printf("  padded (64B apart):       %.2f s\n", t_padded);
    if (t_padded > 0.0)
        printf("  speedup (shared/padded):  %.2fx\n", t_shared / t_padded);
    return 0;
}
