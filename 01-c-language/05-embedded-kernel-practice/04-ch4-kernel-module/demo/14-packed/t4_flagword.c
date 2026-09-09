/*
 * T4: tcp_flag_word union 整字操作 vs 位域操作
 *
 * 内核热路径用 union tcp_word_hdr + tcp_flag_word() + TCP_FLAG_* 整字常量
 * 一次性 AND/OR 设置/清除多个 flag（一条指令搞定），而非逐个访问位域。
 * 实测两种写法的性能差异和代码简洁度。
 *
 * 编译：gcc -O2 -Wall -Wno-unused -fno-pie -no-pie -o t4_flagword t4_flagword.c
 *       clang -O2 -Wall -Wno-unused -fno-pie -no-pie -o t4_flagword t4_flagword.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

typedef uint8_t  __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef __u16 __be16;
typedef __u32 __be32;
typedef __u16 __sum16;

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define __LITTLE_ENDIAN_BITFIELD 1
  #define ENDIAN "LITTLE"
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define __BIG_ENDIAN_BITFIELD 1
  #define ENDIAN "BIG"
#else
  #error "unknown byte order"
#endif

/* TCP 头（同 T1） */
struct tcphdr {
    __be16 source;
    __be16 dest;
    __be32 seq;
    __be32 ack_seq;
#if defined(__LITTLE_ENDIAN_BITFIELD)
    __u16 res1:4, doff:4,
          fin:1, syn:1, rst:1, psh:1,
          ack:1, urg:1, ece:1, cwr:1;
#elif defined(__BIG_ENDIAN_BITFIELD)
    __u16 doff:4, res1:4,
          cwr:1, ece:1, urg:1, ack:1,
          psh:1, rst:1, syn:1, fin:1;
#endif
    __be16  window;
    __sum16 check;
    __be16  urg_ptr;
};

/* union cast（对应内核 union tcp_word_hdr） */
union tcp_word_hdr {
    struct tcphdr hdr;
    __be32        words[5];
};
#define tcp_flag_word(tp) (((union tcp_word_hdr *)(tp))->words[3])

/* TCP_FLAG_* 常量（对应内核 v6.6 tcp.h 的定义）
 * 注意：这些是 __constant_cpu_to_be32，值依赖本机字节序
 * 小端机器: SYN=0x00020000, ACK=0x00100000 ...
 * 大端机器: SYN=0x00020000, ACK=0x00100000 ... (网络序)
 */
/* 小端机器上 __constant_cpu_to_be32(x) = bswap32(x) */
static inline __be32 be32_const(uint32_t x) {
    /* 模拟小端机器 __constant_cpu_to_be32 */
#if defined(__LITTLE_ENDIAN_BITFIELD)
    return __builtin_bswap32(x);
#else
    return x;
#endif
}
#define TCP_FLAG_CWR be32_const(0x00800000)
#define TCP_FLAG_ECE be32_const(0x00400000)
#define TCP_FLAG_URG be32_const(0x00200000)
#define TCP_FLAG_ACK be32_const(0x00100000)
#define TCP_FLAG_PSH be32_const(0x00080000)
#define TCP_FLAG_RST be32_const(0x00040000)
#define TCP_FLAG_SYN be32_const(0x00020000)
#define TCP_FLAG_FIN be32_const(0x00010000)

#define N (50 * 1000 * 1000)  /* 5000 万次 */
#define CACHELINE 64

int main(void)
{
    printf("=== tcp_flag_word 整字操作 vs 位域操作 (%s ENDIAN) ===\n\n", ENDIAN);

    /* 去掉 volatile，用 asm barrier 防止整个循环被优化掉 */
    /* 这样编译器能把位域/整字操作都优化到寄存器 */
    struct tcphdr th_a __attribute__((aligned(CACHELINE)));
    uint32_t sink = 0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        th_a = (struct tcphdr){0};
        th_a.doff = 5;
        th_a.syn = 1;
        th_a.ack = 1;
        th_a.window = 65535;
        sink += th_a.window;
    }
    asm volatile("" : "+r"(sink));
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_bit = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    /* ---- 方式 B：整字操作（热路径写法） ---- */
    struct tcphdr th_b __attribute__((aligned(CACHELINE)));

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        th_b = (struct tcphdr){0};
        th_b.doff = 5;
        th_b.window = 65535;
        /* 整字 OR 设置 SYN+ACK（一条指令设两个 flag） */
        tcp_flag_word(&th_b) |= (TCP_FLAG_SYN | TCP_FLAG_ACK);
        sink += th_b.window;
    }
    asm volatile("" : "+r"(sink));
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_word = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    printf("位域逐个设置 (doff=5, syn=1, ack=1, window=65535):\n");
    printf("  %8.2f ms (%d 万次)\n", ms_bit, N / 10000);
    printf("整字 OR 设置 (TCP_FLAG_SYN | TCP_FLAG_ACK):\n");
    printf("  %8.2f ms (%d 万次)\n", ms_word, N / 10000);
    if (ms_word > 0)
        printf("ratio: %8.2fx (位域/整字)\n\n", ms_bit / ms_word);
    else
        printf("\n");
    printf("注: -O2 -fno-strict-aliasing 下编译器能优化 union cast\n");
    printf("    整字 OR 的优势在一条指令设/清多个 flag，而非循环整体\n\n");

    /* ---- 验证结果一致 ---- */
    printf("=== 验证两种方式结果一致 ===\n");
    struct tcphdr ta, tb;
    memset(&ta, 0, sizeof(ta));
    memset(&tb, 0, sizeof(tb));
    /* A: 位域 */
    ta.doff = 5; ta.syn = 1; ta.ack = 1;
    /* B: 整字 */
    tb.doff = 5;
    tcp_flag_word(&tb) |= (TCP_FLAG_SYN | TCP_FLAG_ACK);
    printf("  A (位域):   syn=%u ack=%u fin=%u rst=%u\n",
           ta.syn, ta.ack, ta.fin, ta.rst);
    printf("  B (整字OR): syn=%u ack=%u fin=%u rst=%u\n",
           tb.syn, tb.ack, tb.fin, tb.rst);
    printf("  一致: %s\n\n",
           (ta.syn == tb.syn && ta.ack == tb.ack &&
            ta.fin == tb.fin && ta.rst == tb.rst) ? "YES" : "NO (!)");

    /* ---- 整字清除多个 flag ---- */
    printf("=== 整字清除多个 flag（热路径卸载） ===\n");
    struct tcphdr tc;
    memset(&tc, 0, sizeof(tc));
    tc.doff = 8; tc.syn = 1; tc.ack = 1; tc.psh = 1; tc.urg = 1;
    printf("  清除前: syn=%u ack=%u psh=%u urg=%u\n", tc.syn, tc.ack, tc.psh, tc.urg);
    /* 一次性清除 SYN|ACK|PSH|URG */
    tcp_flag_word(&tc) &= ~(TCP_FLAG_SYN | TCP_FLAG_ACK | TCP_FLAG_PSH | TCP_FLAG_URG);
    printf("  清除后: syn=%u ack=%u psh=%u urg=%u\n", tc.syn, tc.ack, tc.psh, tc.urg);
    printf("  -> 一条 AND 指令清除 4 个 flag（位域要 4 次写）\n\n");

    printf("sink = %u (防优化)\n", (unsigned)sink);
    return 0;
}
