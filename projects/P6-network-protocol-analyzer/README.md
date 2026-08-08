# P6 — 网络协议分析器

> 用 raw socket 抓包、逐层解析、TCP 流重组，再用 eBPF 追踪内核 NAPI 收包路径，把"报文从网卡到用户态"整条链看穿。
> **做法：项目驱动，[`15`](../../15-network-sockets/) / [`16`](../../16-tcpip-protocols/) / [`17`](../../17-kernel-networking/) / [`20`](../../20-bpf-observability/) 笔记当字典。**

---

## 核心理念

实现一个迷你版 Wireshark + 内核观测器。这是 HFT 网络栈的"理解层"——不优化、不旁路，先把标准路径每一跳讲通。

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [UNP socket 基础](../../15-network-sockets/unix-network-api/1_BasicFoundation/) | socket/bind/recvfrom |
| [TCP/IP ch03 链路层](../../16-tcpip-protocols/chapter03-link-layer/) | Ethernet 帧格式 |
| [TCP/IP ch05 IP](../../16-tcpip-protocols/chapter05-ip-protocol/) | IP 首部字段 |
| [Rosen ch01 引言](../../17-kernel-networking/chapter-01-introduction/) | sk_buff、收包路径概览 |
| [BPF ch01 引言](../../20-bpf-observability/chapter-01-introduction/notes/) | eBPF 是什么 |

---

## Phase 1：raw socket 抓包 + 逐层解析（2-3 小时）

### 做什么

用 `AF_PACKET` raw socket 抓包，解析 Ethernet → IP → TCP/UDP 首部。

### 代码骨架

```c
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

// 创建 raw socket（需 root）
int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

// 可选：绑定到特定网卡
struct sockaddr_ll sll = {0};
sll.sll_ifindex = if_nametoindex("eth0");
bind(sock, (struct sockaddr *)&sll, sizeof(sll));

// 抓包循环
unsigned char buf[65536];
for (;;) {
    int n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);

    // 解析 Ethernet 首部 (14 字节)
    struct ethhdr *eth = (struct ethhdr *)buf;
    uint16_t eth_type = ntohs(eth->h_proto);
    printf("ETH: src=%02x:%02x:... type=0x%04x\n",
           eth->h_source[0], eth->h_source[1], eth_type);

    if (eth_type != ETH_P_IP) continue;  // 只看 IP

    // 解析 IP 首部
    struct iphdr *ip = (struct iphdr *)(buf + 14);
    char src_ip[16], dst_ip[16];
    inet_ntop(AF_INET, &ip->saddr, src_ip, sizeof(src_ip));
    inet_ntop(AF_INET, &ip->daddr, dst_ip, sizeof(dst_ip));
    printf("IP: %s -> %s proto=%d ttl=%d\n",
           src_ip, dst_ip, ip->protocol, ip->ttl);

    // 解析 TCP/UDP
    int ip_hdr_len = ip->ihl * 4;
    if (ip->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp = (struct tcphdr *)(buf + 14 + ip_hdr_len);
        printf("TCP: %d -> %d seq=%u ack=%u flags=%s%s%s\n",
               ntohs(tcp->source), ntohs(tcp->dest),
               ntohl(tcp->seq), ntohl(tcp->ack_seq),
               tcp->syn ? "SYN " : "", tcp->ack ? "ACK " : "",
               tcp->fin ? "FIN " : "");
    }
}
```

### 分步实现

1. **`socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))`**：抓所有协议的包（需 root）
2. **解析 Ethernet**：14 字节首部，`h_proto` 判断上层协议（0x0800=IPv4）
3. **解析 IP**：`ihl * 4` = IP 首部长度（可变），`protocol` 判断 TCP(6)/UDP(17)/ICMP(1)
4. **解析 TCP**：20 字节固定首部，seq/ack/flags 是后续流重组的关键
5. **加 BPF 过滤**：`setsockopt(SO_ATTACH_FILTER)` 只抓 TCP，减少用户态处理量

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 需要 root | socket 创建失败 | `AF_PACKET` 需要 CAP_NET_RAW |
| 看到自己的包 | 回环 | `ETH_P_ALL` 包括发出去的包 |
| IP 首部长度算错 | 解析错位 | `ihl` 以 4 字节为单位，要 `* 4` |
| 字节序 | 端口号不对 | 网络字节序是大端，用 `ntohs` |
| TCP 选项没跳过 | TCP 数据偏移 | `tcphdr->doff * 4` = TCP 首部长度 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| raw socket API | [UNP 基础](../../15-network-sockets/unix-network-api/1_BasicFoundation/) |
| Ethernet/IP/TCP 首部 | [TCP/IP ch03/05](../../16-tcpip-protocols/chapter03-link-layer/) |
| 字节序 | [CSAPP ch02](../../02-computer-systems/chapter-02-representing-information/) |

---

## Phase 2：TCP 流重组 + 统计（2-3 小时）

### 做什么

按 seq 号排序 TCP 段，处理重传，统计 RTT 和重传率。

### 代码骨架

```c
// 每条 TCP 流一个上下文
struct tcp_flow {
    uint32_t src_ip, dst_ip;
    uint16_t src_port, dst_port;
    uint32_t expected_seq;       // 期望的下一个 seq
    uint64_t bytes_received;
    uint64_t retransmit_count;
    // 重排序缓冲（乱序段先存这里）
    struct {
        uint32_t seq;
        uint8_t *data;
        size_t len;
    } reorder_buf[64];
    int reorder_count;
};

// 处理一个 TCP 段
void process_tcp_segment(struct tcp_flow *flow, uint32_t seq,
                         uint8_t *payload, size_t len) {
    if (seq < flow->expected_seq) {
        // 重传！
        flow->retransmit_count++;
        return;
    }
    if (seq > flow->expected_seq) {
        // 乱序，存入重排序缓冲
        store_reorder(flow, seq, payload, len);
        return;
    }
    // 顺序到达
    flow->bytes_received += len;
    flow->expected_seq = seq + len;

    // 检查重排序缓冲里有没有接上的
    flush_reorder_buf(flow);
}
```

### 分步实现

1. **流表**：用哈希表（src_ip, dst_ip, src_port, dst_port）索引 TCP 流
2. **seq 排序**：`seq < expected` = 重传，`seq > expected` = 乱序，`seq == expected` = 顺序
3. **重排序缓冲**：乱序段先存起来，等前面的段到了再按序输出
4. **RTT 估算**：用 SYN/SYN-ACK 的时间差估算初始 RTT，或用 Timestamps 选项
5. **统计输出**：每协议包数、字节数、重传率、RTT 分布

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| seq 回绕 | 大流量后序号错乱 | seq 是 32-bit，高速下会回绕（需 PAWS）|
| 流表太大 | 内存爆 | 用 LRU 或超时清理 |
| 重传判断错 | 误判/漏判 | 重传 = seq < expected，但要排除重复 ACK |
| 合并方向 | 双向流混在一起 | 四元组要区分方向 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| TCP 序列号/重传 | [TCP/IP ch09-11](../../16-tcpip-protocols/) (找 TCP 相关章) |
| 内核 TCP 实现 | [Rosen](../../17-kernel-networking/) (找 TCP 章节) |

---

## Phase 3：eBPF 追踪内核收包路径（2 小时）

### 做什么

用 bpftrace 追踪内核 NAPI 收包路径，与用户态抓包时间线对照。

### 分步实现

1. **安装 bpftrace**：`sudo apt install bpftrace`
2. **追踪收包关键函数**：
   ```bash
   # 追踪 netif_receive_skb（内核收包入口）
   sudo bpftrace -e '
   tracepoint:net:netif_receive_skb {
       @rx_count++;
       printf("%lld netif_receive_skb dev=%s len=%d\n",
              nsecs, args->name, args->len);
   }
   '
   ```
3. **追踪 NAPI poll**：
   ```bash
   sudo bpftrace -e '
   kprobe:napi_poll /comm == "my_analyzer"/ {
       printf("%lld napi_poll\n", nsecs);
   }
   '
   ```
4. **追踪 softirq**：
   ```bash
   sudo bpftrace -e '
   tracepoint:irq:softirq_entry /args->vec == 3/ {
       printf("%lld NET_RX softirq\n", nsecs);
   }
   '
   ```
5. **对照**：同时跑 bpftrace（内核侧）和你的抓包程序（用户态），看同一个包从网卡到用户态的时间差

### 你会看到什么

```
[内核侧 bpftrace]                    [用户态抓包]
1689000000 netif_receive_skb          ← 网卡收包
1689000005 napi_poll                  ← NAPI 轮询
1689000010 NET_RX softirq             ← 软中断处理
1689000050 recvfrom returns           ← 用户态收到（~50us 后）
```

这个 ~50us 就是内核网络栈的开销——P7 用 DPDK 旁路掉的就是这个。

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| bpftrace 语法 | [BPF ch04 bcc](../../20-bpf-observability/chapter-04-bcc/notes/) |
| NAPI 收包路径 | [Rosen ch01](../../17-kernel-networking/chapter-01-introduction/) |
| 现代 NAPI/XDP | [17.5 modern-net](../../17.5-modern-networking/lwn-articles-summary/) |

---

## Phase 4：输出 pcap + Wireshark 验证（30 分钟）

### 做什么

把抓到的包写成 pcap 格式文件，用 Wireshark 打开对照。

### 分步实现

1. **写 pcap 文件头**：
   ```c
   struct pcap_file_header {
       uint32_t magic;      // 0xa1b2c3d4
       uint16_t version_major;  // 2
       uint16_t version_minor;  // 4
       int32_t  thiszone;       // 0
       uint32_t sigfigs;        // 0
       uint32_t snaplen;        // 65535
       uint32_t linktype;       // 1 = ETHERNET
   };
   ```
2. **每个包写 pcap 包头 + 原始数据**：
   ```c
   struct pcap_packet_header {
       uint32_t ts_sec;
       uint32_t ts_usec;
       uint32_t incl_len;   // 实际写入长度
       uint32_t orig_len;   // 原始包长度
   };
   write(fd, &pkt_hdr, sizeof(pkt_hdr));
   write(fd, buf, n);
   ```
3. **验证**：`wireshark capture.pcap` 或 `tcpdump -r capture.pcap`

---

## 交付物

- [ ] raw socket / AF_PACKET 抓包（含混杂模式）
- [ ] 逐层解析：Ethernet → IP → TCP/UDP → 应用层
- [ ] TCP 流重组（按 seq 排序、处理重传、流缓冲）
- [ ] 统计：每协议包数、字节、重传率、RTT 估算
- [ ] eBPF/bpftrace 追踪 NAPI 收包路径
- [ ] 输出 pcap 格式，可用 Wireshark 打开对照

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`15` network-sockets](../../15-network-sockets/) | UNP：raw socket、socket 选项 |
| [`16` tcpip-protocols](../../16-tcpip-protocols/) | Stevens 卷一：IP/TCP/UDP 首部与协议行为 |
| [`17` kernel-networking](../../17-kernel-networking/) | Rosen：sk_buff、NAPI、收包路径 |
| [`17.5` modern-networking](../../17.5-modern-networking/) | 现代 6.x：page_pool、XDP hook、eBPF 网络 |
| [`20` bpf-observability](../../20-bpf-observability/) | bpftrace 追踪内核网络函数 |

## 前置

[P3](../P3-http-server/)（用户态网络编程过关）。

## 里程碑

1. **M1** raw socket 抓包 + 打印 Ethernet/IP/TCP 首部 → Phase 1
2. **M2** TCP 流重组 + 重传检测 → Phase 2
3. **M3** 统计 + RTT 估算 → Phase 2
4. **M4** bpftrace 追踪 NAPI 路径，与抓包时间线对照 → Phase 3
5. **M5** 输出 pcap，Wireshark 验证 → Phase 4

## 状态

⬜ 未开始 → 建议先用 `sudo tcpdump -i eth0` 看看抓包效果，然后写自己的 raw socket。

← [projects 总览](../README.md) · [15 模块](../../15-network-sockets/) · [17 模块](../../17-kernel-networking/)
