/* xdp-proto-filter.bpf.c — XDP 边界检查范式 + 按协议计数（ch08 §8.1 块1/2 补全）
 *
 * verifier 的铁律：每次解引用前必须先证明 [ptr, ptr+size) ⊆ [data, data_end)。
 * 少一层检查 = 加载被拒（见 ch06）。
 *
 * 编译: clang -O2 -g -target bpf -D__TARGET_ARCH_x86 \
 *        -I/usr/include/x86_64-linux-gnu -c xdp-proto-filter.bpf.c -o xdp-proto-filter.o
 * 加载（需 root）: sudo ip link set dev <nic> xdpgeneric obj xdp-proto-filter.o sec xdp
 * 观察: sudo bpftool map dump name proto_cnt
 */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

/* 按协议号计数：key=IP protocol（6=TCP 17=UDP 1=ICMP），value=包数 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 256);
    __type(key, __u8);
    __type(value, __u64);
} proto_cnt SEC(".maps");

/* 块2 原样：两层边界检查的协议识别 */
static __always_inline unsigned char lookup_protocol(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    struct ethhdr *eth = data;
    /* 第一层：先确认以太网头完整落在包内，才能读 eth->h_proto */
    if (data + sizeof(struct ethhdr) > data_end)
        return 0;
    if (bpf_ntohs(eth->h_proto) == ETH_P_IP) {
        /* 第二层：iphdr 紧跟以太网头，同样先检查边界 */
        struct iphdr *iph = data + sizeof(struct ethhdr);
        if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) <= data_end)
            return iph->protocol;   /* 6=TCP, 17=UDP */
    }
    return 0;
}

SEC("xdp")
int count_protocol(struct xdp_md *ctx) {
    unsigned char proto = lookup_protocol(ctx);
    if (proto != 0) {
        __u64 *cnt = bpf_map_lookup_elem(&proto_cnt, &proto);
        if (cnt)
            __sync_fetch_and_add(cnt, 1);
    }
    return XDP_PASS;   /* 只统计，不拦截（改 XDP_DROP 即防火墙雏形） */
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
