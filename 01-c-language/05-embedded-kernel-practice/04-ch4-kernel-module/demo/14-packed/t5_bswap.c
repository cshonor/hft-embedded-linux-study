/*
 * T5: htonl/ntohl 真实实现与汇编输出
 *
 * 对比三种字节序转换实现：
 * 1. __builtin_bswap32（编译器内置，编译成 1 条 BSWAP / REV 指令）
 * 2. 手动移位（4 次移位 + 3 次 OR）
 * 3. union 重解释（类型双关，可能 UB）
 *
 * 编译：gcc -O2 -Wall -Wno-unused -fno-pie -no-pie -o t5_bswap t5_bswap.c
 *       clang -O2 -Wall -Wno-unused -fno-pie -no-pie -o t5_bswap t5_bswap.c
 *
 * 查看汇编（验证 BSWAP 指令）：
 *   gcc -O2 -S -o - t5_bswap.c | grep -A2 bswap
 *   objdump -d t5_bswap | grep -A2 bswap
 */
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

/* ---- 方式 1: __builtin_bswap32（对应内核 __swab32 的有 __has_builtin 版本） ---- */
static inline uint32_t bswap_builtin(uint32_t x) {
    return __builtin_bswap32(x);
}

/* ---- 方式 2: 手动移位（对应内核 ___constant_swab32 的宏展开版） ---- */
static inline uint32_t bswap_manual(uint32_t x) {
    return ((x & 0x000000ff) << 24) |
           ((x & 0x0000ff00) << 8)  |
           ((x & 0x00ff0000) >> 8)  |
           ((x & 0xff000000) >> 24);
}

/* ---- 方式 3: union 重解释（不推荐，可能 UB） ---- */
static inline uint32_t bswap_union(uint32_t x) {
    union { uint32_t u; uint8_t b[4]; } s;
    s.u = x;
    uint8_t t = s.b[0]; s.b[0] = s.b[3]; s.b[3] = t;
    t = s.b[1]; s.b[1] = s.b[2]; s.b[2] = t;
    return s.u;
}

/* htonl/ntohl 的真实定义（小端机器） */
/* 内核: #define ntohl(x) __be32_to_cpu(x) -> __swab32(x) -> __builtin_bswap32 */
#define MY_HTONL(x) bswap_builtin(x)
#define MY_NTOHL(x) bswap_builtin(x)

#define N (50 * 1000 * 1000)
#define CACHELINE 64

int main(void)
{
    printf("=== htonl/ntohl 三种实现对比 ===\n\n");

    /* ---- 正确性验证 ---- */
    uint32_t test = 0x12345678;
    printf("输入: 0x%08x\n", test);
    printf("  __builtin_bswap32: 0x%08x\n", bswap_builtin(test));
    printf("  手动移位:          0x%08x\n", bswap_manual(test));
    printf("  union 重解释:      0x%08x\n", bswap_union(test));
    printf("  三者一致: %s\n\n",
           (bswap_builtin(test) == bswap_manual(test) &&
            bswap_builtin(test) == bswap_union(test)) ? "YES" : "NO (!)");

    /* ---- 模拟 htonl 使用场景：构造 TCP seq ---- */
    printf("=== htonl 使用场景模拟 ===\n");
    uint32_t host_seq = 0x80000000;
    uint32_t net_seq = MY_HTONL(host_seq);
    printf("  主机序 seq  = 0x%08x\n", host_seq);
    printf("  网络序 seq  = 0x%08x (htonl)\n", net_seq);
    printf("  还原(ntohl) = 0x%08x\n", MY_NTOHL(net_seq));
    printf("  -> 小端机器: htonl = bswap32; 大端机器: htonl = no-op\n\n");

    /* ---- 性能对比 ---- */
    printf("=== 性能对比 (%d 万次) ===\n", N / 10000);
    volatile uint32_t sink = 0;

    volatile uint32_t buf[256]; /* 固定 256 大小 */
    for (int i = 0; i < 256; i++) buf[i] = i;

    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        sink += bswap_builtin(buf[i & 255]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_b = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        sink += bswap_manual(buf[i & 255]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_m = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        sink += bswap_union(buf[i & 255]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_u = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    printf("  __builtin_bswap32: %8.2f ms (应编译成 1 条 BSWAP)\n", ms_b);
    printf("  手动移位:          %8.2f ms (4 移位 + 3 OR，编译器可能优化成 BSWAP)\n", ms_m);
    printf("  union 重解释:      %8.2f ms (内存字节交换)\n", ms_u);
    printf("\n  关键：-O2 下手动移位也会被编译器识别为 bswap 模式 -> 同一条指令\n");
    printf("  验证：gcc -O2 -S t5_bswap.c | grep bswap\n");

    printf("\nsink = %u (防优化)\n", (unsigned)sink);
    return 0;
}
