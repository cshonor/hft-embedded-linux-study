/*
 * c6_1_cache_miss.c —— 「IPC 远小于 1」是什么体验：同一份计算，改个顺序就慢几十倍
 *
 * 6.1 说 perf stat 的 instructions/cycles ≪ 1 意味着程序「访存密集」——
 * CPU 大半时间在等内存（cache miss），不是在算。
 *
 * 本机（CE 容器）没有 PMU 权限，测不了真正的 IPC。但可以测**同一现象的另一面**：
 * 完全相同的加法次数，只改**访问顺序**，耗时能差几十倍。
 * 差掉的那部分，就是 CPU 在等 cache line —— 也就是 IPC 掉下去的那部分。
 *
 * 实验 A：二维数组按行遍历 vs 按列遍历
 *   两者加法次数完全一样（N*N 次），差别只在访存模式：
 *     按行：地址连续 → 一个 64B cache line 装 16 个 int，16 次访问只 miss 1 次
 *     按列：stride = N*4 字节 → 每访问一次就换一条 cache line，几乎全 miss
 *   这是硬件预取器（prefetcher）能否救你的分水岭。
 *
 * 实验 B：数组顺序走 vs 链表随机跳（pointer chasing）
 *   两者都是「读一个 int 决定下一步跳哪」。
 *   数组版地址可预测，预取器能提前把数据搬进 cache；
 *   链表版地址由数据本身决定，预取器完全无能为力 → 每次都是依赖链 + cache miss。
 *   这就是 6.1 HFT 关联第 2 条说的「追 cache、追内存」的真实代价。
 *
 * 用法：./c6_1_cache_miss
 * 编译：cc -g -O2 -Wall -Wextra -o c6_1_cache_miss c6_1_cache_miss.c
 *
 * ⚠️ 本文件测的是**本机墙钟时间**。绝对数字随机器型号、cache 容量、容器负载波动，
 *    不是常量。要看的不是绝对值，是**同一台机器上两种写法的倍数关系**。
 * ⚠️ 实验 A 的尺寸是刻意选的：N=4096 → 64 MiB，超过绝大多数 CPU 的 L3
 *    （8～32 MiB 常见）。如果把 N 调小到 2048（16 MiB），矩阵会整个装进 L3，
 *    按列遍历只慢 1.4 倍 —— 这个「拐点」本身就很能说明问题：慢了不是因为
 *    「算法差」，而是因为工作集**装不装得下**。
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N         4096          /* 矩阵边长；N*N*4B = 64 MiB，刻意超过 L3 */
#define CHASE_N   (1 << 20)     /* 1M 个节点，next[] 占 4 MiB */
#define CHASE_STEPS 5000000L

static int  m[N][N];        /* BSS，64 MiB —— 不 touch 就不占物理页 */
static int  next[CHASE_N];  /* 4 MiB */
static long sink;           /* 防止结果被优化掉 */

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* ---------------- 实验 A ---------------- */
static void exp_A(void)
{
    double t0, t1, t_row, t_col;
    long s;

    s = 0;
    t0 = now_ms();
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            s += m[i][j];                 /* 按行：地址连续 */
    t1 = now_ms();
    t_row = t1 - t0;
    sink += s;

    s = 0;
    t0 = now_ms();
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++)
            s += m[i][j];                 /* 按列：stride = N*4 B */
    t1 = now_ms();
    t_col = t1 - t0;
    sink += s;

    printf("实验 A：%d×%d 的 int 矩阵求和，矩阵 %.0f MiB（两边都是 %d 次加法）\n",
           N, N, (double)N * N * sizeof(int) / 1048576.0, N * N);
    printf("  按行遍历（地址连续，1 条 cache line 装 16 个 int）: %8.2f ms\n", t_row);
    printf("  按列遍历（stride %d B，每步换一条 cache line）  : %8.2f ms\n", N * 4, t_col);
    printf("  → 慢 %.1f 倍。加法次数一模一样，多出来的时间全花在等 cache line。\n\n",
           t_col / t_row);
}

/* ---------------- 实验 B ---------------- */
static void exp_B(void)
{
    double t0, t1, t_seq, t_chase;
    int    idx;

    /* 把 next[] 打乱成一个随机置换，模拟「链表节点散落在堆里」 */
    for (int i = 0; i < CHASE_N; i++) next[i] = i;
    unsigned int seed = 12345u;
    for (int i = CHASE_N - 1; i > 0; i--) {
        seed = seed * 1103515245u + 12345u;
        int j = (int)((seed >> 8) % (unsigned)(i + 1));
        int t = next[i]; next[i] = next[j]; next[j] = t;
    }

    /* 顺序走：地址可预测，预取器能帮上忙 */
    idx = 0;
    t0 = now_ms();
    for (long k = 0; k < CHASE_STEPS; k++) {
        idx = (idx + 1) & (CHASE_N - 1);
        sink += next[idx];
    }
    t1 = now_ms();
    t_seq = t1 - t0;

    /* 指针追逐：下一步地址由刚读到的数据决定，预取器彻底失效 */
    idx = 0;
    t0 = now_ms();
    for (long k = 0; k < CHASE_STEPS; k++) {
        idx = next[idx];                  /* ← 依赖链：必须等这次 load 回来才知道下一步 */
        sink += idx;
    }
    t1 = now_ms();
    t_chase = t1 - t0;

    printf("实验 B：%ld 次「读一个 int 决定下一步」（next[] 有 %d 个节点，%d KiB）\n",
           CHASE_STEPS, CHASE_N, CHASE_N * 4 / 1024);
    printf("  顺序下标（预取器有效）:      %8.2f ms\n", t_seq);
    printf("  指针追逐（地址依赖数据）:    %8.2f ms\n", t_chase);
    printf("  → 慢 %.1f 倍。这就是「追 cache」的真实价格，也是 HFT 里\n"
           "     为什么宁可用数组也不用链表的硬理由。\n\n", t_chase / t_seq);
}

int main(void)
{
    /* 先预热一遍，避免把 page fault 算进测量里 */
    for (int i = 0; i < N; i += 64)
        for (int j = 0; j < N; j += 64)
            m[i][j] = i + j;

    printf("=== 同一份计算，只改访存顺序 ===\n\n");
    exp_A();
    exp_B();
    printf("（sink=%ld，仅为阻止编译器把整段循环优化掉）\n", sink);
    return 0;
}
