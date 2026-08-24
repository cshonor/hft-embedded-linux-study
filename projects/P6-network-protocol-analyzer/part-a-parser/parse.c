#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * 不抓网卡：合成一帧 Ethernet+IPv4+TCP SYN，解析字段，写出最小 pcap。
 * AF_PACKET 需要 root，先把字节序和 ihl/doff 算对。
 */

struct eth_hdr {
    uint8_t dst[6];
    uint8_t src[6];
    uint8_t etype[2];
} __attribute__((packed));

struct ipv4_hdr {
    uint8_t ver_ihl;
    uint8_t tos;
    uint8_t tot_len[2];
    uint8_t id[2];
    uint8_t frag[2];
    uint8_t ttl;
    uint8_t proto;
    uint8_t check[2];
    uint8_t saddr[4];
    uint8_t daddr[4];
} __attribute__((packed));

struct tcp_hdr {
    uint8_t sport[2];
    uint8_t dport[2];
    uint8_t seq[4];
    uint8_t ack[4];
    uint8_t off_res;
    uint8_t flags;
    uint8_t win[2];
    uint8_t check[2];
    uint8_t urg[2];
} __attribute__((packed));

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

struct pcap_file_hdr {
    uint32_t magic;
    uint16_t vmaj, vmin;
    int32_t zone;
    uint32_t sigfigs, snaplen, linktype;
};

struct pcap_pkt_hdr {
    uint32_t ts_sec, ts_usec, incl_len, orig_len;
};

int main(void)
{
    uint8_t frame[sizeof(struct eth_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct tcp_hdr)];
    memset(frame, 0, sizeof frame);

    struct eth_hdr *eth = (struct eth_hdr *)frame;
    eth->etype[0] = 0x08;
    eth->etype[1] = 0x00;

    struct ipv4_hdr *ip = (struct ipv4_hdr *)(frame + sizeof *eth);
    ip->ver_ihl = 0x45;
    ip->ttl = 64;
    ip->proto = 6;
    ip->saddr[0] = 10;
    ip->saddr[3] = 1;
    ip->daddr[0] = 10;
    ip->daddr[3] = 2;

    struct tcp_hdr *tcp = (struct tcp_hdr *)(frame + sizeof *eth + sizeof *ip);
    tcp->sport[0] = 0x30;
    tcp->sport[1] = 0x39; /* 12345 */
    tcp->dport[1] = 80;
    tcp->off_res = 0x50;
    tcp->flags = 0x02; /* SYN */

    if (be16(eth->etype) != 0x0800) {
        fprintf(stderr, "ethertype\n");
        return 1;
    }
    int ihl = (ip->ver_ihl & 0xf) * 4;
    if (ihl != 20 || ip->proto != 6) {
        fprintf(stderr, "ip\n");
        return 1;
    }
    uint16_t sp = be16(tcp->sport);
    uint16_t dp = be16(tcp->dport);
    if (sp != 12345 || dp != 80 || (tcp->flags & 0x02) == 0) {
        fprintf(stderr, "tcp %u->%u\n", sp, dp);
        return 1;
    }

    FILE *fp = fopen("/tmp/p6_syn.pcap", "wb");
    if (!fp)
        return 1;
    struct pcap_file_hdr fh = {0xa1b2c3d4, 2, 4, 0, 0, 65535, 1};
    struct pcap_pkt_hdr ph = {0, 0, (uint32_t)sizeof frame, (uint32_t)sizeof frame};
    fwrite(&fh, sizeof fh, 1, fp);
    fwrite(&ph, sizeof ph, 1, fp);
    fwrite(frame, sizeof frame, 1, fp);
    fclose(fp);

    printf("ETH IPv4 TCP %u -> %u SYN  pcap=/tmp/p6_syn.pcap\n", sp, dp);
    return 0;
}
