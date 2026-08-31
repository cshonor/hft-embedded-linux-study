/* SPDX-License-Identifier: BSD-3-Clause
 *
 * mcast-minimal —— DPDK UDP 组播收包最小原型（HFT 行情接入骨架）
 *
 * 对应笔记：
 *   ../notes/chapter-05-组播行情接入.md        旁路下的组播接入
 *   ../notes/chapter-02-mbuf与内存池.md        mbuf / mempool
 *   ../notes/chapter-03-PMD与轮询模式.md       rte_eth_rx_burst 轮询语义
 *   12.5/chapter-02/notes/05-multicast-rx-path.md   内核栈组播路径（对照）
 *   12.5/chapter-15/notes/03-latency-measurement.md 分位统计方法论
 *
 * 编译：make（需要 DPDK 22.11 LTS / 23.11 LTS，pkg-config libdpdk）
 * 运行：sudo ./mcast_minimal -l 2 -n 4 -- -g 224.1.2.3 -p 12345
 *       （-- 之前是 EAL 参数，之后是本程序参数）
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>      /* getopt / optarg（POSIX 归属，getopt.h 未必声明） */
#include <netinet/in.h>  /* IPPROTO_UDP */
#include <arpa/inet.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_byteorder.h>  /* rte_be_to_cpu_16 / rte_cpu_to_be_16 */
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_cycles.h>
#include <rte_lcore.h>

#include "hist.h"

#define RX_RING_SIZE     4096
#define TX_RING_SIZE     4096
#define NUM_MBUFS        8191
#define MBUF_CACHE_SIZE  250
#define BURST_SIZE       32

/* 统计打印间隔：约 1 秒（用 TSC 折算，避免热路径调用 gettimeofday） */
#define STATS_INTERVAL_US 1000000

static volatile sig_atomic_t force_quit;

static struct hist hist_parse;   /* 每包解析耗时（纯解析代码开销） */
static struct hist hist_burst;   /* burst 内位置延迟（含队头等待） */
static struct hist hist_oh;      /* rdtsc 基线开销：报告时要从上面两项扣除 */

/* 过滤条件；group/port 为 0 表示不限制 */
static uint32_t g_mcast_group_net;   /* 网络字节序 */
static uint16_t g_udp_port_net;      /* 网络字节序 */

static void signal_handler(int sig)
{
    (void)sig;
    force_quit = 1;
}

/* ------------------------------------------------------------------ */
/* 包解析：DPDK 不做协议栈，边界检查必须自己做                          */
/* ------------------------------------------------------------------ */

struct pkt_view {
    uint32_t src_ip;      /* 网络字节序 */
    uint32_t dst_ip;      /* 网络字节序 */
    uint16_t dst_port;    /* 主机字节序 */
    uint16_t payload_len;
    const uint8_t *payload;
};

/* 返回 0 = 是要的行情包；<0 = 丢弃（原因见枚举值） */
enum {
    PARSE_OK        = 0,
    PARSE_TOO_SHORT = -1,
    PARSE_NOT_IPV4  = -2,
    PARSE_NOT_UDP   = -3,
    PARSE_NO_MATCH  = -4,
};

static inline int parse_packet(const struct rte_mbuf *m, struct pkt_view *v)
{
    const struct rte_ether_hdr *eth;
    const struct rte_ipv4_hdr *ip;
    const struct rte_udp_hdr *udp;
    size_t ihl, off;

    /* 多段 mbuf（开 jumbo / RTE_ETH_RX_OFFLOAD_SCATTER 时）在内存里不连续，
       下面这套"线性指针偏移"解析会直接读飞 —— 必须先拒绝，或先
       rte_pktmbuf_linearize() 拉平。MTU 1500 且未开 scatter 时恒为 1 段。 */
    if (m->nb_segs != 1)
        return PARSE_TOO_SHORT;

    /* 关键：mbuf 里的就是裸以太网帧，没有任何内核帮你做的长度校验。
       行情包通常很小，但网络上什么包都可能有，必须先验长度再解引用。 */
    if (m->data_len < sizeof(*eth))
        return PARSE_TOO_SHORT;

    eth = rte_pktmbuf_mtod(m, const struct rte_ether_hdr *);
    if (rte_be_to_cpu_16(eth->ether_type) != RTE_ETHER_TYPE_IPV4)
        return PARSE_NOT_IPV4;

    off = sizeof(*eth);
    if (m->data_len < off + sizeof(*ip))
        return PARSE_TOO_SHORT;
    ip = (const struct rte_ipv4_hdr *)((const uint8_t *)eth + off);

    if (ip->next_proto_id != IPPROTO_UDP)
        return PARSE_NOT_UDP;

    /* IP 头长度可变（选项），必须用 IHL 而不是固定 20 字节 */
    ihl = (size_t)(ip->version_ihl & 0x0f) * 4;
    if (ihl < sizeof(*ip))
        return PARSE_TOO_SHORT;
    off += ihl;

    if (m->data_len < off + sizeof(*udp))
        return PARSE_TOO_SHORT;
    udp = (const struct rte_udp_hdr *)((const uint8_t *)eth + off);

    /* 过滤：先看 dst IP（组播组），再看端口。先比代价小的。 */
    if (g_mcast_group_net && ip->dst_addr != g_mcast_group_net)
        return PARSE_NO_MATCH;
    if (g_udp_port_net && udp->dst_port != g_udp_port_net)
        return PARSE_NO_MATCH;

    v->src_ip   = ip->src_addr;
    v->dst_ip   = ip->dst_addr;
    v->dst_port = rte_be_to_cpu_16(udp->dst_port);

    off += sizeof(*udp);
    {
        /* ★ dgram_len 是"对面声明的"，不是"实际收到的" ★
           DPDK 不会替你校验二者是否一致：一个畸形包可以声明 65535 字节，
           而 mbuf 里只有 40 字节。若直接拿它当 payload 长度去读就出界了。
           先按声明算，再与真实剩余字节核对，不一致就丢。
           正常行情包二者必然相等 —— 不等说明帧被截断或就是畸形包。 */
        uint16_t dlen = rte_be_to_cpu_16(udp->dgram_len);
        if (dlen < sizeof(*udp))
            return PARSE_TOO_SHORT;
        v->payload_len = (uint16_t)(dlen - sizeof(*udp));
    }
    if ((size_t)off + v->payload_len > m->data_len)
        return PARSE_TOO_SHORT;

    v->payload = (const uint8_t *)eth + off;

    /* 真正的行情解析在这里接：MoldUDP64 / ITCH / 自定义二进制协议。
       此处只做骨架 —— 真实实现应直接读 v->payload，不再拷贝。 */
    return PARSE_OK;
}

/* ------------------------------------------------------------------ */
/* 端口初始化                                                          */
/* ------------------------------------------------------------------ */

static struct rte_mempool *g_mbuf_pool;

static int port_init(uint16_t port_id)
{
    /* 只设多队列模式为 NONE，其余保持默认 —— 最小依赖，避免 DPDK 版本间
       结构体字段差异（如 21.11 起 rxmode.max_rx_pkt_len 被 mtu 取代）。 */
    const struct rte_eth_conf port_conf = {
        .rxmode = { .mq_mode = RTE_ETH_MQ_RX_NONE },
        .txmode = { .mq_mode = RTE_ETH_MQ_TX_NONE },
    };
    const uint16_t nb_rx_q = 1, nb_tx_q = 1;
    int ret;

    ret = rte_eth_dev_configure(port_id, nb_rx_q, nb_tx_q, &port_conf);
    if (ret < 0)
        return ret;

    ret = rte_eth_rx_queue_setup(port_id, 0, RX_RING_SIZE,
                                 rte_eth_dev_socket_id(port_id),
                                 NULL, g_mbuf_pool);
    if (ret < 0)
        return ret;

    /* TX 队列本例不发单，但 dev_configure 声明了 1 个，必须配套 setup */
    ret = rte_eth_tx_queue_setup(port_id, 0, TX_RING_SIZE,
                                 rte_eth_dev_socket_id(port_id), NULL);
    if (ret < 0)
        return ret;

    ret = rte_eth_dev_start(port_id);
    if (ret < 0)
        return ret;

    /* ★ 组播关键 ★
       旁路之后内核不再管理这张网卡，IGMP Membership Report 不会自动发出，
       交换机（IGMP snooping）可能根本不往这个端口转发流量。
       本函数只是让网卡"硬件不过滤组播"，保证流量能进到 rx_burst；
       交换机侧的组播转发仍需：静态组播配置，或应用自己发 IGMP report。
       → 12.5/chapter-02/notes/05-multicast-rx-path.md */
    ret = rte_eth_allmulticast_enable(port_id);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "allmulticast enable 失败: %d\n", ret);

    return 0;
}

/* ------------------------------------------------------------------ */
/* 主流程                                                              */
/* ------------------------------------------------------------------ */

static void usage(const char *prog)
{
    printf("用法: %s [EAL 参数] -- [-g 组播组] [-p 端口]\n"
           "  -g <a.b.c.d>  只统计该组播组（默认全部）\n"
           "  -p <port>     只统计该 UDP 目的端口（默认全部）\n"
           "例: %s -l 2 -n 4 -- -g 224.1.2.3 -p 12345\n", prog, prog);
}

int main(int argc, char **argv)
{
    uint16_t port_id;
    uint64_t tsc_hz;
    uint64_t total_pkts = 0, matched = 0, dropped_short = 0;
    uint64_t last_print = 0, last_pkts = 0;
    struct rte_eth_stats stats;
    int ret;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL 初始化失败\n");

    /* EAL 已消费掉自己的参数（含 "--"），其余是本程序的 */
    argc -= ret;
    argv += ret;

    int opt;
    while ((opt = getopt(argc, argv, "g:p:h")) != -1) {
        char grp[INET_ADDRSTRLEN];
        struct in_addr addr;
        switch (opt) {
        case 'g':
            if (inet_pton(AF_INET, optarg, &addr) != 1) {
                fprintf(stderr, "非法组播地址: %s\n", optarg);
                return EXIT_FAILURE;
            }
            g_mcast_group_net = addr.s_addr;   /* 已是网络字节序 */
            break;
        case 'p':
            g_udp_port_net = rte_cpu_to_be_16((uint16_t)atoi(optarg));
            break;
        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (rte_eth_dev_count_avail() == 0)
        rte_exit(EXIT_FAILURE, "没有可用的 DPDK 网卡，先用 dpdk-devbind.py 绑定\n");

    /* 本例单端口单队列：真实系统会按 lcore 数起多个线程，一核一队列。
       ★ 顺序很重要：先定端口，再建 mbuf 池 ★
       池的 socket_id 要跟"网卡挂在哪个 NUMA 节点"走，而不是"当前线程
       跑在哪个核上"—— 双路机器上这两者常常不是同一个节点。池建错节点，
       每个包都要多一次跨 socket 的远程内存访问，正好抵消掉旁路省下的时间。
       → chapter-02-mbuf与内存池.md 的 socket_id 一节 */
    int found = 0;
    RTE_ETH_FOREACH_DEV(port_id) { found = 1; break; }
    if (!found)
        rte_exit(EXIT_FAILURE, "没有可用端口\n");

    g_mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
                                          NUM_MBUFS,
                                          MBUF_CACHE_SIZE,
                                          0,
                                          RTE_MBUF_DEFAULT_BUF_SIZE,
                                          rte_eth_dev_socket_id(port_id));
    if (g_mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "mbuf pool 创建失败（大页配置了吗？）\n");

    if (port_init(port_id) != 0)
        rte_exit(EXIT_FAILURE, "端口 %u 初始化失败\n", port_id);

    tsc_hz = rte_get_tsc_hz();

    /* 用 TSC 前必须确认稳定，否则延迟数据不可信
       → 12.5/chapter-15/notes/03-latency-measurement.md */
    if (!rte_eal_has_hpet())
        printf("提示: 未检测到 HPET，请确保 TSC 恒定（grep constant_tsc /proc/cpuinfo）\n");

    hist_init(&hist_parse);
    hist_init(&hist_burst);
    hist_init(&hist_oh);

    /* rdtsc 基线：连续两次读取的间隔。热路径每包要读两次 rdtsc，
       这份开销属于"测量成本"而非被测对象，报告时应扣除。
       先预热再采样 —— 否则冷启动的头几个样本会把基线算高。 */
    for (int i = 0; i < 2000; i++) {
        uint64_t a = rte_rdtsc(), b = rte_rdtsc();
        (void)a; (void)b;
    }
    for (int i = 0; i < 10000; i++) {
        uint64_t a = rte_rdtsc(), b = rte_rdtsc();
        hist_record(&hist_oh, b - a);
    }

    printf("\n开始收包 (lcore %u, 端口 %u, TSC=%.3f GHz)\n"
           "  过滤: %s:%u\n  按 Ctrl-C 结束并输出分位统计\n\n",
           rte_lcore_id(), port_id, (double)tsc_hz / 1e9,
           g_mcast_group_net ? inet_ntoa(*(struct in_addr *)&g_mcast_group_net) : "*",
           rte_be_to_cpu_16(g_udp_port_net));

    struct rte_mbuf *bufs[BURST_SIZE];
    uint64_t stat_interval = (STATS_INTERVAL_US * tsc_hz) / 1000000ULL;

    while (!force_quit) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        if (nb_rx == 0)
            continue;

        /* burst 开始时刻：用于衡量"batch 内后包等待前包"的队头效应。
           BURST_SIZE 越大吞吐越高，但队头等待越长 —— 这里给出实测数据。 */
        uint64_t t_burst0 = rte_rdtsc();

        for (uint16_t i = 0; i < nb_rx; i++) {
            struct pkt_view v;
            uint64_t t0 = rte_rdtsc();
            int rc = parse_packet(bufs[i], &v);
            uint64_t t1 = rte_rdtsc();

            hist_record(&hist_parse, t1 - t0);
            hist_record(&hist_burst, t1 - t_burst0);

            if (rc == PARSE_OK) {
                matched++;
                /* 这里接行情解码：v.payload / v.payload_len */
            } else if (rc == PARSE_TOO_SHORT) {
                dropped_short++;
            }
            total_pkts++;
            rte_pktmbuf_free(bufs[i]);   /* 必须归还，否则 mbuf 池耗尽 → rx_nombuf */
        }

        /* 周期性速率输出（用 TSC 计时，不调用系统调用） */
        if (t_burst0 - last_print >= stat_interval) {
            struct rte_eth_link link;
            double secs = (double)(t_burst0 - last_print) / (double)tsc_hz;
            printf("  收包 %llu pps  累计 %llu  命中 %llu\n",
                   (unsigned long long)((total_pkts - last_pkts) / secs),
                   (unsigned long long)total_pkts,
                   (unsigned long long)matched);
            rte_eth_link_get_nowait(port_id, &link);
            if (!link.link_status)
                printf("  ⚠ 链路 down —— 检查光模块/交换机\n");
            last_print = t_burst0;
            last_pkts  = total_pkts;
        }
    }

    /* ---------------- 退出统计 ---------------- */
    printf("\n================ 统计 ================\n");
    printf("总包数 %llu  命中行情 %llu  过短丢弃 %llu\n",
           (unsigned long long)total_pkts,
           (unsigned long long)matched,
           (unsigned long long)dropped_short);

    memset(&stats, 0, sizeof(stats));
    if (rte_eth_stats_get(port_id, &stats) == 0) {
        printf("\n网卡计数:\n");
        printf("  ipackets  %llu   ibytes  %llu\n",
               (unsigned long long)stats.ipackets, (unsigned long long)stats.ibytes);
        printf("  imissed   %llu   <- 网卡收不进来（PCIe/队列满）\n",
               (unsigned long long)stats.imissed);
        printf("  ierrors   %llu\n", (unsigned long long)stats.ierrors);
        printf("  rx_nombuf %llu   <- mbuf 池耗尽（用户态消费太慢）\n",
               (unsigned long long)stats.rx_nombuf);
        if (stats.imissed || stats.rx_nombuf)
            printf("\n⚠ 有丢包。组播无重传，必须做序列号 gap 检测 + TCP 补单通道。\n");
    }

    hist_dump(&hist_oh,    1e9 / (double)tsc_hz, "rdtsc 基线开销 (ns) —— 报告时扣除");
    hist_dump(&hist_parse, 1e9 / (double)tsc_hz, "每包解析延迟 (ns)");
    hist_dump(&hist_burst, 1e9 / (double)tsc_hz, "burst 内位置延迟 (ns)");
    printf("\n说明: hist_parse 是纯解析代码开销；hist_burst 含 batch 内队头等待，\n"
           "      后者更接近端到端真实延迟。两者都包含了每包 2 次 rdtsc 的开销，\n"
           "      严格报告时按上面的基线扣除。对比这两项能看出 BURST_SIZE 的代价。\n");

    rte_eth_dev_stop(port_id);
    rte_eth_dev_close(port_id);
    return EXIT_SUCCESS;
}
