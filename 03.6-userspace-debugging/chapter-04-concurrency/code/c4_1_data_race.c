/*
 * c4_1_data_race.c —— Ch4 贯穿 demo：同一件事的四种写法，只有三种是对的
 *
 * 需求：两个线程各做 N 次自增，最后应该得到 2N。
 *
 * 用法：
 *   ./c4_1_data_race race     # 裸自增，无同步         → 数据竞争（丢更新）
 *   ./c4_1_data_race mutex    # 互斥锁                 → 结果对
 *   ./c4_1_data_race atomic   # C11 原子操作           → 结果对
 *   ./c4_1_data_race local    # 每线程私有 + 最后合并 → 结果对，且最快
 *
 * 编译（查竞争，必须带 TSan）：
 *   cc -g -O1 -fsanitize=thread -pthread -o c4_1_tsan c4_1_data_race.c
 *   ./c4_1_tsan race          # → TSan 报告 + 退出码 66
 *   ./c4_1_tsan mutex         # → 无报告 + 退出码 0
 *
 * 编译（只看结果对不对，不带 sanitizer；此时才谈得上比较耗时）：
 *   cc -g -O2 -pthread -o c4_1 c4_1_data_race.c
 *
 * 关键理解一：一行 C 代码 ≠ 一个原子操作
 *   g_race++ 在机器层面是三条指令 —— load / add / store。
 *   两个线程的这三条指令交错时就会互相覆盖：A 读 100，B 也读 100，
 *   各自加一写回 101 —— 两次自增只涨了 1。这叫「读-改-写」竞争（RMW）。
 *
 * 关键理解二：为什么四种写法里都塞了 tiny_work()？
 *   ① 为了让四种写法的「计算量」对等，耗时才可比；
 *   ② 对 race 模式尤其重要：它把「读」和「写回」之间的窗口显式撑开。
 *      现实里这个窗口由 cache miss、分支预测失败、调度抢占自然撑开，
 *      手动撑开只是为了让它稳定可见、不靠运气。
 *
 * 关键理解三：共享状态会杀死优化
 *   local 模式里 local++ 是线程私有的，编译器敢把整圈折叠；
 *   而 race/mutex/atomic 三种共享写法，编译器一步都不敢省。
 *   这就是 HFT「把共享写入降到最少」的编译器版本收益。
 */

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NITER    100000   /* 每线程自增次数 */
#define NTHREAD  2
#define SPIN     40       /* per-iteration 的假活量（拉宽竞争窗口 / 拉平计算量） */

/* 是否在 TSan 下编译 —— TSan 让耗时彻底失去意义，所以只跑 1 次 */
#if defined(__SANITIZE_THREAD__)
#  define UNDER_TSAN 1
#elif defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define UNDER_TSAN 1
#  endif
#endif
#ifndef UNDER_TSAN
#  define UNDER_TSAN 0
#endif
#define REPEAT (UNDER_TSAN ? 1 : 3)   /* 非 TSan 下取 3 次最小值，压掉容器调度噪声 */

/* ------------------------------------------------------------------ *
 * 写法 1：裸自增 —— 有数据竞争
 * 这里把 g_race++ 显式拆成 load / work / store 三步，好让窗口看得见。
 * 就 C 语义而言，它和直接写 g_race++ 是同一件事。
 * ------------------------------------------------------------------ */
static int g_race = 0;

/* ------------------------------------------------------------------ *
 * 写法 2：互斥锁 —— 通用，但有争用开销
 * ------------------------------------------------------------------ */
static int g_mutex = 0;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/* ------------------------------------------------------------------ *
 * 写法 3：C11 原子操作 —— 无锁，HFT 常用
 * ------------------------------------------------------------------ */
static atomic_int g_atomic = 0;

/* ------------------------------------------------------------------ *
 * 写法 4：每线程私有累加，最后合并一次 —— HFT 首选
 * ------------------------------------------------------------------ */
static long g_local_sum[NTHREAD];

/* ------------------------------------------------------------------ */

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* 不被优化掉的假活：volatile 循环必须真的执行 */
static inline void tiny_work(void) {
    for (volatile int k = 0; k < SPIN; k++) { /* 空转 */ }
}

static void *th_race(void *arg) {
    (void)arg;
    for (int i = 0; i < NITER; i++) {
        int v = g_race;        /* ① 读 */
        tiny_work();           /*    ← 「读」与「写回」之间的窗口 */
        g_race = v + 1;        /* ② 改 ③ 写回 */
    }
    return NULL;
}

static void *th_mutex(void *arg) {
    (void)arg;
    for (int i = 0; i < NITER; i++) {
        pthread_mutex_lock(&g_lock);
        g_mutex++;                                     /* ← 临界区 */
        tiny_work();
        pthread_mutex_unlock(&g_lock);
    }
    return NULL;
}

static void *th_atomic(void *arg) {
    (void)arg;
    for (int i = 0; i < NITER; i++) {
        atomic_fetch_add(&g_atomic, 1);                /* ← 无锁原子 */
        tiny_work();
    }
    return NULL;
}

static void *th_local(void *arg) {
    int id = *(int *)arg;
    long local = 0;                                    /* 栈上，线程私有 */
    for (int i = 0; i < NITER; i++) {
        local++;                                       /* ← 零共享、零同步 */
        tiny_work();
    }
    pthread_mutex_lock(&g_lock);                       /* 只在合并时锁一次 */
    g_local_sum[id] = local;
    pthread_mutex_unlock(&g_lock);
    return NULL;
}

static void run_threads(void *(*fn)(void *), int with_arg) {
    pthread_t t[NTHREAD];
    int ids[NTHREAD];
    for (int i = 0; i < NTHREAD; i++) {
        ids[i] = i;
        pthread_create(&t[i], NULL, fn, with_arg ? &ids[i] : NULL);
    }
    for (int i = 0; i < NTHREAD; i++) pthread_join(t[i], NULL);
}

int main(int argc, char **argv) {
    const char *mode = (argc > 1) ? argv[1] : "race";
    const long expect = (long)NITER * NTHREAD;
    double best = 1e18;
    long got = 0;
    int i;

    printf("每线程 %d 次自增 × %d 个线程，期望结果 %ld；每轮附 %d 次假活\n",
           NITER, NTHREAD, expect, SPIN);
    printf("模式 = %s   重复 %d 次取最小耗时\n\n", mode, REPEAT);

    for (i = 0; i < REPEAT; i++) {
        double t0, t1;
        g_race = 0; g_mutex = 0; atomic_store(&g_atomic, 0);
        g_local_sum[0] = g_local_sum[1] = 0;

        if (strcmp(mode, "race") == 0) {
            t0 = now_ms(); run_threads(th_race, 0);  t1 = now_ms(); got = g_race;
        } else if (strcmp(mode, "mutex") == 0) {
            t0 = now_ms(); run_threads(th_mutex, 0); t1 = now_ms(); got = g_mutex;
        } else if (strcmp(mode, "atomic") == 0) {
            t0 = now_ms(); run_threads(th_atomic, 0); t1 = now_ms();
            got = atomic_load(&g_atomic);
        } else if (strcmp(mode, "local") == 0) {
            t0 = now_ms(); run_threads(th_local, 1); t1 = now_ms();
            got = g_local_sum[0] + g_local_sum[1];
        } else {
            fprintf(stderr, "用法: %s {race|mutex|atomic|local}\n", argv[0]);
            return 2;
        }
        if (t1 - t0 < best) best = t1 - t0;
    }

    printf("\n实际结果 = %ld   （丢了 %ld 次更新，%.2f%%）\n",
           got, expect - got, 100.0 * (expect - got) / expect);
    printf("耗时     = %.1f ms（%d 次最小值）\n", best, REPEAT);
    if (UNDER_TSAN)
        printf("           （TSan 插桩后耗时无参考价值，只跑 1 次）\n");

    if (got != expect) {
        printf("\n注意：进程**没有崩**，退出码仍是 0，stderr 也是空的。\n"
               "      结果错了，进程却「成功」了 —— 这就是 1.2 决策树里\n"
               "      「结果偶尔不对」必须用 TSan、不能盯退出码的原因。\n");
    } else {
        printf("\n注意：这一次结果**恰好对了**，但竞争依然存在。\n"
               "      换成 TSan 编译再跑，它照样会报 —— TSan 检查的是\n"
               "      「竞争关系」本身，不是「竞争的后果」。这就是它比\n"
               "      「多跑几遍看结果」可靠的地方。\n");
    }
    return 0;
}
