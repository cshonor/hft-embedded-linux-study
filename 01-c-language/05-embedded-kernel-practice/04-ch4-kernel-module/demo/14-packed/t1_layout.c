/*
 * T1: 协议结构体真实布局 sizeof/offsetof 对比
 *
 * 对照 v6.6 内核真实定义，打印每个结构体的 sizeof 和成员偏移，
 * 验证「TCP/IP 头不加 packed，USB/Ethernet 头加 packed」的原因。
 *
 * 编译：gcc -O0 -Wall -Wno-unused -fno-pie -no-pie -o t1_layout t1_layout.c
 *       clang -O0 -Wall -Wno-unused -fno-pie -no-pie -o t1_layout t1_layout.c
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

/* ---- 类型别名（模拟内核 __be16/__be32/__sum16/__u8/__u16/__u32） ---- */
typedef uint8_t  __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
/* __bitwise 是 sparse 标注，编译器层面无定义，这里用空宏 */
#ifndef __bitwise
#define __bitwise
#endif
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __be32;
typedef __u16 __bitwise __sum16;
typedef __u16 __bitwise __le16;
typedef __u32 __bitwise __le32;

#define __packed __attribute__((packed))

/* ---- 本机字节序探测（编译期，对应内核 __LITTLE_ENDIAN_BITFIELD） ---- */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define __LITTLE_ENDIAN_BITFIELD 1
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define __BIG_ENDIAN_BITFIELD 1
#else
  #error "unknown byte order"
#endif

/* ============================================================
 * TCP 头（v6.6 include/uapi/linux/tcp.h）—— 不加 packed
 * ============================================================ */
struct tcphdr {
    __be16 source;
    __be16 dest;
    __be32 seq;
    __be32 ack_seq;
#if defined(__LITTLE_ENDIAN_BITFIELD)
    __u16 res1:4,
          doff:4,
          fin:1,
          syn:1,
          rst:1,
          psh:1,
          ack:1,
          urg:1,
          ece:1,
          cwr:1;
#elif defined(__BIG_ENDIAN_BITFIELD)
    __u16 doff:4,
          res1:4,
          cwr:1,
          ece:1,
          urg:1,
          ack:1,
          psh:1,
          rst:1,
          syn:1,
          fin:1;
#endif
    __be16  window;
    __sum16 check;
    __be16  urg_ptr;
};

/* TCP union cast（热路径整字操作） */
union tcp_word_hdr {
    struct tcphdr hdr;
    __be32        words[5];
};
#define tcp_flag_word(tp) (((union tcp_word_hdr *)(tp))->words[3])

/* ============================================================
 * IP 头（v6.6 include/uapi/linux/ip.h）—— 不加 packed
 * ============================================================ */
struct iphdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    __u8 ihl:4,
         version:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
    __u8 version:4,
         ihl:4;
#endif
    __u8    tos;
    __be16  tot_len;
    __be16  id;
    __be16  frag_off;
    __u8    ttl;
    __u8    protocol;
    __sum16 check;
    /* __struct_group 简化为直接成员 */
    __be32  saddr;
    __be32  daddr;
};

/* ============================================================
 * UDP 头（v6.6 include/uapi/linux/udp.h）—— 不加 packed
 * ============================================================ */
struct udphdr {
    __be16  source;
    __be16  dest;
    __be16  len;
    __sum16 check;
};

/* ============================================================
 * Ethernet 头（v6.6 include/uapi/linux/if_ether.h）—— 加 packed
 * ============================================================ */
#define ETH_ALEN 6
struct ethhdr {
    unsigned char h_dest[ETH_ALEN];
    unsigned char h_source[ETH_ALEN];
    __be16        h_proto;
} __packed;

/* 对比：ethhdr 不加 packed 的版本 */
struct ethhdr_nopack {
    unsigned char h_dest[ETH_ALEN];
    unsigned char h_source[ETH_ALEN];
    __be16        h_proto;
};

/* ============================================================
 * USB 设备描述符（v6.6 include/uapi/linux/usb/ch9.h）—— 加 packed
 * ============================================================ */
struct usb_device_descriptor {
    __u8  bLength;
    __u8  bDescriptorType;
    __le16 bcdUSB;
    __u8  bDeviceClass;
    __u8  bDeviceSubClass;
    __u8  bDeviceProtocol;
    __u8  bMaxPacketSize0;
    __le16 idVendor;
    __le16 idProduct;
    __le16 bcdDevice;
    __u8  iManufacturer;
    __u8  iProduct;
    __u8  iSerialNumber;
    __u8  bNumConfigurations;
} __packed;

/* 对比：USB 描述符不加 packed 的版本 */
struct usb_device_descriptor_nopack {
    __u8  bLength;
    __u8  bDescriptorType;
    __le16 bcdUSB;
    __u8  bDeviceClass;
    __u8  bDeviceSubClass;
    __u8  bDeviceProtocol;
    __u8  bMaxPacketSize0;
    __le16 idVendor;
    __le16 idProduct;
    __le16 bcdDevice;
    __u8  iManufacturer;
    __u8  iProduct;
    __u8  iSerialNumber;
    __u8  bNumConfigurations;
};

/* ---- 真正需要 packed 的排列：__u8 后面直接跟 __le16 ----
 * 如果成员排列是 [__u8, __le16, __u8]，不加 packed 就会产生 padding
 * 这才是 packed 真正不可省的场景
 */
struct usb_bad_order_packed {
    __u8  bLength;
    __le16 wValue;   /* offset 1, 不对齐 2 -> packed 必须有 */
    __u8  bType;
} __packed;

struct usb_bad_order_nopack {
    __u8  bLength;
    __le16 wValue;
    __u8  bType;
};

/* ---- 打印宏 ---- */
#define P_FIELD(T, f) \
    printf("  %-20s off=%2zu  size=%zu\n", #f, offsetof(T, f), sizeof(((T*)0)->f))
#define P_STRUCT(T) \
    printf("%-32s sizeof=%zu  align=%zu\n", #T, sizeof(T), _Alignof(T))

int main(void)
{
#ifdef __LITTLE_ENDIAN_BITFIELD
    printf("=== 字节序: LITTLE ENDIAN (x86/ARM-LE) ===\n\n");
#else
    printf("=== 字节序: BIG ENDIAN ===\n\n");
#endif

    printf("========================================\n");
    printf(" TCP 头 struct tcphdr [不加 packed]\n");
    printf("========================================\n");
    P_STRUCT(struct tcphdr);
    P_FIELD(struct tcphdr, source);
    P_FIELD(struct tcphdr, dest);
    P_FIELD(struct tcphdr, seq);
    P_FIELD(struct tcphdr, ack_seq);
    P_FIELD(struct tcphdr, window);
    P_FIELD(struct tcphdr, check);
    P_FIELD(struct tcphdr, urg_ptr);
    printf("  -> 位域 doff/fin/syn... 共占 1 个 __u16（offset=12, 2B）\n");
    printf("  -> 共 5 个 32 位字 = 20 字节（最小 TCP 头）\n\n");

    printf("========================================\n");
    printf(" IP 头 struct iphdr [不加 packed]\n");
    printf("========================================\n");
    P_STRUCT(struct iphdr);
    P_FIELD(struct iphdr, tos);
    P_FIELD(struct iphdr, tot_len);
    P_FIELD(struct iphdr, id);
    P_FIELD(struct iphdr, frag_off);
    P_FIELD(struct iphdr, ttl);
    P_FIELD(struct iphdr, protocol);
    P_FIELD(struct iphdr, check);
    P_FIELD(struct iphdr, saddr);
    P_FIELD(struct iphdr, daddr);
    printf("  -> ihl+version 共占 1 个 __u8（offset=0, 1B）\n\n");

    printf("========================================\n");
    printf(" UDP 头 struct udphdr [不加 packed]\n");
    printf("========================================\n");
    P_STRUCT(struct udphdr);
    P_FIELD(struct udphdr, source);
    P_FIELD(struct udphdr, dest);
    P_FIELD(struct udphdr, len);
    P_FIELD(struct udphdr, check);
    printf("  -> 4 个 __be16/__sum16 = 8 字节\n\n");

    printf("========================================\n");
    printf(" Ethernet 头 struct ethhdr [加 packed]\n");
    printf("========================================\n");
    P_STRUCT(struct ethhdr);
    P_FIELD(struct ethhdr, h_dest);
    P_FIELD(struct ethhdr, h_source);
    P_FIELD(struct ethhdr, h_proto);
    printf("\n");
    printf("  对比不加 packed:\n");
    P_STRUCT(struct ethhdr_nopack);
    P_FIELD(struct ethhdr_nopack, h_proto);
    printf("  -> 实测: packed 和 nopack 的 sizeof/offset 相同！\n");
    printf("     差别只在 align: packed=%zu, nopack=%zu\n",
           _Alignof(struct ethhdr), _Alignof(struct ethhdr_nopack));
    printf("     packed 是防御性的: 保证所有编译器/排列下无 padding\n\n");

    printf("========================================\n");
    printf(" USB 描述符 struct usb_device_descriptor [加 packed]\n");
    printf("========================================\n");
    P_STRUCT(struct usb_device_descriptor);
    P_FIELD(struct usb_device_descriptor, bLength);
    P_FIELD(struct usb_device_descriptor, bDescriptorType);
    P_FIELD(struct usb_device_descriptor, bcdUSB);
    P_FIELD(struct usb_device_descriptor, idVendor);
    P_FIELD(struct usb_device_descriptor, idProduct);
    printf("\n");
    printf("  对比不加 packed:\n");
    P_STRUCT(struct usb_device_descriptor_nopack);
    P_FIELD(struct usb_device_descriptor_nopack, bcdUSB);
    P_FIELD(struct usb_device_descriptor_nopack, idVendor);
    printf("  -> 实测: packed 和 nopack 的 sizeof/offset 也相同！\n");
    printf("     差别只在 align: packed=%zu, nopack=%zu\n\n",
           _Alignof(struct usb_device_descriptor),
           _Alignof(struct usb_device_descriptor_nopack));

    printf("\n");
    printf("========================================\n");
    printf(" 真正需要 packed 的排列 [u8, le16, u8]\n");
    printf("========================================\n");
    P_STRUCT(struct usb_bad_order_packed);
    P_FIELD(struct usb_bad_order_packed, bLength);
    P_FIELD(struct usb_bad_order_packed, wValue);
    P_FIELD(struct usb_bad_order_packed, bType);
    printf("\n  对比不加 packed:\n");
    P_STRUCT(struct usb_bad_order_nopack);
    P_FIELD(struct usb_bad_order_nopack, bLength);
    P_FIELD(struct usb_bad_order_nopack, wValue);
    P_FIELD(struct usb_bad_order_nopack, bType);
    printf("  -> packed:  wValue offset=1, sizeof=4 (紧凑，wValue 未对齐!)\n");
    printf("  -> nopack: wValue offset=2, sizeof=6 (有 1B+1B padding!)\n");
    printf("  -> 这才是 packed 不可省的场景: 访问 wValue 需要 byte-by-byte\n\n");

    printf("========================================\n");
    printf(" 结论\n");
    printf("========================================\n");
    printf("  TCP/IP/UDP 头: 成员全 2/4 字节自然对齐 -> 无 padding -> packed 多余\n");
    printf("  Ethernet 头:   char[6]+char[6]+__be16 -> 恰好无 padding, packed 防御性\n");
    printf("  USB 描述符:    __u8+__le16 混合 -> 恰好无 padding, packed 防御性\n");
    printf("  bad_order:     __u8+__le16 紧邻 -> 有 padding -> packed 必须!\n");
    printf("\n  结论: packed 的真正作用 = 消除 padding + 降 align 到 1\n");
    printf("        好排列时 packed 是防御性的, 坏排列时 packed 是必须的\n");

    return 0;
}
