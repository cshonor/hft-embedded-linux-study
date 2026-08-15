/*
 * Hennessy Ch5 · 无锁 SPSC 环形队列 — C11 atomics
 *
 * 对照笔记:
 *   03/chapter-05/notes/section-5.6-内存一致性模型.md (release-acquire)
 *   03/chapter-05/notes/section-5.5-同步基础.md (CAS/LL-SC)
 *   02/chapter-12/notes/section-12.7-其他并发问题.md
 *
 * 编译:
 *   gcc -Wall -Wextra -std=c11 -O2 -o ch05_spsc ch05-spsc-ringbuf.c -lpthread
 * 运行:
 *   ./ch05_spsc
 *
 * HFT 关联: SPSC 是 HFT 最核心的无锁结构
 *   - 网卡收包线程 → SPSC → 策略线程
 *   - 策略线程 → SPSC → 下单线程
 *   - 每个 pipeline stage 之间用 SPSC 解耦
 *
 * 关键: publish 序 (写 data → release store tail)
 *       consume 序 (acquire load tail → 读 data)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define CAPACITY 1024  /* 必须是 2 的幂 — 用 & 代替 % */
#define MASK      (CAPACITY - 1)

/* ---------- SPSC 环形缓冲 ---------- */
typedef struct {
    void   *slots[CAPACITY];    /* 数据槽 */
    _Atomic uint64_t head;      /* 生产者写位置 (只生产者写) */
    _Atomic uint64_t tail;      /* 消费者读位置 (只消费者写) */
    char   pad1[64];            /* 隔离 head 和 tail 到不同 cache line */
} SpscQueue;

/* 初始化 */
static void spsc_init(SpscQueue *q)
{
    memset(q->slots, 0, sizeof(q->slots));
    atomic_store(&q->head, 0);
    atomic_store(&q->tail, 0);
}

/* 生产者: 入队 (单线程调用) */
static bool spsc_enqueue(SpscQueue *q, void *item)
{
    uint64_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    uint64_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
    /* acquire load tail: 看到 tail 后, 消费者已读完的槽位数据可见 */

    /* 队列满? head - tail == CAPACITY */
    if (head - tail >= CAPACITY)
        return false;  /* 满, 入队失败 */

    /* 写数据 */
    q->slots[head & MASK] = item;

    /* release store head: 之前的写 (slots) 对消费者可见 */
    atomic_store_explicit(&q->head, head + 1, memory_order_release);
    return true;
}

/* 消费者: 出队 (单线程调用) */
static bool spsc_dequeue(SpscQueue *q, void **item)
{
    uint64_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    uint64_t head = atomic_load_explicit(&q->head, memory_order_acquire);
    /* acquire load head: 看到 head 后, 生产者写入的 slots 数据可见 */

    /* 队列空? head == tail */
    if (head == tail)
        return false;  /* 空, 出队失败 */

    /* 读数据 */
    *item = q->slots[tail & MASK];

    /* release store tail: 之前的读完成, 生产者可复用此槽位 */
    atomic_store_explicit(&q->tail, tail + 1, memory_order_release);
    return true;
}

/* ---------- 测试: 生产者-消费者吞吐量 ---------- */
static SpscQueue queue __attribute__((aligned(64)));
static _Atomic uint64_t total_enqueued = 0;
static _Atomic uint64_t total_dequeued = 0;
static _Atomic bool producer_done = false;

#define NITEMS 10000000  /* 1000 万 */

static void *producer_fn(void *arg)
{
    (void)arg;
    uint64_t count = 0;
    for (uint64_t i = 0; i < NITEMS; i++) {
        /* 用值 (void*)(i+1) 作为 item, 0 = 空 */
        void *item = (void *)((i + 1) == 0 ? (uintptr_t)1 : (i + 1));
        while (!spsc_enqueue(&queue, item))
            ; /* 自旋等空位 — 生产环境可加 backoff */
        count++;
    }
    atomic_store(&producer_done, true);
    atomic_store(&total_enqueued, count);
    return NULL;
}

static void *consumer_fn(void *arg)
{
    (void)arg;
    uint64_t count = 0;
    void *item;

    while (true) {
        if (spsc_dequeue(&queue, &item)) {
            count++;
        } else {
            /* 队列空, 检查生产者是否完成 */
            if (atomic_load(&producer_done) &&
                !spsc_dequeue(&queue, &item))
                break;
        }
    }
    atomic_store(&total_dequeued, count);
    return NULL;
}

/* ---------- 正确性验证: 值按顺序到达 ---------- */
static void test_ordering(void)
{
    printf("--- 1. FIFO 顺序验证 ---\n\n");

    SpscQueue q __attribute__((aligned(64)));
    spsc_init(&q);

    /* 入队 0,1,2,...,9 */
    for (int i = 0; i < 10; i++)
        spsc_enqueue(&q, (void *)(uintptr_t)(i + 1));

    void *item;
    int expected = 1;
    bool ok = true;
    while (spsc_dequeue(&q, &item)) {
        if ((int)(uintptr_t)item != expected) {
            printf("  顺序错误: 期望 %d, 实际 %d\n", expected, (int)(uintptr_t)item);
            ok = false;
        }
        expected++;
    }
    printf("  入队 10 项, 出队 %d 项, 顺序%s\n\n", expected - 1, ok ? "正确 ✓" : "错误 ✗");
}

int main(void)
{
    printf("=== Hennessy Ch5 · 无锁 SPSC 环形队列 (C11) ===\n\n");
    printf("  CAPACITY=%d (2 的幂, & MASK 代替 %% CAPACITY)\n\n", CAPACITY);

    test_ordering();

    /* 吞吐量测试 */
    printf("--- 2. 吞吐量测试 (%d 万 items) ---\n\n", NITEMS / 10000);

    spsc_init(&queue);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer_fn, NULL);
    pthread_create(&cons, NULL, consumer_fn, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;

    printf("  入队: %lu\n", (unsigned long)atomic_load(&total_enqueued));
    printf("  出队: %lu\n", (unsigned long)atomic_load(&total_dequeued));
    printf("  耗时: %.3f ms\n", elapsed * 1000.0);
    printf("  吞吐: %.1f M ops/sec\n",
           (double)NITEMS / elapsed / 1e6);

    printf("\n内存序关键:\n");
    printf("  生产者: 写 slots[head] → release store head+1\n");
    printf("  消费者: acquire load head → 读 slots[tail]\n");
    printf("  保证: 消费者看到 head 更新后, 必定看到 slots 数据\n");
    printf("\n  HFT pipeline:\n");
    printf("    NIC RX → SPSC → 策略引擎 → SPSC → 下单线程\n");
    printf("    每个 stage 单线程, stage 间用 SPSC 解耦\n");
    printf("    无锁 → 无 mutex → 无内核态切换 → 微秒级延迟\n");

    return 0;
}
