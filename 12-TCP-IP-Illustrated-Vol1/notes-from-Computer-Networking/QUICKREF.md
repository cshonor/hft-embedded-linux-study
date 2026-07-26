# 18 章一页速览 · 考点 + Go/Rust（Fall 第 2 版）

> 配合 [OUTLINE.md](./OUTLINE.md) · 自顶向下精读 [03_transport_layer/study.md](../top_down/03_transport_layer/study.md)

| 章 | 主题 | 核心考点 | Go / Rust 场景 |
|----|------|----------|----------------|
| 1 | 概述 | **命运共享/端到端**、封装分用、Clark 目标、[考点](chapter01-overview/study.md#ch01-exam) | Socket API；见 [ch01 精读](chapter01-overview/study.md) |
| 2 | 地址结构 | **CIDR/VLSM**、私有/环回、EUI-64、[考点](chapter02-ip-address-architecture/study.md#ch02-exam) | `net.ParseIP`、`IPNet`、前缀 `/x` → [ch02](chapter02-ip-address-architecture/study.md) |
| 3 | 链路层 | 以太网/802.11/PPP、**MTU 1500**、STP、CSMA/CA、[考点](chapter03-link-layer/study.md#ch03-exam) | MTU/MSS、环回 `127.0.0.1` → [ch03](chapter03-link-layer/study.md) |
| 4 | ARP | 广播请求/单播应答、**Incomplete**、代理/免费 ARP、[考点](chapter04-arp-protocol/study.md#ch04-exam) | `arp -a` / `ip neigh`；IPv6 用 ND → [ch04](chapter04-arp-protocol/study.md) |
| 5 | IP | v4/v6 头、**LPM**、v6 仅源分片、Mobile IP、[考点](chapter05-ip-protocol/study.md#ch05-exam) | `Don't Fragment`、TTL → [ch05](chapter05-ip-protocol/study.md) |
| 6 | DHCP | **DORA**、T1/T2、SLAAC/EUI-64、Rogue DHCP、[考点](chapter06-dhcp-config/study.md#ch06-exam) | UDP 67/68；169.254 APIPA → [ch06](chapter06-dhcp-config/study.md) |
| 7 | NAT/防火墙 | NAPT、**EIM/ADF**、Hairpin、STUN/ICE、[考点](chapter07-firewall-nat/study.md#ch07-exam) | K8s SNAT、conntrack 满 → [ch07](chapter07-firewall-nat/study.md) |
| 8 | ICMP | ND/DAD、**PTB/PMTUD**、差错不嵌套、[考点](chapter08-icmpv4-icmpv6/study.md#ch08-exam) | 勿拦 Type2；`ip neigh` → [ch08](chapter08-icmpv4-icmpv6/study.md) |
| 9 | 广播/多播 | 32:1 MAC 映射、IGMPv3/SSM、[考点](chapter09-broadcast-multicast/study.md#ch09-exam) | IGMP Snooping、mDNS → [ch09 考点精读](chapter09-broadcast-multicast/study.md#ch09-exam) |
| 10 | UDP | 消息边界、v6 强制校验和、**分片连锁丢包**、[考点](chapter10-udp-ip-fragment/study.md#ch10-exam) | 单包 ≤1400B；[ch10 考点精读](chapter10-udp-ip-fragment/study.md#ch10-exam)；[§3.3 UDP](../top_down/03_transport_layer/study.md#ch3-3) |
| 11 | DNS | 递归/权威、**TTL**、EDNS0/TC→TCP、[考点](chapter11-dns-domain-resolve/study.md#ch11-exam) | `dig +trace`、Happy Eyeballs → [ch11](chapter11-dns-domain-resolve/study.md) |
| 12 | TCP 基础 | ARQ、**min(rwnd,cwnd)**、累积 ACK、[考点](chapter12-tcp-basic/study.md#ch12-exam) | [tcp_header.png](../top_down/03_transport_layer/assets/tcp_header.png) → [ch12](chapter12-tcp-basic/study.md) |
| 13 | TCP 连接 | 1.5 RTT、SYN Cookie、PMTUD/MSS、[考点](chapter13-tcp-connection-manage/study.md#ch13-exam) | TIME_WAIT、backlog → [ch13](chapter13-tcp-connection-manage/study.md) · [§3.1](../top_down/03_transport_layer/study.md#ch3-1-tcp-conn) |
| 14 | 超时重传 | Jacobson RTO、**3 dup ACK**、SACK、伪超时/Eifel、[考点](chapter14-tcp-timeout-retransmit/study.md#ch14-exam) | `ss -ti`、勿过小 RTO → [ch14](chapter14-tcp-timeout-retransmit/study.md) |
| 15 | 数据流/窗口 | **min(rwnd,cwnd)**、Nagle↔延迟ACK、Persist、SWS、[考点](chapter15-tcp-flow-window/study.md#ch15-exam) | `SetNoDelay(true)` → [ch15](chapter15-tcp-flow-window/study.md) |
| 16 | 拥塞控制 | AIMD、Reno/NewReno、**CUBIC**、ECN、[考点](chapter16-tcp-congestion-control/study.md#ch16-exam) | `tcp_congestion_control` → [ch16](chapter16-tcp-congestion-control/study.md) |
| 17 | 保活 | SEQ=NXT−1 探测、半开、NAT 静默断连、[考点](chapter17-tcp-keepalive/study.md#ch17-exam) | `SetKeepAlive` + 应用心跳 → [ch17](chapter17-tcp-keepalive/study.md) |
| 18 | 安全 | CIA、TLS1.3/IPsec、DNSSEC、PFS、[考点](chapter18-network-security/study.md#ch18-exam) | `rustls`/`crypto/tls` → [ch18](chapter18-network-security/study.md) |

## 五条易混（背）

1. **rwnd**（接收方） vs **cwnd**（网络）→ 发送 `min(rwnd, cwnd)`  
2. **TCP 分段**（MSS） vs **IP 分片**（MTU）→ UDP 大包怕后者  
3. **UDP 分用**：目的端口；**TCP**：四元组  
4. **TIME_WAIT**：主动关闭方、约 **2MSL**  
5. 第 2 版 **无** Telnet/FTP 章 → 应用协议看自顶向下第 2 章 + 工程实践

## 推荐学习顺序（后端）

`1→2→10→13→14→15→16` → 并行 [03_transport_layer/study.md](../top_down/03_transport_layer/study.md) → `5→7→11→18`
