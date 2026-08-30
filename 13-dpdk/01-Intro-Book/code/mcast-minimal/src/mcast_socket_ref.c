/* SPDX-License-Identifier: BSD-3-Clause
 *
 * mcast-socket-ref —— 内核协议栈版 UDP 组播收包（对照组）
 *
 * 存在的唯一目的：给 DPDK 版提供 baseline，量化"旁路到底省了什么"。
 * 同一个 hist.h、同一套分位统计口径，两个数可以直接比。
 *
 * 对照关系（口径要对齐，否则数字没有意义）：
 *   hist_recv   = recvmmsg 系统调用 + 内核→用户态拷贝  ← 走内核栈独有，DPDK 为 0
 *   hist_burst  = 批次内位置延迟                      ← 与 DPDK 版同口径
 *   hist_oh     = 测量本身开销（基线，报告里要扣除）
 *   socket 端到端 ≈ hist_recv + hist_burst
 *
 * 内核栈多做的事（本程序享受了但看不到）：
 *   - 自动发 IGMP Membership Report（交换机才会转发）
 *   - IP/UDP 头解析、校验和验证
 *   - 组播复制（同一组多个 socket 各拿一份）
 *   - socket 匹配、接收队列、唤醒/epoll
 * 代价就是上面这些全部要 CPU 时间和一次内存拷贝。
 *
 * 无需 DPDK / 大页 / 网卡绑定 —— 任何 Linux 都能编译运行（含树莓派）。
 * 编译: make mcast_socket_ref    或    gcc -O2 -Wall -o mcast_socket_ref mcast_socket_ref.c
 * 运行: ./mcast_socket_ref -g 224.1.2.3 -p 12345 -i 192.168.1.10 -v 32
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "hist.h"

#define MAX_VLEN   1024
#define BUFSIZE    2048
#define DEF_VLEN   32      /* 对应 DPDK 版的 BURST_SIZE */

static volatile sig_atomic_t force_quit;

static struct hist hist_recv;    /* recvmmsg 调用耗时 */
static struct hist hist_burst;   /* 批次内位置延迟 */
static struct hist hist_oh;      /* 时钟开销基线 */

static void signal_handler(int sig)
{
    (void)sig;
    force_quit = 1;
}

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    /* CLOCK_MONOTONIC_RAW：不受 NTP 频率调整影响，内部测量唯一正确的选择。
       绝不能用 CLOCK_REALTIME —— NTP 跳变会让你测出负延迟。 */
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void usage(const char *prog)
{
    printf("用法: %s -g <组播组> -p <端口> [-i <本机IP>] [-v <批量>] [-b <busy-poll微秒>]\n"
           "  -g  组播组地址（必需）\n"
           "  -p  UDP 端口（必需）\n"
           "  -i  加入组播使用的本机接口 IP（默认 INADDR_ANY）\n"
           "  -v  recvmmsg 批量大小，对应 DPDK 的 BURST_SIZE（默认 %d）\n"
           "  -b  SO_BUSY_POLL 微秒数，0=关闭（默认 0）\n"
           "例: %s -g 224.1.2.3 -p 12345 -i 192.168.1.10 -v 32 -b 50\n",
           prog, DEF_VLEN, prog);
}

int main(int argc, char **argv)
{
    const char *grp_str = NULL, *if_str = NULL;
    int port = 0, vlen = DEF_VLEN, busy_us = 0;
    int fd, opt, rc;

    while ((opt = getopt(argc, argv, "g:p:i:v:b:h")) != -1) {
        switch (opt) {
        case 'g': grp_str = optarg; break;
        case 'p': port = atoi(optarg); break;
        case 'i': if_str = optarg;    break;
        case 'v': vlen = atoi(optarg); break;
        case 'b': busy_us = atoi(optarg); break;
        default:  usage(argv[0]); return EXIT_FAILURE;
        }
    }
    if (!grp_str || port <= 0) { usage(argv[0]); return EXIT_FAILURE; }
    if (vlen <= 0 || vlen > MAX_VLEN) {
        fprintf(stderr, "vlen 必须在 1..%d\n", MAX_VLEN);
        return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return EXIT_FAILURE; }

    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    /* 接收缓冲：组播突发时队列满了就丢，且 UDP 不重传 */
    int rcvbuf = 32 * 1024 * 1024;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    /* busy polling：拿 CPU 换延迟，跳过中断唤醒。
       5.11+ 还有 napi_defer_hard_irqs + gro_flush_timeout 的网卡级方案，
       见 12.5/chapter-02/notes/06-busy-poll-mechanism.md */
    if (busy_us > 0) {
        if (setsockopt(fd, SOL_SOCKET, SO_BUSY_POLL, &busy_us, sizeof(busy_us)) < 0)
            perror("SO_BUSY_POLL (忽略，内核可能不支持)");
        else
            printf("busy poll: %d us（该核将 100%% 占用）\n", busy_us);
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family      = AF_INET;
    sa.sin_port        = htons((uint16_t)port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);   /* 收该端口所有组播，用户态再过滤 */

    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("bind"); return EXIT_FAILURE;
    }

    /* 加入组播 —— 内核会自动发出 IGMP Membership Report。
       这一步是内核栈相对 DPDK 的最大便利：交换机 IGMP snooping 自动学到，
       流量立刻过来。DPDK 旁路后没有这个，必须自己处理。 */
    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    if (inet_pton(AF_INET, grp_str, &mreq.imr_multiaddr) != 1) {
        fprintf(stderr, "非法组播地址: %s\n", grp_str);
        return EXIT_FAILURE;
    }
    if (if_str) {
        if (inet_pton(AF_INET, if_str, &mreq.imr_interface) != 1) {
            fprintf(stderr, "非法接口地址: %s\n", if_str);
            return EXIT_FAILURE;
        }
    } else {
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    }
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("IP_ADD_MEMBERSHIP (接口地址对吗? 组播路由配了吗?)");
        return EXIT_FAILURE;
    }

    /* 预分配：热路径不做任何内存分配 */
    static struct mmsghdr msgs[MAX_VLEN];
    static struct iovec   iovecs[MAX_VLEN];
    static char           bufs[MAX_VLEN][BUFSIZE];

    for (int i = 0; i < vlen; i++) {
        iovecs[i].iov_base         = bufs[i];
        iovecs[i].iov_len          = BUFSIZE;
        msgs[i].msg_hdr.msg_iov    = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
    }

    hist_init(&hist_recv);
    hist_init(&hist_burst);
    hist_init(&hist_oh);

    /* 基线：连续两次 now_ns 的间隔 = 测量本身的开销，报告时要扣除。
       先跑 2000 次预热，把时钟调用本身带进缓存/分支预测的稳定状态；
       否则头几个样本会偏慢，把基线算高。 */
    for (int i = 0; i < 2000; i++) {
        uint64_t a = now_ns(), b = now_ns();
        (void)a; (void)b;
    }
    for (int i = 0; i < 10000; i++) {
        uint64_t a = now_ns(), b = now_ns();
        hist_record(&hist_oh, b - a);
    }

    printf("\n开始收包 (内核协议栈 / recvmmsg vlen=%d)\n", vlen);
    printf("  组播组 %s:%d\n  按 Ctrl-C 结束并输出分位统计\n\n", grp_str, port);

    uint64_t total = 0, batches = 0;
    uint64_t last_print_ns = now_ns(), last_total = 0;

    while (!force_quit) {
        /* MSG_WAITFORONE: 阻塞等第一个包，其余非阻塞收 —— 低延迟场景的标准用法。
           没有它，recvmmsg 会尽量凑满 vlen，尾延迟会变难看。 */
        uint64_t t0 = now_ns();
        rc = recvmmsg(fd, msgs, (unsigned)vlen, MSG_WAITFORONE, NULL);
        uint64_t t1 = now_ns();
        if (rc < 0) {
            if (errno == EINTR) continue;
            perror("recvmmsg");
            break;
        }
        if (rc == 0) continue;

        hist_record(&hist_recv, t1 - t0);
        batches++;
        total += (uint64_t)rc;

        for (int i = 0; i < rc; i++) {
            /* 与 DPDK 版同口径：批次内第 i 个包相对批次开始的位置延迟。
               ★ 每包只调用一次 now_ns()。多打一个时间戳，那个调用的开销
                 就会被算进 t2 - t0 里 —— 测量代码污染被测路径，
                 会让 hist_burst 每包虚高约 25ns。基线已在启动时单独测好。
               真实解析放在这里（本骨架只统计数据已到用户态）。 */
            (void)msgs[i].msg_len;
            uint64_t t2 = now_ns();
            hist_record(&hist_burst, t2 - t0);
        }

        uint64_t now = now_ns();
        if (now - last_print_ns >= 1000000000ULL) {
            double secs = (double)(now - last_print_ns) / 1e9;
            printf("  收包 %.0f pps  累计 %llu  批次 %llu\n",
                   (double)(total - last_total) / secs,
                   (unsigned long long)total,
                   (unsigned long long)batches);
            last_print_ns = now;
            last_total    = total;
        }
    }

    printf("\n============ 统计（内核协议栈） ============\n");
    printf("总包数 %llu  批次数 %llu  平均批量 %.1f\n",
           (unsigned long long)total, (unsigned long long)batches,
           batches ? (double)total / (double)batches : 0.0);

    /* socket 队列溢出计数：组播无重传，这个数必须盯着 */
    int ovfl = 0;
    socklen_t olen = sizeof(ovfl);
    if (getsockopt(fd, SOL_SOCKET, SO_RXQ_OVFL, &ovfl, &olen) == 0) {
        printf("socket 队列溢出丢弃: %u\n", (unsigned)ovfl);
        if (ovfl)
            printf("⚠ 有丢包。组播无重传，必须做序列号 gap 检测 + TCP 补单通道。\n");
    }

    hist_dump(&hist_oh,    1.0, "测量基线开销 (ns) —— 报告时扣除");
    hist_dump(&hist_recv,  1.0, "recvmmsg 耗时 (ns) —— 系统调用 + 内核拷贝");
    hist_dump(&hist_burst, 1.0, "批次内位置延迟 (ns) —— 与 DPDK 版同口径");

    printf("\n与 DPDK 版对照: socket 端到端 ≈ recvmmsg + burst；\n"
           "DPDK 没有 recvmmsg 这一项（无系统调用、无拷贝），只有 burst。\n"
           "差值就是内核协议栈 + 数据拷贝的成本。\n");

    close(fd);
    return EXIT_SUCCESS;
}
