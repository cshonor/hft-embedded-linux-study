/*
 * Hennessy Ch2/Ch5 · 伪共享 (False Sharing) — cache-line padding 对比
 *
 * 对照笔记:
 *   03/chapter-02/notes/section-2.3-缓存性能十项高级优化.md
 *   03/chapter-05/notes/section-5.3-性能分析与伪共享.md
 *   02/chapter-06/notes/section-6.4.2-直接映射.md (cache line 概念)
 *
 * 编译:
 *   gcc -Wall -Wextra -std=c11 -O2 -o ch02_false ch02-false-sharing.c -lpthread
 * 运行:
 *   ./ch02_false
 *
 * HFT 关联: 多核计数器/统计字段必须 cache-line 隔离
 *           perf c2c 可检测伪共享热点
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>

#define NTHREADS 4
#define NLOOP    100000000  /* 1 亿次 — 放大伪共享效应 */

static long page_size = 0;
static int  cache_line = 64;  /* 绝大多数 x86/ARM = 64B */

/* ---------- 版本 1: 伪共享 — 两个计数器紧邻 ---------- */
typedef struct {
    _Atomic uint64_t c1;
    _Atomic uint64_t c2;
} CountersBad;  /* sizeof = 16B, 两个计数器在同一 cache line */

static CountersBad bad;

static void *worker_bad1(void *arg)
{
    (void)arg;
    for (int i = 0; i < NLOOP; i++)
        atomic_fetch_add(&bad.c1, 1);
    return NULL;
}

static void *worker_bad2(void *arg)
{
    (void)arg;
    for (int i = 0; i < NLOOP; i++)
        atomic_fetch_add(&bad.c2, 1);
    return NULL;
}

/* ---------- 版本 2: 消除伪共享 — cache-line padding ---------- */
typedef struct {
    _Atomic uint64_t c1;
    char pad1[64 - sizeof(uint64_t)];   /* 填充到 64B */
    _Atomic uint64_t c2;
    char pad2[64 - sizeof(uint64_t)];   /* c2 独占一个 cache line */
} CountersGood;  /* sizeof = 128B, 各独占 cache line */

static CountersGood good;

static void *worker_good1(void *arg)
{
    (void)arg;
    for (int i = 0; i < NLOOP; i++)
        atomic_fetch_add(&good.c1, 1);
    return NULL;
}

static void *worker_good2(void *arg)
{
    (void)arg;
    for (int i = 0; i < NLOOP; i++)
        atomic_fetch_add(&good.c2, 1);
    return NULL;
}

/* ---------- 版本 3: 每线程私有计数器, 最后合并 ---------- */
typedef struct {
    _Atomic uint64_t value;
    char pad[64 - sizeof(uint64_t)];
} AlignedCounter;

static AlignedCounter per_thread[NTHREADS]
    __attribute__((aligned(64)));

static void *worker_perthread(void *arg)
{
    int tid = *(int *)arg;
    for (int i = 0; i < NLOOP; i++)
        atomic_fetch_add(&per_thread[tid].value, 1);
    return NULL;
}

/* ---------- 计时 ---------- */
static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void run2(void *(*fn1)(void *), void *(*fn2)(void *),
                 const char *label)
{
    pthread_t t1, t2;
    double t0 = now_sec();
    pthread_create(&t1, NULL, fn1, NULL);
    pthread_create(&t2, NULL, fn2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    double t1_elapsed = now_sec() - t0;
    printf("  %-40s  %8.3f ms\n", label, t1_elapsed * 1000.0);
}

int main(void)
{
    page_size = sysconf(_SC_PAGESIZE);
    printf("=== Hennessy Ch2/5 · 伪共享对比 (cache_line=%dB) ===\n\n", cache_line);

    printf("--- 2 线程, 各写不同计数器, %d M 次 ---\n\n", NLOOP / 1000000);

    printf("sizeof(CountersBad)  = %zu  (c1,c2 同 cache line)\n", sizeof(CountersBad));
    printf("sizeof(CountersGood) = %zu  (c1,c2 各独占 cache line)\n\n", sizeof(CountersGood));

    /* 伪共享 */
    run2(worker_bad1, worker_bad2, "伪共享 (c1,c2 同 line)");

    /* 消除伪共享 */
    run2(worker_good1, worker_good2, "cache-line padding (各独占)");

    /* 每线程私有 */
    printf("\n--- %d 线程私有计数器, 最后合并 ---\n\n", NTHREADS);
    {
        pthread_t tids[NTHREADS];
        int ids[NTHREADS];
        double t0 = now_sec();
        for (int i = 0; i < NTHREADS; i++) {
            ids[i] = i;
            pthread_create(&tids[i], NULL, worker_perthread, &ids[i]);
        }
        for (int i = 0; i < NTHREADS; i++)
            pthread_join(tids[i], NULL);

        uint64_t total = 0;
        for (int i = 0; i < NTHREADS; i++)
            total += atomic_load(&per_thread[i].value);

        double elapsed = now_sec() - t0;
        printf("  %-40s  %8.3f ms  total=%lu\n",
               "per-thread 私有计数器", elapsed * 1000.0,
               (unsigned long)total);
    }

    printf("\n机制解释:\n");
    printf("  伪共享: 核A写 c1 → c1 所在 cache line 在核B 失效\n");
    printf("          核B写 c2 → c2 所在 cache line 在核A 失效\n");
    printf("          → 乒乓 (ping-pong), 总线一致性流量暴涨\n");
    printf("\n  对策:\n");
    printf("    1. alignas(64) / __attribute__((aligned(64))) 填充\n");
    printf("    2. per-thread 私有计数器, 周期性合并\n");
    printf("    3. perf c2c 检测伪共享热点\n");
    printf("\n  HFT: 订单簿统计、风控计数器必须避免伪共享\n");

    return 0;
}
