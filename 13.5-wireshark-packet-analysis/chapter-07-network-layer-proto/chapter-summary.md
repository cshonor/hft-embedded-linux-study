# 第7章 网络层协议

> 全书：[../README.md](../README.md) · 上一章：[第6章 命令行](../chapter-06-tshark-tcpdump/chapter-summary.md)

## 整体框架

```text
7.1 ARP（Request 广播 · Reply 单播 · Gratuitous）
        ↓
7.2 IPv4（TTL · 分片） / IPv6（NDP · 主机分片 · 扩展头）
        ↓
7.3 ICMP / ICMPv6（Ping · Traceroute · 报错 · NDP/PMTUD）
```

| 小节 | 文件 |
|------|------|
| 7.1 ARP | [01-arp-protocol.md](./01-arp-protocol.md) |
| 7.2.1 IPv4 | [02-ipv4-protocol.md](./02-ipv4-protocol.md) |
| 7.2.2 IPv6 | [03-ipv6-protocol.md](./03-ipv6-protocol.md) |
| 7.3 ICMP | [04-icmp-protocol.md](./04-icmp-protocol.md) |

## 重点难点

| 点 | Wireshark 提示 |
|----|----------------|
| ARP Request | 以太网 DA 全 F；Target MAC 全 0 |
| Gratuitous ARP | Sender IP = Target IP |
| TTL | 每跳减 1；与 Traceroute、ICMP 11 联动 |
| IP 分片 | ID + offset + MF；IPv6 路由器不分片 |
| NDP | `icmpv6` 135/136，非 `arp` |
| Ping | ICMP 8/0；防火墙可导致无 Reply |
| Traceroute | TTL 递增 + Time Exceeded |

## 实操要点

1. 同网段 `ping` 先抓 `arp` 再抓 `icmp`。
2. 过滤器：`arp` · `ip` · `ipv6` · `icmp` · `icmpv6`。
3. 对照 [cheatsheet](../cheatsheet/notes.md) 与 TCP/IP 第 2 章地址笔记。
4. 安全场景：异常 Gratuitous ARP → 对照 [§2.3.4 ARP 污染](../chapter-02-traffic-monitor/03-sniff-on-switched-network.md)。

## 小节索引

- [7.1 地址解析协议](./01-arp-protocol.md)
- [7.2.1 IPv4](./02-ipv4-protocol.md)
- [7.2.2 IPv6](./03-ipv6-protocol.md)
- [7.3 ICMP / ICMPv6](./04-icmp-protocol.md)
