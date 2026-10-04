/* false_sharing_demo.c — 伪共享 vs 填充隔离 实测对比（ch13 §13.12 块11）
 *
 * 两个线程各写各的计数器，看似无竞争；
 * 但两个字段在同一 64B cache line 上 → MESI 让缓存行在两核间乒乓
 * → 每次写都变成跨核总线事务。
 *
 * 编译: gcc -g -O2 -Wall -pthread -o false_sharing_demo false_sharing_demo.c
 * 运行: ./false_sharing_demo
 * 配套: perf c2c record -p <pid> -- sleep 15 && perf c2c report  # 抓 HITM 证据
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define ITERS 200000000ull

/* ① 无填充：ticks 和 msgs 落在同一 cache line（0x00 与 0x08） */
struct counters_packed {
    volatile uint64_t ticks;   /* 线程 A 写 */
    volatile uint64_t msgs;    /* 线程 B 写 —— 同一 64B line！ */
};

/* ② 填充隔离：各占一条 cache line */
struct counters_padded {
    volatile uint64_t ticks;
    char _pad[64 - sizeof(uint64_t)];
    volatile uint64_t msgs;
    char _pad2[64 - sizeof(uint64_t)];
};

static struct counters_packed g_packed;
static struct counters_padded g_padded;

static double now_s(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* 用 __sync_fetch_and_add（lock xadd）而不是 volatile x++：
 * lock 前缀强制每次写都拿缓存行独占权，乒乓立刻显形（本机实测 3.2×）；
 * 而普通 volatile 写在新款 Intel 上几乎测不出差异（存储缓冲吸收 +
 * 依赖链本身已是瓶颈）——这本身就是一个观测口径的教训：怎么写决定你测到什么。 */
static void *worker_packed_a(void *_) { (void)_; for (uint64_t i = 0; i < ITERS; i++) __sync_fetch_and_add(&g_packed.ticks, 1); return NULL; }
static void *worker_packed_b(void *_) { (void)_; for (uint64_t i = 0; i < ITERS; i++) __sync_fetch_and_add(&g_packed.msgs, 1);  return NULL; }
static void *worker_padded_a(void *_) { (void)_; for (uint64_t i = 0; i < ITERS; i++) __sync_fetch_and_add(&g_padded.ticks, 1); return NULL; }
static void *worker_padded_b(void *_) { (void)_; for (uint64_t i = 0; i < ITERS; i++) __sync_fetch_and_add(&g_padded.msgs, 1);  return NULL; }

static void pin_to_core(pthread_t t, int core) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    pthread_setaffinity_np(t, sizeof(set), &set);
}

static double race(void *(*fa)(void *), void *(*fb)(void *)) {
    pthread_t ta, tb;
    double t0 = now_s();
    pthread_create(&ta, NULL, fa, NULL);
    pthread_create(&tb, NULL, fb, NULL);
    /* 绑到两个不同物理核（2 与 4），避免落 HT 兄弟/同核共享 L1 而测不出乒乓 */
    pin_to_core(ta, 2);
    pin_to_core(tb, 4);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    return now_s() - t0;
}

int main(void) {
    /* 打印地址证明：packed 两字段同 line，padded 不在 */
    printf("packed:  &ticks=%p &msgs=%p  距离=%ld B（同 line）\n",
           (void *)&g_packed.ticks, (void *)&g_packed.msgs,
           (char *)&g_packed.msgs - (char *)&g_packed.ticks);
    printf("padded:  &ticks=%p &msgs=%p  距离=%ld B（异 line）\n\n",
           (void *)&g_padded.ticks, (void *)&g_padded.msgs,
           (char *)&g_padded.msgs - (char *)&g_padded.ticks);

    double tp = race(worker_packed_a, worker_packed_b);
    double tq = race(worker_padded_a, worker_padded_b);

    printf("packed（伪共享）: %.3fs\n", tp);
    printf("padded（隔离）  : %.3fs\n", tq);
    printf("伪共享放大了 %.1f×\n", tp / tq);
    printf("\n取证命令: perf c2c record 跑 packed 版 → report 看 HITM 占比\n");
    return 0;
}
