/*
 * T3: packed 结构体未对齐访问性能代价
 *
 * packed 结构体的成员可能不在自然对齐地址上，
 * 访问需要多条指令（x86 硬件支持未对齐但有性能代价）。
 * 实测 packed vs 非 packed 循环访问的耗时和生成指令差异。
 *
 * 编译：gcc -O2 -Wall -Wno-unused -fno-pie -no-pie -o t3_unaligned t3_unaligned.c
 *       clang -O2 -Wall -Wno-unused -fno-pie -no-pie -o t3_unaligned t3_unaligned.c
 *   注意：用 -O2 看真实热路径行为（-O0 数据被编译器保守化不真实）
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define __packed __attribute__((packed))

/* 模拟 USB 设备描述符——packed，有未对齐的 __le16 成员 */
struct usb_desc_packed {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;        /* offset 2, 对齐 2 (packed 后) */
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;      /* offset 8 */
    uint16_t idProduct;
} __packed;

/* 对比：手动 padding 的对齐版本 */
struct usb_desc_aligned {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
};

/* 极端：故意制造未对齐的 uint32_t */
struct bad_packed {
    uint8_t  head;
    uint32_t val;           /* offset 1, 未对齐 4 字节 */
} __packed;

struct good_aligned {
    uint8_t  head;
    uint8_t  pad[3];
    uint32_t val;           /* offset 4, 对齐 4 字节 */
};

#define N (10 * 1000 * 1000)  /* 1000 万次 */
#define CACHELINE 64

int main(void)
{
    printf("=== packed vs aligned 未对齐访问性能对比 ===\n\n");

    /* 用 volatile 防止编译器把整个循环优化掉 */
    volatile struct usb_desc_packed  pk[2] __attribute__((aligned(CACHELINE)));
    volatile struct usb_desc_aligned al[2] __attribute__((aligned(CACHELINE)));
    volatile struct bad_packed       bp[2] __attribute__((aligned(CACHELINE)));
    volatile struct good_aligned     ga[2] __attribute__((aligned(CACHELINE)));

    volatile uint32_t sink = 0;

    /* ---- USB 描述符 packed vs aligned ---- */
    printf("USB 描述符 packed (sizeof=%zu) vs aligned (sizeof=%zu):\n",
           sizeof(struct usb_desc_packed), sizeof(struct usb_desc_aligned));

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        int idx = i & 1;
        pk[idx].idVendor = (uint16_t)(i & 0xFFFF);
        pk[idx].idProduct = (uint16_t)(i & 0xFFFF);
        sink += pk[idx].idVendor;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_pk = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        int idx = i & 1;
        al[idx].idVendor = (uint16_t)(i & 0xFFFF);
        al[idx].idProduct = (uint16_t)(i & 0xFFFF);
        sink += al[idx].idVendor;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_al = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    printf("  packed:  %8.2f ms\n", ms_pk);
    printf("  aligned: %8.2f ms\n", ms_al);
    printf("  ratio:   %8.2fx\n\n", ms_pk / ms_al);

    /* ---- 极端：uint32_t 未对齐 ---- */
    printf("uint32_t 未对齐 (offset=1) vs 对齐 (offset=4):\n");
    printf("  bad_packed  sizeof=%zu val_off=%zu\n",
           sizeof(struct bad_packed), (size_t)((char*)&bp[0].val - (char*)&bp[0]));
    printf("  good_aligned sizeof=%zu val_off=%zu\n",
           sizeof(struct good_aligned), (size_t)((char*)&ga[0].val - (char*)&ga[0]));

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        int idx = i & 1;
        bp[idx].val = (uint32_t)i;
        sink += bp[idx].val;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_bp = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        int idx = i & 1;
        ga[idx].val = (uint32_t)i;
        sink += ga[idx].val;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms_ga = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    printf("  unaligned: %8.2f ms\n", ms_bp);
    printf("  aligned:   %8.2f ms\n", ms_ga);
    printf("  ratio:     %8.2fx\n", ms_bp / ms_ga);
    printf("  (x86 有硬件未对齐支持，代价较小；ARM/SPARC 可能触发异常)\n\n");

    printf("sink = %u (防优化)\n", (unsigned)sink);
    return 0;
}
