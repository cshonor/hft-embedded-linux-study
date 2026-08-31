/* SPDX-License-Identifier: BSD-3-Clause
 *
 * 分位直方图 —— header-only，热路径全部 inline
 *
 * 设计取舍（与 12.5/chapter-15/notes/03-latency-measurement.md 一致）：
 *   1. 记录的是 "ticks"，语义由调用方决定（DPDK 传 rdtsc cycles，socket 版传 ns）
 *      理由：热路径只做整数除法 + 数组自增，把浮点换算推到报表时才做。
 *   2. 线性桶而非 HdrHistogram 的对数桶：精度受桶宽限制（±64 tick），
 *      对 μs 级测量够用，实现只需几十行。要跨 ns~s 多量级再换 HdrHistogram。
 *   3. 单线程无锁：HFT 收包是一个核一条队列一个循环，不需要跨线程聚合。
 *      若多核各记各的，每个核一个 struct hist 并让数组按 cacheline 对齐
 *      （见下面的 aligned(64)），报表时再合并 —— 避免伪共享。
 */

#ifndef HIST_H
#define HIST_H

#include <stdint.h>
#include <stdio.h>

/* 4096 桶 × 64 tick = 262144 tick 量程。
   3GHz TSC 下约 87μs；socket 版（tick=ns）约 262μs。
   超出量程的样本进 overflow —— 若 overflow 持续增长，把桶宽调大。 */
#define HIST_BUCKETS         4096u
#define HIST_TICKS_PER_BUCKET 64u

struct hist {
    uint64_t buckets[HIST_BUCKETS];
    uint64_t overflow;
    uint64_t count;
} __attribute__((aligned(64)));

static inline void hist_init(struct hist *h)
{
    /* 32KB，只在启动时清一次；热路径不再触碰 */
    for (uint32_t i = 0; i < HIST_BUCKETS; i++)
        h->buckets[i] = 0;
    h->overflow = 0;
    h->count = 0;
}

/* 热路径：一次除法 + 一次自增。__builtin_expect 提示正常包远快于量程上限 */
static inline void hist_record(struct hist *h, uint64_t ticks)
{
    uint64_t idx = ticks / HIST_TICKS_PER_BUCKET;
    if (__builtin_expect(idx < HIST_BUCKETS, 1))
        h->buckets[idx]++;
    else
        h->overflow++;
    h->count++;
}

/* 分位数：从最慢的桶往回累加，累计达到"应有 need 个样本比它慢"时的桶上界。
   返回 tick 单位；溢出区返回 UINT64_MAX 表示"比量程还慢"。

   ★ need 必须向上取整，不能截断 —— 这是实测出来的坑：
     · 截断会让 need 偏小，溢出样本略少于 (1-q) 比例时，
       p999 被误判为"落在溢出区"，返回无意义值；
     · 小样本时会把 max 当成 p999（例：1001 个样本里 1 个极慢值，
       截断版返回那个极慢值，正确值应是第二慢的），尾延迟被系统性高估。
   不用 ceil() 是为了避免链接 libm（DPDK 的 Makefile 未必带 -lm）。 */
static inline uint64_t hist_quantile(const struct hist *h, double q)
{
    if (h->count == 0)
        return 0;

    double   rank  = (double)h->count * (1.0 - q);
    uint64_t trunc = (uint64_t)rank;
    uint64_t need  = (rank > (double)trunc) ? trunc + 1 : trunc;
    if (need == 0)                 /* q 极接近 1 或样本极少：至少看最慢的一个 */
        need = 1;

    uint64_t acc = h->overflow;
    if (acc >= need)
        return UINT64_MAX;

    for (int i = (int)HIST_BUCKETS - 1; i >= 0; i--) {
        acc += h->buckets[i];
        if (acc >= need)
            return (uint64_t)(i + 1) * HIST_TICKS_PER_BUCKET;
    }
    return 0;
}

static inline double hist_mean(const struct hist *h)
{
    if (h->count == 0)
        return 0.0;
    uint64_t sum = 0;
    for (uint32_t i = 0; i < HIST_BUCKETS; i++)
        sum += h->buckets[i] * ((uint64_t)i * HIST_TICKS_PER_BUCKET
                                + HIST_TICKS_PER_BUCKET / 2);
    sum += h->overflow * ((uint64_t)HIST_BUCKETS * HIST_TICKS_PER_BUCKET);
    return (double)sum / (double)h->count;
}

/* ns_per_tick: DPDK 传 1e9/rte_get_tsc_hz()，socket 版传 1.0（tick 就是 ns） */
static inline void hist_dump(const struct hist *h, double ns_per_tick,
                             const char *title)
{
    printf("\n%s  (样本 %llu, 溢出 %llu)\n", title,
           (unsigned long long)h->count, (unsigned long long)h->overflow);
    printf("  %-8s %-10s %-10s %-10s %-10s %-10s\n",
           "p50", "p99", "p999", "p9999", "max", "mean");

    /* ★ 超量程的分位数绝不能打印成 0 ★
       真实含义是"慢到量程装不下"，显示成 0 会被读成"快得不可思议"，
       恰好相反 —— 而尾延迟恰恰是最该被看见的那个数。统一标记 ">量程"。 */
    char pbuf[4][24];
    const double qs[4] = { 0.50, 0.99, 0.999, 0.9999 };
    for (int i = 0; i < 4; i++) {
        uint64_t t = hist_quantile(h, qs[i]);
        if (t == UINT64_MAX)
            snprintf(pbuf[i], sizeof(pbuf[i]), ">量程");
        else
            snprintf(pbuf[i], sizeof(pbuf[i]), "%llu",
                     (unsigned long long)((double)t * ns_per_tick));
    }

    uint64_t max_t = 0;
    for (int i = (int)HIST_BUCKETS - 1; i >= 0; i--) {
        if (h->buckets[i]) {
            max_t = (uint64_t)(i + 1) * HIST_TICKS_PER_BUCKET;
            break;
        }
    }

    /* 有溢出样本时真实 max 一定大于量程上限，拿量程内的最大值冒充 max 会低估尾部。
       同理 mean 也只是下界（溢出样本被按量程上限计入），标 ">=" 而不是给个假精确值。 */
    char maxbuf[24], meanbuf[24];
    if (h->overflow)
        snprintf(maxbuf, sizeof(maxbuf), ">%llu",
                 (unsigned long long)((double)HIST_BUCKETS *
                                      HIST_TICKS_PER_BUCKET * ns_per_tick));
    else
        snprintf(maxbuf, sizeof(maxbuf), "%llu",
                 (unsigned long long)((double)max_t * ns_per_tick));

    snprintf(meanbuf, sizeof(meanbuf), "%s%.1f",
             h->overflow ? ">=" : "", hist_mean(h) * ns_per_tick);

    printf("  %-8s %-10s %-10s %-10s %-10s %-10s\n",
           pbuf[0], pbuf[1], pbuf[2], pbuf[3], maxbuf, meanbuf);

    if (h->overflow)
        printf("  ⚠ 有 %llu 个样本超出量程，分位数偏乐观，请调大 HIST_TICKS_PER_BUCKET\n",
               (unsigned long long)h->overflow);
    if (h->count < 1000000)
        printf("  ⚠ 样本不足 100 万，p999/p9999 无统计意义\n");
}

#endif /* HIST_H */
