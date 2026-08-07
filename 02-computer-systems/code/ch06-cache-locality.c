/*
 * CSAPP Ch6 · 缓存局部性 — 行优先 vs 列优先 + 分块/tiling
 *
 * 对照笔记:
 *   chapter-06/notes/section-6.2-局部性.md
 *   chapter-06/notes/section-6.4.2-直接映射.md
 *
 * 编译:
 *   gcc -Wall -Wextra -std=c11 -O2 -o ch06_cache ch06-cache-locality.c
 * 运行:
 *   ./ch06_cache
 *
 * HFT 关联: 订单簿遍历、风控矩阵运算必须 stride-1 访问
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DIM 4096  /* 4096x4096 matrix = 64MB (double), 远超 L1/L2 */

static double matrix[DIM][DIM];

/* ---------- 行优先求和: stride-1, 缓存友好 ---------- */
double sum_row_major(int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            sum += matrix[i][j];   /* 连续地址 — 空间局部性好 */
    return sum;
}

/* ---------- 列优先求和: stride-DIM, 缓存不友好 ---------- */
double sum_col_major(int n)
{
    double sum = 0.0;
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++)
            sum += matrix[i][j];   /* 每次跳 DIM*8 字节 — 跨 cache line */
    return sum;
}

/* ---------- 分块求和: 提高时间局部性 ---------- */
#define BLOCK 64  /* 64x64 block ~ 32KB, 适配 L1 */
double sum_blocked(int n)
{
    double sum = 0.0;
    for (int ii = 0; ii < n; ii += BLOCK)
        for (int jj = 0; jj < n; jj += BLOCK)
            for (int i = ii; i < ii + BLOCK && i < n; i++)
                for (int j = jj; j < jj + BLOCK && j < n; j++)
                    sum += matrix[i][j];
    return sum;
}

/* ---------- 结构体数组 vs 数组结构 (AoS vs SoA) ---------- */
struct OrderAoS {
    int    order_id;    /* 4B */
    double price;       /* 8B */
    int    qty;         /* 4B */
    double timestamp;   /* 8B */
};                      /* 24B + padding = 24 或 32B */

/* 只遍历 price 字段时, AoS 会拉入无用字段 (浪费 cache line) */
double sum_price_AoS(const struct OrderAoS *orders, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        sum += orders[i].price;   /* stride = sizeof(OrderAoS), 非 stride-1 */
    return sum;
}

/* SoA: 每个字段单独数组, 遍历 price 时 stride-1 */
struct OrderSoA {
    int    *order_id;
    double *price;
    int    *qty;
    double *timestamp;
};

double sum_price_SoA(const struct OrderSoA *orders, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        sum += orders->price[i];  /* stride-1, 缓存完美利用 */
    return sum;
}

/* ---------- 计时 ---------- */
static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void)
{
    printf("=== CSAPP Ch6 · 缓存局部性对比 (%dx%d matrix) ===\n\n", DIM, DIM);

    /* 初始化 */
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
            matrix[i][j] = (double)((i + j) % 100) * 0.01;

    double t0, t1, result;

    t0 = now_sec();
    result = sum_row_major(DIM);
    t1 = now_sec();
    printf("  行优先 (stride-1):   %8.3f ms   sum=%.1f\n",
           (t1 - t0) * 1000.0, result);

    t0 = now_sec();
    result = sum_col_major(DIM);
    t1 = now_sec();
    printf("  列优先 (stride-DIM): %8.3f ms   sum=%.1f\n",
           (t1 - t0) * 1000.0, result);

    t0 = now_sec();
    result = sum_blocked(DIM);
    t1 = now_sec();
    printf("  分块 (BLOCK=%d):     %8.3f ms   sum=%.1f\n",
           BLOCK, (t1 - t0) * 1000.0, result);

    /* AoS vs SoA */
    printf("\n=== AoS vs SoA (遍历 price 字段) ===\n\n");

    int n = 1024 * 1024;  /* 1M orders */
    struct OrderAoS *aos = malloc(sizeof(struct OrderAoS) * n);
    struct OrderSoA soa;
    soa.order_id  = malloc(sizeof(int) * n);
    soa.price     = malloc(sizeof(double) * n);
    soa.qty       = malloc(sizeof(int) * n);
    soa.timestamp = malloc(sizeof(double) * n);

    for (int i = 0; i < n; i++) {
        aos[i].order_id  = i;
        aos[i].price     = (double)(i % 1000) * 0.1;
        aos[i].qty       = i % 500;
        aos[i].timestamp = (double)i;
        soa.price[i]     = aos[i].price;
    }

    t0 = now_sec();
    double r1 = sum_price_AoS(aos, n);
    t1 = now_sec();
    printf("  AoS (stride=%zuB):  %8.3f ms   sum=%.1f\n",
           sizeof(struct OrderAoS), (t1 - t0) * 1000.0, r1);

    t0 = now_sec();
    double r2 = sum_price_SoA(&soa, n);
    t1 = now_sec();
    printf("  SoA (stride=8B):    %8.3f ms   sum=%.1f\n",
           (t1 - t0) * 1000.0, r2);

    printf("\n关键点:\n");
    printf("  1. 行优先 vs 列优先: 差 3-10x (矩阵越大越明显)\n");
    printf("  2. 分块: 当工作集 > cache 时, 分块提高时间局部性\n");
    printf("  3. SoA: 只遍历部分字段时, AoS 浪费 cache line 拉入无用数据\n");

    free(aos);
    free(soa.order_id); free(soa.price);
    free(soa.qty); free(soa.timestamp);
    return 0;
}
