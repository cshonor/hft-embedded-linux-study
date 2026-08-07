/*
 * CSAPP Ch5 · 优化程序性能 — 循环展开 + restrict + 累加器
 *
 * 对照笔记:
 *   chapter-05/notes/section-5.6-消除不必要的内存引用.md
 *   chapter-05/notes/section-5.8-循环展开.md
 *   chapter-05/notes/section-5.9-提高并行性.md
 *
 * 编译:
 *   gcc -Wall -Wextra -std=c11 -O2 -o ch05_opt ch05-optimization.c -lm
 *   # 对比不同优化级别:
 *   gcc -Wall -std=c11 -O0 -o ch05_opt_o0 ch05-optimization.c -lm
 *   gcc -Wall -std=c11 -O2 -o ch05_opt_o2 ch05-optimization.c -lm
 * 运行:
 *   ./ch05_opt
 *
 * HFT 关联: 热循环里的累加器必须用寄存器，不能反复读写内存
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N (1024 * 1024 * 16)  /* 16M elements */

/* ---------- 版本 1: 原始 — 每轮读写 *dest (内存瓶颈) ---------- */
void combine1(double *data, int len, double *dest)
{
    *dest = 0.0;
    for (int i = 0; i < len; i++)
        *dest = *dest + data[i];  /* 每轮 load + store */
}

/* ---------- 版本 2: 累加器提到寄存器 ---------- */
void combine2(double *data, int len, double *dest)
{
    double acc = 0.0;             /* 寄存器累加 */
    for (int i = 0; i < len; i++)
        acc = acc + data[i];
    *dest = acc;                  /* 循环结束后才写一次 */
}

/* ---------- 版本 3: restrict 承诺无别名 ---------- */
void combine3(double *restrict data, int len, double *restrict dest)
{
    double acc = 0.0;
    for (int i = 0; i < len; i++)
        acc += data[i];
    *dest = acc;
}

/* ---------- 版本 4: 2x1 循环展开 ---------- */
void combine4(double *restrict data, int len, double *restrict dest)
{
    double acc = 0.0;
    int limit = len - 1;
    int i;
    for (i = 0; i < limit; i += 2) {
        acc = (acc + data[i]) + data[i + 1];  /* 两次加法可并行 */
    }
    for (; i < len; i++)
        acc += data[i];
    *dest = acc;
}

/* ---------- 版本 5: 2x2 循环展开 + 双累加器 (指令级并行) ---------- */
void combine5(double *restrict data, int len, double *restrict dest)
{
    double acc0 = 0.0, acc1 = 0.0;
    int limit = len - 1;
    int i;
    for (i = 0; i < limit; i += 2) {
        acc0 += data[i];     /* 独立累加器 — 打破依赖链 */
        acc1 += data[i + 1];
    }
    for (; i < len; i++)
        acc0 += data[i];
    *dest = acc0 + acc1;
}

/* ---------- 计时工具 ---------- */
static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

#define BENCH(fn, label) do {                        \
    double dest = 0.0, t0 = now_sec();               \
    fn(data, N, &dest);                              \
    double t1 = now_sec();                           \
    printf("  %-28s  %8.3f ms   result=%.1f\n",      \
           label, (t1 - t0) * 1000.0, dest);         \
} while (0)

int main(void)
{
    double *data = malloc(sizeof(double) * N);
    if (!data) { perror("malloc"); return 1; }

    /* 初始化数据 */
    for (int i = 0; i < N; i++)
        data[i] = (double)(i % 100) * 0.01;

    printf("=== CSAPP Ch5 · 优化对比 (N=%d) ===\n\n", N);

    BENCH(combine1, "v1 原始 (内存读写)");
    BENCH(combine2, "v2 累加器→寄存器");
    BENCH(combine3, "v3 +restrict");
    BENCH(combine4, "v4 2x1 循环展开");
    BENCH(combine5, "v5 2x2 双累加器");

    printf("\n关键点:\n");
    printf("  1. v1→v2: 消除每轮内存读写，最大提升\n");
    printf("  2. v2→v3: restrict 让编译器敢做更激进的优化\n");
    printf("  3. v4: 循环展开减少分支开销\n");
    printf("  4. v5: 双累加器打破依赖链，利用 ILP\n");
    printf("\n  对比 -O0 vs -O2 看 v2/v3 差异 — restrict 在 -O2 效果明显\n");

    free(data);
    return 0;
}
