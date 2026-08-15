# HFT 场景 05：eBPF 对比 Wireshark

> [总览](./00-overview.md) · [HFT 模块：BPF 可观测性](../../16-bpf-observability/) · [HFT 场景 03：内核旁路](./03-kernel-bypass-limitations.md)

**核心问题**：Wireshark 在用户态处理包，高流量场景下会丢包或增加延迟。eBPF 在内核态过滤和处理，开销更低，但能力不同。两者不是替代关系，而是互补。

## 1. 架构对比

```
Wireshark/tcpdump 路径：
  网卡 RX → 内核驱动 → 协议栈 → AF_PACKET socket → 用户态
                                              ↑
                                         全量拷贝到用户态
                                         高流量下丢包/高 CPU

eBPF 路径：
  网卡 RX → XDP/TC eBPF 程序 → 内核态处理（过滤/统计/重定向）
                                ↑
                           零拷贝，不到达用户态
                           可达线速（10G/40G/100G）
```

| 维度 | Wireshark/tcpdump | eBPF/bpftrace |
|------|-------------------|---------------|
| **处理位置** | 用户态 | 内核态 |
| **包拷贝** | 全量拷贝到用户态 | 零拷贝（XDP）或按需拷贝 |
| **性能开销** | 高（1Gbps+ 开始丢包） | 极低（线速 100Gbps） |
| **过滤能力** | BPF 过滤器（在内核） + 显示过滤器（在用户态） | eBPF 程序（任意逻辑） |
| **协议解析** | 丰富（3000+ 协议） | 需手写解析逻辑 |
| **实时分析** | 实时 + 离线 | 实时为主，可导出 pcap |
| **HFT 适用** | 离线分析 + 低流量实时 | 生产环境实时监控 |
```

## 2. 各自定位

| 场景 | 推荐 | 原因 |
|------|------|------|
| 协议学习/教学 | Wireshark | 丰富的协议解析和 GUI |
| 离线 pcap 分析 | Wireshark | 最好的离线分析工具 |
| 生产环境实时监控 | eBPF | 低开销，不影响交易延迟 |
| 高流量抓包（10G+） | eBPF 过滤 + tcpdump 落盘 | 先 eBPF 过滤，再 tcpdump 抓过滤后的 |
| 深度协议解析 | Wireshark | eBPF 不擅长复杂协议树 |
| 延迟/丢包统计 | eBPF | 内核态时间戳更准确 |
| 安全分析/入侵检测 | 两者结合 | eBPF 实时告警 + Wireshark 离线取证 |

## 3. eBPF 网络监控工具

| 工具 | 功能 | 对标 Wireshark |
|------|------|---------------|
| **bpftrace** | 一行命令追踪内核函数 | 无对标（Wireshark 不追踪内核函数） |
| **BCC tcptop** | TCP 连接吞吐统计 | Statistics → Conversations |
| **BCC tcplife** | TCP 连接生命周期 | 无直接对标 |
| **BCC tcpdrop** | 内核丢包追踪 | Expert Info（但只能看包内信息） |
| **BCC tcpretrans** | TCP 重传追踪 | `tcp.analysis.retransmission` |
| **BCC biolatency** | 块 I/O 延迟 | 无对标（非网络） |
| **Cilium Hubble** | K8s 网络可观测性 | 无对标（K8s 专属） |

## 4. 用 eBPF 增强 Wireshark 抓不到的场景

### 场景 A：内核协议栈丢包

Wireshark 只能看到**到达 AF_PACKET 的包**，内核在协议栈中丢弃的包看不到。

```bash
# 用 BCC tcpdrop 追踪内核丢包
sudo bpftrace -e '
  kprobe:tcp_drop {
    printf("DROP: %s pid=%d saddr=%s daddr=%s sport=%d dport=%d\n",
      comm, pid,
      ntop(args->sk->__sk_common.skc_daddr),
      ntop(args->sk->__sk_common.skc_rcv_saddr),
      args->sk->__sk_common.skc_dport >> 8,
      args->sk->__sk_common.skc_num);
  }'

# 或用 BCC 工具
sudo tcpdrop
```

### 场景 B：XDP 层处理

XDP 程序在网卡驱动层运行，Wireshark 完全看不到。

```bash
# 用 xdpdump（xdp-tools 包）抓 XDP 层的包
sudo xdpdump -i eth0 --rx-capture entry -w xdp_capture.pcap

# 然后用 Wireshark 分析
wireshark xdp_capture.pcap
```

### 场景 C：Socket 队列延迟

Wireshark 能看到包到达时间，但看不到包在 Socket 队列中等待了多久。

```bash
# 用 bpftrace 追踪 skb 从到达到被应用 read 的时间
sudo bpftrace -e '
  kprobe:tcp_rcv_established {
    @skb_time[args->skb] = nsecs;
  }
  kretprobe:tcp_recvmsg /@skb_time[args->skb]/ {
    $delta = (nsecs - @skb_time[args->skb]) / 1000;
    printf("skb queue delay: %d us\n", $delta);
    delete(@skb_time[args->skb]);
  }'
```

## 5. 协作工作流：eBPF 过滤 + tcpdump 落盘 + Wireshark 分析

```bash
# 步骤 1：用 eBPF 在内核态过滤出目标流量（不拷贝包，只标记）
# 例如：只抓交易服务器 IP 的 TCP 重传包

# 步骤 2：用 tcpdump 配合 BPF 过滤器抓包落盘
sudo tcpdump -nni eth0 -w retrans_only.pcap \
  'host 10.0.0.5 and tcp[tcpflags] & (tcp-syn|tcp-fin|tcp-rst) == 0'

# 步骤 3：用 eBPF 记录内核事件（时间戳 + 上下文）
sudo bpftrace -e '
  kprobe:tcp_retransmit_skb {
    printf("%lld retransmit seq=%d\n", nsecs, args->skb->seq);
  }' > retrans_events.txt &

# 步骤 4：Wireshark 离线分析 pcap，结合 eBPF 事件日志
wireshark retrans_only.pcap
# 将 Wireshark 时间戳与 retrans_events.txt 对齐
```

## 6. 性能对比：什么时候必须用 eBPF

| 流量速率 | tcpdump 丢包率 | eBPF 丢包率 | 建议 |
|----------|---------------|-------------|------|
| < 1 Gbps | ~0% | 0% | tcpdump + Wireshark |
| 1–10 Gbps | 5–30% | ~0% | eBPF 过滤 + tcpdump 落盘 |
| 10–40 Gbps | 50%+ | <1% | eBPF 统计，必要时 AF_XDP |
| 40–100 Gbps | 90%+ | <5% | 必须 eBPF/XDP |

```bash
# 测试 tcpdump 丢包率
sudo tcpdump -nni eth0 -w test.pcap -c 100000
# 然后对比 ifconfig rx packets 数量

# 检查 tcpdump 是否丢包
sudo tcpdump -nni eth0 -c 10000
# 输出最后一行： "N packets captured" vs "N packets received by filter"
# 如果 received > captured → 丢包了
```

## 7. HFT 自测题

1. 为什么在 10Gbps 流量下 `tcpdump -w` 会丢包，而 eBPF 程序不会？
2. 你需要分析一个交易连接的 TCP 重传，但流量是 25Gbps。设计一个 eBPF + Wireshark 协作方案。
3. `tcpdump` 输出显示 "10000 packets captured, 12000 packets received by filter"。这意味着什么？
4. XDP 程序返回 `XDP_DROP` 的包，Wireshark 能看到吗？为什么？如何抓到？

## 交叉引用

- [HFT 场景 01：TCP 延迟](./01-tcp-latency-analysis.md)
- [HFT 场景 03：内核旁路](./03-kernel-bypass-limitations.md)
- [HFT 场景 04：容器/云抓包](./04-container-cloud-capture.md)
- [HFT 模块：BPF 可观测性](../../16-bpf-observability/)
- [HFT 模块：内核网络](../../13-kernel-networking/)
- [HFT 模块：现代网络](../../13.5-modern-networking/)
- [HFT 模块：系统性能](../../15-systems-performance/)
