/*
 * CSAPP Ch12 · 并发编程 — pthread + 互斥 + 生产者-消费者 + 竞态演示
 *
 * 对照笔记:
 *   chapter-12/notes/section-12.3-基于线程的并发编程.md
 *   chapter-12/notes/section-12.4-多线程程序中的共享变量.md
 *   chapter-12/notes/section-12.5-信号量与预线程化.md
 *
 * 编译:
 *   gcc -Wall -Wextra -std=c11 -O2 -o ch12_conc ch12-concurrency.c -lpthread
 * 运行:
 *   ./ch12_conc
 *
 * HFT 关联: 多线程订单处理、共享计数器、无锁 vs 有锁
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

#define NTHREADS 4
#define NLOOP    1000000

/* ---------- 1. 竞态条件演示 ---------- */
static int counter_unsafe = 0;  /* 无保护 — 有竞态 */

static void *race_unsafe(void *arg)
{
    (void)arg;
    for (int i = 0; i < NLOOP; i++)
        counter_unsafe++;  /* ++ 不是原子操作: load, add, store */
    return NULL;
}

/* ---------- 2. 互斥锁保护 ---------- */
static int counter_mutex = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static void *race_mutex(void *arg)
{
    (void)arg;
    for (int i = 0; i < NLOOP; i++) {
        pthread_mutex_lock(&mtx);
        counter_mutex++;
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

/* ---------- 3. C11 原子操作 ---------- */
static atomic_int counter_atomic = 0;

static void *race_atomic(void *arg)
{
    (void)arg;
    for (int i = 0; i < NLOOP; i++)
        atomic_fetch_add(&counter_atomic, 1);  /* 原子加 */
    return NULL;
}

/* ---------- 4. 生产者-消费者 (信号量) ---------- */
#define BUFSZ 8

static int buffer[BUFSZ];
static int buf_in = 0, buf_out = 0;

static sem_t sem_empty;   /* 空槽数, 初始 BUFSZ */
static sem_t sem_full;    /* 满槽数, 初始 0 */
static pthread_mutex_t buf_mtx = PTHREAD_MUTEX_INITIALIZER;

static void *producer(void *arg)
{
    int id = *(int *)arg;
    for (int i = 0; i < 1000; i++) {
        int item = id * 1000 + i;

        sem_wait(&sem_empty);              /* 等空槽 */
        pthread_mutex_lock(&buf_mtx);
        buffer[buf_in] = item;
        buf_in = (buf_in + 1) % BUFSZ;
        pthread_mutex_unlock(&buf_mtx);
        sem_post(&sem_full);               /* 通知消费者 */

        if (i % 200 == 0)
            printf("  [P%d] 生产 item=%d\n", id, item);
    }
    return NULL;
}

static void *consumer(void *arg)
{
    int id = *(int *)arg;
    for (int i = 0; i < 1000; i++) {
        sem_wait(&sem_full);               /* 等数据 */
        pthread_mutex_lock(&buf_mtx);
        int item = buffer[buf_out];
        buf_out = (buf_out + 1) % BUFSZ;
        pthread_mutex_unlock(&buf_mtx);
        sem_post(&sem_empty);              /* 通知生产者 */

        if (i % 200 == 0)
            printf("  [C%d] 消费 item=%d\n", id, item);
    }
    return NULL;
}

/* ---------- 运行多线程测试 ---------- */
static void run_threads(void *(*fn)(void *), int nthreads, const char *label)
{
    pthread_t tids[NTHREADS];
    int ids[NTHREADS];

    for (int i = 0; i < nthreads; i++) {
        ids[i] = i;
        pthread_create(&tids[i], NULL, fn, &ids[i]);
    }
    for (int i = 0; i < nthreads; i++)
        pthread_join(tids[i], NULL);

    printf("  %-28s  result=%d  (期望=%d)\n",
           label, 
           fn == race_unsafe ? counter_unsafe :
           fn == race_mutex  ? counter_mutex  :
           atomic_load(&counter_atomic),
           nthreads * NLOOP);
}

int main(void)
{
    printf("=== CSAPP Ch12 · 并发编程 (C/pthread) ===\n\n");

    /* 竞态对比 */
    printf("--- 1. 竞态条件对比 (%d threads × %d loops) ---\n\n", NTHREADS, NLOOP);
    run_threads(race_unsafe, NTHREADS, "无保护 (有竞态)");
    run_threads(race_mutex,  NTHREADS, "pthread_mutex");
    run_threads(race_atomic, NTHREADS, "C11 atomic");

    printf("\n  注意: 无保护版本 result < 期望值 (丢失更新)\n");
    printf("  mutex vs atomic: 原子操作无锁, 但仅适用于简单操作\n\n");

    /* 生产者-消费者 */
    printf("--- 2. 生产者-消费者 (信号量) ---\n\n");

    sem_init(&sem_empty, 0, BUFSZ);
    sem_init(&sem_full,  0, 0);

    pthread_t prod[2], cons[2];
    int pids[2] = {1, 2};
    int cids[2] = {1, 2};

    for (int i = 0; i < 2; i++) {
        pthread_create(&prod[i], NULL, producer, &pids[i]);
        pthread_create(&cons[i], NULL, consumer, &cids[i]);
    }
    for (int i = 0; i < 2; i++) {
        pthread_join(prod[i], NULL);
        pthread_join(cons[i], NULL);
    }

    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);

    printf("\n关键点:\n");
    printf("  1. ++ 不是原子操作: load→add→store 三步, 多线程会丢失更新\n");
    printf("  2. mutex: 通用但开销大 (可能陷入内核)\n");
    printf("  3. atomic: 无锁, 适合简单计数器\n");
    printf("  4. 信号量: 生产者-消费者经典模式\n");
    printf("  HFT: 热路径用无锁队列 (SPSC), 避免 mutex/pthread_cond_wait\n");

    return 0;
}
