/*
 * T2: 位域字节序实测
 *
 * 用 union 把位域字段和原始字节关联，打印内存字节，
 * 对比位域访问 vs 手动移位，验证内核为什么要写位域双版本。
 *
 * 编译：gcc -O0 -Wall -Wno-unused -fno-pie -no-pie -o t2_bitfield t2_bitfield.c
 *       clang -O0 -Wall -Wno-unused -fno-pie -no-pie -o t2_bitfield t2_bitfield.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t  __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define __LITTLE_ENDIAN_BITFIELD 1
  #define ENDIAN_NAME "LITTLE ENDIAN"
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define __BIG_ENDIAN_BITFIELD 1
  #define ENDIAN_NAME "BIG ENDIAN"
#else
  #error "unknown byte order"
#endif

/* ---- IP 头第一个字节的位域（对应内核 iphdr 的 ihl:4, version:4） ---- */
union ip_first_byte {
    __u8 raw;
#if defined(__LITTLE_ENDIAN_BITFIELD)
    struct { __u8 ihl:4, version:4; } le;
#elif defined(__BIG_ENDIAN_BITFIELD)
    struct { __u8 version:4, ihl:4; } be;
#endif
};

/* ---- TCP doff+flags 的 16 位位域（对应内核 tcphdr 第 13-14 字节） ---- */
union tcp_flags_word {
    __u16 raw;
    __u8  bytes[2];
#if defined(__LITTLE_ENDIAN_BITFIELD)
    struct {
        __u16 res1:4, doff:4;
        __u16 fin:1, syn:1, rst:1, psh:1;
        __u16 ack:1, urg:1, ece:1, cwr:1;
    } le;
#elif defined(__BIG_ENDIAN_BITFIELD)
    struct {
        __u16 doff:4, res1:4;
        __u16 cwr:1, ece:1, urg:1, ack:1;
        __u16 psh:1, rst:1, syn:1, fin:1;
    } be;
#endif
};

/* ---- 手动移位版（不依赖位域，跨平台一致） ---- */
/* IP: 低 4 位 = ihl，高 4 位 = version（这是线上格式的大端定义） */
static inline __u8 ip_get_ihl_manual(__u8 raw)    { return raw & 0x0F; }
static inline __u8 ip_get_version_manual(__u8 raw) { return (raw >> 4) & 0x0F; }
static inline __u8 ip_make_byte_manual(__u8 ihl, __u8 ver) {
    return (ver << 4) | (ihl & 0x0F);
}

/* TCP: doff 在高 4 位（byte[0] 的高 nibble），flags 在 byte[1] */
/* 注意：这是大端线上格式，但在小端机器上内存布局不同 */
static inline __u8 tcp_get_doff_manual(const __u8 *p) {
    /* doff 在 byte[0] 的高 4 位 */
    return (p[0] >> 4) & 0x0F;
}
static inline __u8 tcp_get_flags_manual(const __u8 *p) {
    /* flags 在 byte[1] */
    return p[1];
}

static void print_bytes(const char *label, const void *p, size_t n)
{
    const unsigned char *b = p;
    printf("  %s: ", label);
    for (size_t i = 0; i < n; i++)
        printf("%02x ", b[i]);
    printf("\n");
}

int main(void)
{
    printf("=== 字节序: %s ===\n\n", ENDIAN_NAME);

    /* ---- IP 第一字节：ihl=5, version=4 ---- */
    printf("========================================\n");
    printf(" IP ihl+version 位域 vs 手动移位\n");
    printf("========================================\n");

    union ip_first_byte ipb;
    /* 手动构造线上字节：version=4 在高 4 位, ihl=5 在低 4 位 -> 0x45 */
    ipb.raw = ip_make_byte_manual(5, 4);
    printf("  目标: version=4, ihl=5 -> 线上字节 0x45\n");
    print_bytes("raw byte", &ipb.raw, 1);

#if defined(__LITTLE_ENDIAN_BITFIELD)
    printf("  位域读: ihl=%u version=%u (LE 版本)\n", ipb.le.ihl, ipb.le.version);
#elif defined(__BIG_ENDIAN_BITFIELD)
    printf("  位域读: ihl=%u version=%u (BE 版本)\n", ipb.be.ihl, ipb.be.version);
#endif
    printf("  手动读: ihl=%u version=%u\n",
           ip_get_ihl_manual(ipb.raw), ip_get_version_manual(ipb.raw));

    /* 反向：用位域设置值 */
    union ip_first_byte ipb2;
    memset(&ipb2, 0, sizeof(ipb2));
#if defined(__LITTLE_ENDIAN_BITFIELD)
    ipb2.le.ihl = 5;
    ipb2.le.version = 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
    ipb2.be.ihl = 5;
    ipb2.be.version = 4;
#endif
    printf("  位域写: ihl=5, version=4 -> ");
    print_bytes("生成的字节", &ipb2.raw, 1);
    printf("  手动写: ihl=5, version=4 -> 0x%02x\n", ip_make_byte_manual(5, 4));
    printf("  -> 两者一致: %s\n\n",
           ipb2.raw == ip_make_byte_manual(5, 4) ? "YES" : "NO (!)");

    /* ---- TCP doff+flags ---- */
    printf("========================================\n");
    printf(" TCP doff+flags 位域 vs 手动移位\n");
    printf("========================================\n");

    union tcp_flags_word tfw;
    memset(&tfw, 0, sizeof(tfw));
    /* 设置: doff=8(32B头), syn=1, ack=1 */
#if defined(__LITTLE_ENDIAN_BITFIELD)
    tfw.le.doff = 8;
    tfw.le.syn = 1;
    tfw.le.ack = 1;
#elif defined(__BIG_ENDIAN_BITFIELD)
    tfw.be.doff = 8;
    tfw.be.syn = 1;
    tfw.be.ack = 1;
#endif
    print_bytes("raw 2B", &tfw, 2);
    printf("  raw = 0x%04x\n", tfw.raw);

#if defined(__LITTLE_ENDIAN_BITFIELD)
    printf("  位域读(LE): doff=%u fin=%u syn=%u rst=%u psh=%u ack=%u\n",
           tfw.le.doff, tfw.le.fin, tfw.le.syn,
           tfw.le.rst, tfw.le.psh, tfw.le.ack);
#elif defined(__BIG_ENDIAN_BITFIELD)
    printf("  位域读(BE): doff=%u fin=%u syn=%u rst=%u psh=%u ack=%u\n",
           tfw.be.doff, tfw.be.fin, tfw.be.syn,
           tfw.be.rst, tfw.be.psh, tfw.be.ack);
#endif
    printf("  手动读: doff=%u flags=0x%02x\n",
           tcp_get_doff_manual(tfw.bytes),
           tcp_get_flags_manual(tfw.bytes));

    printf("\n  关键：大小端机器上同一位域代码生成的 raw 字节不同！\n");
    printf("  内核用 #if __LITTLE_ENDIAN_BITFIELD / __BIG_ENDIAN_BITFIELD 双版本\n");
    printf("  保证「无论本机什么字节序，生成的线上字节都符合 RFC」。\n\n");

    /* ---- 位域跨字节边界陷阱 ---- */
    printf("========================================\n");
    printf(" 位域跨存储单元边界陷阱\n");
    printf("========================================\n");
    /* C 标准不保证位域跨存储单元边界的行为 */
    /* 内核的 tcphdr 位域在 1 个 __u16（16 位）内，不跨边界 */
    /* 如果有人写 res1:4, doff:4, bigfield:9 跨 16 位边界 -> 不可移植 */
    printf("  内核 tcphdr 位域: res1:4 + doff:4 + 8x1bit = 16 bit = 1 个 __u16\n");
    printf("  -> 不跨存储单元边界，安全\n");
    printf("  -> 如果写 res1:4 + doff:4 + bigfield:9 = 17 bit 跨 __u16 边界\n");
    printf("     -> C 标准未定义，gcc/clang 行为不一致 -> 不可移植\n");

    return 0;
}
