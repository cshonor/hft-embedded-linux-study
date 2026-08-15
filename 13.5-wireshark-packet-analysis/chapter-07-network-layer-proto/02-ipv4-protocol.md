# 7.2.1 互联网协议第 4 版（IPv4）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 对照：[TCP/IP 第2章 地址](../../TCP-IP-Volume1-Protocols/chapter02-ip-address-architecture/chapter-summary.md)

**核心主旨**：IPv4 寻址/掩码、头部关键字段、MTU 分片——读包与排障基础。

## 核心知识点

### 寻址与子网

| 项 | 说明 |
|----|------|
| 长度 | **32 位** → 点分十进制（`192.168.0.1`） |
| 组成 | **网络位** + **主机位**；边界由 **子网掩码** 定（1=网络，0=主机） |
| CIDR | `192.168.1.0/24` 简写前缀长度 |

### IPv4 头部关键字段（抓包必看）

| 字段 | 作用 |
|------|------|
| **TTL** | 每经一跳路由器 **减 1**；为 0 则丢弃并常回 **ICMP Time Exceeded** → 防路由环路 DoS |
| **Protocol** | 上层协议号（6=TCP，17=UDP，1=ICMP） |
| **Identification** | 分片归属同一原包 |
| **Flags / Fragment offset** | 是否还有更多片、片偏移 |
| **Total Length** | 本 IP 包长度 |
| **Header Checksum** | 头部校验 |

### IP 分片（Fragmentation）

| 触发 | 包长 > 出站链路 **MTU**（以太网常 **1500**） |
|------|---------------------------------------------|
| **More Fragments** | 除最后一片外均为 1 |
| **Fragment offset** | 以 8 字节为单位（如 0、185、370… 对应字节偏移 0、1480、2960…） |
| **Identification** | 各片相同，接收端重组 |

**Wireshark 提示**：`ip.flags.mf==1` 找分片；`ip.frag_offset > 0` 找后续片；专家信息可能标 **Fragmented**。

> 路径 MTU 发现（PMTUD）可减少中间分片；见 TCP/IP [MTU/PMTUD](../../TCP-IP-Volume1-Protocols/chapter03-link-layer/3.8-mtu.md)。

## 抓包/实操记录

| 实验 | 过滤器 / 操作 |
|------|----------------|
| 只看 IPv4 | `ip` |
| 某主机 | `ip.addr == 192.168.1.10` |
| TTL 递减 | traceroute 抓包对比每跳 `ip.ttl` |
| 分片 | `ip.frag_offset` 或 ping 大包 `-f`（DF）测 MTU |

展开 **Internet Protocol Version 4** 树对照字段与 [Packet Bytes](../chapter-03-wireshark-intro/04-get-started.md)。

## 疑问与总结

- **私有地址** 不可路由公网；NAT 改变可见 IP（抓包点决定看到哪一侧）。
- 分片增加丢包面；现代网络倾向 **DF + PMTUD** 由源端发合适大小。
