/* tcpip-ill-vol1: minimal IPv4/UDP/TCP header layout dump (study aid, not Stevens book source).
 * Build: gcc -Wall -o hdr_dump hdr_dump.c
 * Run:   ./hdr_dump
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#pragma pack(push, 1)
struct ipv4_hdr {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
};
struct udp_hdr {
    uint16_t sport;
    uint16_t dport;
    uint16_t len;
    uint16_t check;
};
struct tcp_hdr {
    uint16_t sport;
    uint16_t dport;
    uint32_t seq;
    uint32_t ack_seq;
    uint8_t  off_res;
    uint8_t  flags;
    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
};
#pragma pack(pop)

static void dump(const char *name, const void *p, size_t n) {
    const unsigned char *b = p;
    printf("%s (%zu bytes):", name, n);
    for (size_t i = 0; i < n; i++) printf(" %02x", b[i]);
    printf("\n");
}

int main(void) {
    struct ipv4_hdr ip;
    struct udp_hdr udp;
    struct tcp_hdr tcp;
    memset(&ip, 0, sizeof ip);
    memset(&udp, 0, sizeof udp);
    memset(&tcp, 0, sizeof tcp);
    ip.ver_ihl = 0x45;
    ip.ttl = 64;
    ip.protocol = 17; /* UDP */
    ip.tot_len = 20 + 8;
    udp.sport = 1234;
    udp.dport = 53;
    udp.len = 8;
    tcp.off_res = 5 << 4;
    tcp.flags = 0x02; /* SYN */
    printf("sizeof ipv4=%zu udp=%zu tcp=%zu\n", sizeof ip, sizeof udp, sizeof tcp);
    dump("IPv4", &ip, sizeof ip);
    dump("UDP", &udp, sizeof udp);
    dump("TCP", &tcp, sizeof tcp);
    return 0;
}
