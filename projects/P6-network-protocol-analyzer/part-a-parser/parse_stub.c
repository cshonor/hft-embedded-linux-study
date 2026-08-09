#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Minimal Ethernet+IPv4 header peek (no live capture yet). */
struct eth_hdr {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t ethertype;
} __attribute__((packed));

int main(void)
{
    uint8_t frame[64];
    memset(frame, 0, sizeof frame);
    frame[12] = 0x08;
    frame[13] = 0x00; /* IPv4 */

    const struct eth_hdr *eth = (const struct eth_hdr *)frame;
    uint16_t type = (uint16_t)((eth->ethertype >> 8) | (eth->ethertype << 8));
    printf("ethertype=0x%04x (expect 0x0800)\n", type);
    return type == 0x0800 ? 0 : 1;
}
