/*
 * Hennessy Ch5 · 内存一致性模型 — C11 原子操作 + memory_order
 *
 * 对照笔记:
 *   03/chapter-05/notes/section-5.6-内存一致性模型.md
 *   03/chapter-05/notes/section-5.5-同步基础.md
 *   02/chapter-12/notes/section-12.7-其他并发问题.md
 *
 * 编译:
 *   gcc -Wall -Wextra -std=c11 -O2 -o ch05_ato ch05-atomics-ordering.c -lpthread
 * 运行:
 *   ./ch05_ato
 *
 * HFT 关联: 无锁结构的 publish 序 (写数据 → release store 索引)
 *           错误用 relaxed 读标志 → 看到半初始化对象
 */

#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

/* ---------- 1. memory_order 对比: Message Passing 模式 ---------- */
/* 线程 A 写 data, 然后 release store ready=1
 * 线程 B acquire load ready, 如果看到 1, 则能读到 data 的完整值
 *
 * 错误: 用 relaxed → 线程 B 可能看到 ready=1 但 data 还没写入
 */

typedef struct {
    int    payload[4];
    _Atomic int ready;
} Message;

static Message msg;
static _Atomic int seq_cst_reorder_count = 0;
static _Atomic int relaxed_fail_count = 0;

/* release-acquire 模式: 正确 */
static void *producer_release(void *arg)
{
    (void)arg;
    /* 写 payload */
    msg.payload[0] = 42;
    msg.payload[1] = 99;
    msg.payload[2] = 7;
    msg.payload[3] = 13;

    /* release store: 之前的写对 acquire load 可见 */
    atomic_store_explicit(&msg.ready, 1, memory_order_release);
    return NULL;
}

static void *consumer_acquire(void *arg)
{
    (void)arg;
    /* acquire load: 看到 ready=1 后, 之前的写都可见 */
    while (atomic_load_explicit(&msg.ready, memory_order_acquire) == 0)
        ; /* spin */

    if (msg.payload[0] != 42 || msg.payload[1] != 99 ||
        msg.payload[2] != 7   || msg.payload[3] != 13)
        atomic_fetch_add(&seq_cst_reorder_count, 1);
    return NULL;
}

/* relaxed 模式: 可能出错 (x86 上几乎不会, ARM 上会) */
static void *producer_relaxed(void *arg)
{
    (void)arg;
    msg.payload[0] = 42;
    msg.payload[1] = 99;
    msg.payload[2] = 7;
    msg.payload[3] = 13;

    /* relaxed: 不保证之前的写对其他线程可见! */
    atomic_store_explicit(&msg.ready, 1, memory_order_relaxed);
    return NULL;
}

static void *consumer_relaxed(void *arg)
{
    (void)arg;
    while (atomic_load_explicit(&msg.ready, memory_order_relaxed) == 0)
        ;

    if (msg.payload[0] != 42 || msg.payload[1] != 99 ||
        msg.payload[2] != 7   || msg.payload[3] != 13)
        atomic_fetch_add(&relaxed_fail_count, 1);
    return NULL;
}

/* ---------- 2. 自旋锁 (test-and-set) ---------- */
typedef struct {
    _Atomic int locked;
} SpinLock;

static void spin_lock(SpinLock *l)
{
    /* test-and-set: 期望 0, 交换为 1; 如果已经是 1 则自旋 */
    while (atomic_exchange(&l->locked, 1) == 1)
        ; /* 自旋 — 生产环境应加 pause / backoff */
}

static void spin_unlock(SpinLock *l)
{
    atomic_store(&l->locked, 0);  /* release 语义 */
}

static SpinLock slock;
static int shared_counter = 0;
#define LOCK_ITERS 1000000

static void *lock_worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < LOCK_ITERS; i++) {
        spin_lock(&slock);
        shared_counter++;
        spin_unlock(&slock);
    }
    return NULL;
}

/* ---------- 3. fetch_add 原子计数器 ---------- */
static _Atomic uint64_t counter = 0;

static void *atomic_counter_worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < LOCK_ITERS; i++)
        atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);
    return NULL;
}

/* ---------- 运行 ---------- */
#define NTHREADS 4

static void test_message_passing(void)
{
    printf("--- 1. Message Passing (release-acquire vs relaxed) ---\n\n");

    int trials = 10000;
    for (int i = 0; i < trials; i++) {
        atomic_store(&msg.ready, 0);
        msg.payload[0] = msg.payload[1] = msg.payload[2] = msg.payload[3] = 0;

        pthread_t prod, cons;
        pthread_create(&prod, NULL, producer_release, NULL);
        pthread_create(&cons, NULL, consumer_acquire, NULL);
        pthread_join(prod, NULL);
        pthread_join(cons, NULL);
    }
    printf("  release-acquire: %d trials, %d failures (期望 0)\n",
           trials, atomic_load(&seq_cst_reorder_count));

    for (int i = 0; i < trials; i++) {
        atomic_store(&msg.ready, 0);
        msg.payload[0] = msg.payload[1] = msg.payload[2] = msg.payload[3] = 0;

        pthread_t prod, cons;
        pthread_create(&prod, NULL, producer_relaxed, NULL);
        pthread_create(&cons, NULL, consumer_relaxed, NULL);
        pthread_join(prod, NULL);
        pthread_join(cons, NULL);
    }
    printf("  relaxed:          %d trials, %d failures (x86 上通常 0, ARM 上可能 >0)\n\n",
           trials, atomic_load(&relaxed_fail_count));
}

static void test_spinlock(void)
{
    printf("--- 2. 自旋锁 (atomic exchange) ---\n\n");

    atomic_store(&slock.locked, 0);
    shared_counter = 0;

    pthread_t tids[NTHREADS];
    for (int i = 0; i < NTHREADS; i++)
        pthread_create(&tids[i], NULL, lock_worker, NULL);
    for (int i = 0; i < NTHREADS; i++)
        pthread_join(tids[i], NULL);

    printf("  自旋锁保护: counter=%d (期望=%d)\n\n",
           shared_counter, NTHREADS * LOCK_ITERS);
}

static void test_atomic_counter(void)
{
    printf("--- 3. 原子计数器 (fetch_add relaxed) ---\n\n");

    atomic_store(&counter, 0);

    pthread_t tids[NTHREADS];
    for (int i = 0; i < NTHREADS; i++)
        pthread_create(&tids[i], NULL, atomic_counter_worker, NULL);
    for (int i = 0; i < NTHREADS; i++)
        pthread_join(tids[i], NULL);

    printf("  fetch_add: counter=%lu (期望=%d)\n\n",
           (unsigned long)atomic_load(&counter),
           NTHREADS * LOCK_ITERS);
}

int main(void)
{
    printf("=== Hennessy Ch5 · 内存一致性模型 (C11 atomics) ===\n\n");

    test_message_passing();
    test_spinlock();
    test_atomic_counter();

    printf("memory_order 速查:\n");
    printf("  relaxed:     只保证原子, 不保证顺序 (计数器可用)\n");
    printf("  acquire:     load 后续读写不能重排到此 load 前\n");
    printf("  release:     store 之前读写不能重排到此 store 后\n");
    printf("  seq_cst:     全局顺序一致, 最强保证, 开销最大\n");
    printf("\n  HFT: SPSC 环形缓冲 publish 序:\n");
    printf("       写 data → release store index → acquire load index → 读 data\n");
    printf("       错误: relaxed store index → 消费者可能看到半写入的 data\n");

    return 0;
}
