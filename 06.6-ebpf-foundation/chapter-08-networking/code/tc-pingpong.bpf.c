/* tc-pingpong.bpf.c — TC 版 ping 应答器（ch08 §8.1 块4 补全）
 * 出自《Learning eBPF》第 8 章 pingpong 例子
 *
 * 原理：ICMP request 进来 → 交换 MAC/IP、type 8→0 → clone_redirect 从原口发回
 * 效果：给网卡 IP 发 ping，回包是 BPF 程序"伪造"的（内核协议栈根本没看到）
 *
 * 编译: clang -O2 -g -target bpf -D__TARGET_ARCH_x86 \
 *        -I/usr/include/x86_64-linux-gnu -c tc-pingpong.bpf.c -o tc-pingpong.o
 * 加载（需 root）:
 *   sudo tc qdisc add dev <nic> clsact
 *   sudo tc filter add dev <nic> ingress bpf da obj tc-pingpong.o sec classifier
 *   ping <nic 的 IP>
 * 卸载: sudo tc qdisc del dev <nic> clsact
 */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

/* linux/icmp.h 在 -target bpf 下会拉入 glibc 桩头（stubs-32.h 缺失）；
 * icmphdr 很小，自带定义更干净（字段布局与 uapi 一致） */
#define IPPROTO_ICMP    1
#define ICMP_ECHO       8
#define ICMP_ECHOREPLY  0
struct icmphdr {
    __u8  type;
    __u8  code;
    __u16 checksum;
    __u16 id;
    __u16 sequence;
};

#define TC_ACT_OK   0
#define TC_ACT_SHOT 2

static __always_inline void swap(__u8 *a, __u8 *b, int len) {
#pragma unroll
    for (int i = 0; i < len; i++) {
        __u8 t = a[i]; a[i] = b[i]; b[i] = t;
    }
}

SEC("classifier")
int pingpong(struct __sk_buff *skb) {
    void *data = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;

    /* 边界检查范式同 XDP（__sk_buff 也有 data/data_end） */
    struct ethhdr *eth = data;
    if (data + sizeof(*eth) > data_end)
        return TC_ACT_SHOT;
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return TC_ACT_OK;

    struct iphdr *iph = data + sizeof(*eth);
    if (data + sizeof(*eth) + sizeof(*iph) > data_end)
        return TC_ACT_SHOT;
    if (iph->protocol != IPPROTO_ICMP)
        return TC_ACT_OK;

    struct icmphdr *icmp = (void *)iph + sizeof(*iph);
    if ((void *)icmp + sizeof(*icmp) > data_end)
        return TC_ACT_SHOT;
    if (icmp->type != ICMP_ECHO)          /* 只接 request(type=8) */
        return TC_ACT_OK;

    /* 1. 交换 MAC */
    swap(eth->h_dest, eth->h_source, ETH_ALEN);
    /* 2. 交换 IP（结构体赋值 = 一次 32bit 拷贝） */
    __u32 tmp = iph->saddr;
    iph->saddr = iph->daddr;
    iph->daddr = tmp;
    /* 3. ICMP type: 8(request) → 0(response)，helper 增量修校验和 */
    bpf_skb_change_type(skb, ICMP_ECHOREPLY);

    /* 4. 克隆一份从进入的接口发回（0=ingress 方向），本体丢弃 */
    bpf_clone_redirect(skb, skb->ifindex, 0);
    return TC_ACT_SHOT;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
