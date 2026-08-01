#include <stdio.h>
#include <stdint.h>

struct normal {
    uint8_t  type;
    uint32_t value;
};

struct packed {
    uint8_t  type;
    uint32_t value;
} __attribute__((packed));

struct aligned16 {
    uint8_t  type;
    uint32_t value;
} __attribute__((aligned(16)));

/* 以太网帧头风格：协议字段必须紧凑 */
struct eth_hdr {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t ethertype;
} __attribute__((packed));

int main(void)
{
    struct normal   n = { .type = 1, .value = 0x12345678 };
    struct packed   p = { .type = 1, .value = 0x12345678 };
    struct aligned16 a = { .type = 1, .value = 0x12345678 };
    struct eth_hdr  hdr = { .ethertype = 0x0800 };

    printf("sizeof normal   = %zu\n", sizeof(struct normal));
    printf("sizeof packed   = %zu\n", sizeof(struct packed));
    printf("sizeof aligned16= %zu\n", sizeof(struct aligned16));
    printf("sizeof eth_hdr  = %zu\n", sizeof(struct eth_hdr));
    printf("alignof aligned16 = %zu\n", _Alignof(struct aligned16));

    printf("n @%p  p @%p  a @%p\n", (void *)&n, (void *)&p, (void *)&a);
    printf("gdb: x/16xb &p   # 观察无 padding 的 5 字节布局\n");

    (void)hdr;
    return 0;
}
