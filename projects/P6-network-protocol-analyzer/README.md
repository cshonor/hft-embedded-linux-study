# P6 — 网络协议分析器

> 用 raw socket 抓包、逐层解析、TCP 流重组，再用 eBPF 追踪内核 NAPI 收包路径，把"报文从网卡到用户态"整条链看穿。

## 项目目标

实现一个迷你版 Wireshark + 内核观测器。这是 HFT 网络栈的"理解层"——不优化、不旁路，先把标准路径每一跳讲通。

## 交付物

- [ ] raw socket / AF_PACKET 抓包（含混杂模式）
- [ ] 逐层解析：Ethernet → IP → TCP/UDP → 应用层
- [ ] TCP 流重组（按 seq 排序、处理重传、流缓冲）
- [ ] 统计：每协议包数、字节、重传率、RTT 估算
- [ ] eBPF/bpftrace 追踪 NAPI 收包路径（`net_dev_queue`/`napi_poll`）
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

## 学习目标

- 报文各层首部字段与协议行为（IP 分片、TCP 窗口/重传）
- raw socket 抓包的权限与过滤（BPF filter）
- TCP 流重组的边界情况（乱序、重传、半连接）
- 内核收包路径：中断 → NAPI → softirq → sk_buff → socket
- eBPF 追踪内核函数的低开销观测

## 里程碑

1. **M1** raw socket 抓包 + 打印 Ethernet/IP/TCP 首部
2. **M2** TCP 流重组 + 重传检测
3. **M3** 统计 + RTT 估算
4. **M4** bpftrace 追踪 NAPI 路径，与抓包时间线对照
5. **M5** 输出 pcap，Wireshark 验证

## 参考模块

- [15-network-sockets/](../../15-network-sockets/) — UNP Ch3/4/8、PNP epoll 实战
- [16-tcpip-protocols/](../../16-tcpip-protocols/) — TCP/IP 卷一 Ch3/8/9-11
- [17-kernel-networking/](../../17-kernel-networking/) — Rosen Ch11/14、组播 IGMP
- [17.5-modern-networking/](../../17.5-modern-networking/) — LWN 收包路径/XDP/NAPI 现代版
- [20-bpf-observability/](../../20-bpf-observability/) — bpftrace 网络 tracing
