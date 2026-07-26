# 第 10 章：用户数据报协议（UDP）与 IP 分片

> 按书节速记：[10.1](10.1-introduction.md) · [10.2](10.2-udp-header-format.md) · [10.3](10.3-udp-checksum.md) · [10.4](10.4-udp-dns-example.md) · [10.5](10.5-udp-ipv6-teredo.md) · [10.6](10.6-udp-lite.md) · [10.7](10.7-ip-fragment-mechanism.md) · [10.8](10.8-udp-pmtud.md) · [10.9](10.9-fragment-arp-nd.md) · [10.10](10.10-udp-datagram-length.md) · [10.11](10.11-udp-server-design.md) · [10.12](10.12-udp-protocol-translation.md) · [10.13](10.13-udp-typical-application.md) · [10.14](10.14-udp-security.md) · [10.15](10.15-summary.md) · [QUICKREF §10](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/04_transport_layer/ch10_udp.md)）  
> 前置：**IP/MSS/PMTUD** — [ch05 IP](../chapter05-ip-protocol/study.md) · **链路 MTU** — [ch03 链路层](../chapter03-link-layer/study.md) · **PTB** — [ch08 ICMP](../chapter08-icmpv4-icmpv6/study.md)  
> **组播**：[ch09](../chapter09-broadcast-multicast/study.md) · 自顶向下：[§3.3 UDP](../../top_down/03_transport_layer/study.md#ch3-3)

UDP 不提供可靠传输、不重排、不重传——它把 **端到端语义**裁剪到「**端口多路复用** + （可选）**数据完整性覆盖**」，从而成为 **DNS、SNMP、流媒体、VXLAN、QUIC 底层运载**的共同起点。本章后半说明：**一旦上层一次交付的字节序列超过链路 MTU，IPv4 会在中间路由器分片**；任一丢失都会让整「逻辑报文」在接收端被丢弃——这是 UDP 互联网上**隐藏的脆弱点**。[ch03](../chapter03-link-layer/study.md) MTU、[ch08](../chapter08-icmpv4-icmpv6/study.md) `PTB` 与本章一起记。

---

<a id="ch10-1"></a>

## 10.1 引言：**无连接 datagram**

| 语义 | UDP 的承诺 | 隐含成本 |
|------|------------|-----------|
| **无连接状态** | 无握手、`connect`/`bind`仅为本地 API | 防火墙/NAT 只能凭五元组**猜**会话 |
| **保留报文边界** | **一次写入** ⇒ **一则 IP 载荷中的 UDP datagram** | 缓冲区小于包长时需 `MSG_TRUNC` 或丢包[^1] |
| **不可靠、可能乱序** | 照搬 IP：`ECMP`、`队列丢失`都会导致乱序或丢片 | QUIC 等在 UDP 头上重建可靠性 |

[^1]: 《详解》对不同 OS 的实现差异给出了 BSD vs Windows vs Linux 的注意点——工程上应尽量用**已知最大报长度**缓冲区或应用层分包。

UDP 不改变 IP TTL、ToS、`Don't Fragment`，这些仍由下层套接字/内核策略控制 **[ch05](../chapter05-ip-protocol/study.md)**。

---

<a id="ch10-2"></a>

## 10.2 UDP 首部与长度

![UDP Header](../../top_down/03_transport_layer/assets/udp_header_fields.png)

UDP 首部 **固定 8 字节**。字段表（建议与 Wireshark 「User Datagram Protocol」一起看）：

| 字段（英文惯用名） | 位宽 | 取值 / 语义 | 实务 |
|-------------------|------|---------------|------|
| **Source Port** | **16** | 发送方可选；若不需要回音可 **`0`** | `netstat`、`ss -u` |
| **Destination Port** | **16** | **解复用的键**：决定哪个 socket 收下 | ACL、SDN Match |
| **Length**（UDP Datagram Length） | **16** | **首部 + 数据** 字节总数；理论上最大 **65535** | IPv4+jumbo frame 特例需考虑 |
| **Checksum** | **16** | 覆盖首部、数据、伪首部（见 **[§10.3](#ch10-3)**） | IPv4：**可以全 0 表示跳过** |

**总长关系**：上层一次交付 `L_data` ⇒ UDP Length = `8 + L_data`。随后 IP Total Length ≥ `UDP Length + IPv4_Header`（或无 jumbo）。

---

<a id="ch10-3"></a>

## 10.3–10.4 端到端论点与 **校验和**：伪首部

UDP 首部 Checksum **含伪首部 Pseudo Header**，把 **源/目的 IP、协议字段、UDP 长度**并入计算，从而在「IP 首部没有自身校验和」（IPv6）或「路由重写地址」场景中仍有机会侦测错投。

```
┌ Pseudo Header ──────────────────────────┐  ← 不参与链路上真实发送  
│ Src IP (32 / 128)                       │
│ Dst IP (32 / 128)                       │
│ Zero (8 bits) │ Proto (8)=17 │ UDP Len │
└ Pseudo Header ──────────────────────────┘
        + UDP header + Payload  → one's complement sum → Checksum field
```

图参考：[udp_header_pseudo.png](../../top_down/03_transport_layer/assets/udp_header_pseudo.png)

### IPv4 **vs IPv6**：伪首部结构与「是否允许关闭校验和」

| 项目 | **IPv4** | **IPv6** |
|------|----------|-----------|
| **IP 首部自身校验和** | 有 **[ch05](../chapter05-ip-protocol/study.md)** | **无**：错误检测责任外溢到上层 / 链路 |
| UDP Checksum **`0`** 语义 | **`0`** 常量表示 *not computed*「发送方跳过」 | **`0`** 作为计算结果时需编码为 **`0xffff`**；**必须为有效值**并由接收方核验 |
| 典型工程结论 | 许多栈仍计算 UDP checksum | **必须开启** IPv6 UDP checksum；否则会丢包或被中间件扔掉 |

校验和仍为 **端到端**：中间路由器**一般不重算** UDP checksum（NAT 重写地址时需特殊处理）[ch07](../chapter07-firewall-nat/study.md)。

---

<a id="ch10-4"></a>

## 10.5 UDP-Lite（RFC 3828）

**UDP-Lite** 允许只对 **前缀 N 字节**做校验：**语音/视频会议**可以接受「后部若干字节损坏」，但无法忍受整包被判无效。网络设备若不理解该协议需在部署前确认。**考点**：识别「部分覆盖」语义与音视频延迟取舍。

---

<a id="ch10-5"></a>

## 10.6 IPv4 **分片**字段与路径 MTU

当 **IPv4 DF=0**，且 **`Total Length` > egress MTU** 时，路由器或主机把 IP 载荷切成多段，每段外挂自己的 IPv4 Header。

| IPv4 Header 字段（复习） | 作用 |
|--------------------------|------|
| **Identification**（16-bit） | 同一「未分片原始报文」拷贝到各片上；接收端重组拼接键的一部分 |
| **Flags** — **DF**, **MF** | **Don't Fragment**：若超限则 ICMP **Type 3 Code 4 Fragmentation Needed**（PTB）[ch08](../chapter08-icmpv4-icmpv6/study.md)；**MF=1**：还有后续片 |
| **Fragment Offset**（13-bit）×8 | **8 字节粒度**对齐；首片偏移 `0`，第二片常为 `ceil(first_payload / 8)` |

**IPv6**：中间路由器 **不分片**；仅源端可把扩展头里做 **分段扩展**（或由上层避免） **[ch05](../chapter05-ip-protocol/study.md)**。

### 考点链：**TCP MSS** vs **IP Fragmentation**

TCP 通过在握手阶段获知 **路径 MSS** ⇒ 尽量让每个 **TCP Segment + IP/TCP Header** 不经 IP 层再切；UDP 没有这个默认协调 ⇒ **单次 write 超长 ⇒ IP 被动分片 ⇒ 任一一片丢失 ⇒ 上位应用看到整 UDP 丢失**——「分片丢失放大」效应。

---

<a id="ch10-6"></a>

## 10.7 **分片算例**：总 IPv4 **4020 B**、链路 **MTU=1500**（三片）

> 条件：外层 IPv4 Header **固定 20 B** (`IHL=5`，无选项)；MTU **指链路允许的最大帧净荷中的 IP datagram**，故每片 **`1500`** 总长 ⇒ **载荷 ≤ 1480 B**。**Payload 必须为 8 的倍数**，1480 ✅。

设某主机发出 **总长 4020 B** IPv4 Datagram：**20 B IPv4 HDR + `4000` B UDP 整块（UDP 8 + 应用 3992）** ⇒ 与用户指定 **4020** 对上。

**分解**：IP 载荷 `4000 B` = `1480 + 1480 + 1040`（三段均为 8 的倍数）。

| # | MF | Fragment Offset ×8 （十进制载荷起点） | 本片的 **IP 载荷** 字节 |
|---|-----|---------------------------------------|---------------------------|
| 1 | **`1`** | **0** | **1480**（含 **UDP 首部 8 B** + **应用数据 1472 B**）[^2] |
| 2 | **`1`** | **`185`** ⇒ 起点 `1480` | **1480** |
| 3 | **`0`** | **`370`** ⇒ 起点 `2960` | **1040**（剩余 UDP/应用：`4000 − 2960`） |

各片链路帧长：`1500 + 1500 + 1060 = 4060 B`（总长增加来自重复 IPv4 Header ×3）。

[^2]: 首片中 **偏移 0** 的那一个 **UDP header 仅出现一次**：后续片不包含 UDP header 副本，`Offset` >0 的包从 UDP payload 的中间字节开始装载。

考点：**偏移用 8B 粒度** ⇒ `1480÷8 = 185`，`2960÷8 = 370`。若算出第三片载荷不是 **1040** 则算术错误——常见笔误写成 `1032` 来自把「UDP 总长」误认为 **3992 IP 载荷**。

---

<a id="ch10-7"></a>

## 10.8 **ARP / ND 陷阱**（《详解》强调的 race）

超长 UDP ⇒ **多分片**。若内核在发送第一片时才首次解析邻居：

1. ARP **[ch04](../chapter04-arp-protocol/study.md)** / ND **[ch08](../chapter08-icmpv4-icmpv6/study.md)** Pending 队列只短暂缓存少数后续片；
2. **后发片早于 ARP Reply** 到期 ⇒ **悄悄丢弃** ⇒ 远端永远收不齐、`Missing fragment` ⇒ 上位应用误判「网络极差」；

**Mitigation**：预先用 `ping`/`connect`/`ND` warmup；或 **永远不要接近 MTU**：应用层分包。

---

<a id="ch10-8"></a>

## 10.9 服务器实现注意（简述）

《详解》列出的端口 `0`、`IPV6_V6ONLY`、缓冲区、`EADDRINUSE`、`weak end system`/`strong end system`、`IP_RECVERR`/`IP_RECVDSTADDR`/`IP_PKTINFO`、`UDP` 校验和卸载等在现代 Linux/BSD/Fuchsia 仍存在 — 本节只抓面试要点：

| 主题 | Do / Don't |
|------|------------|
| **多宿主 `INADDR_ANY`** | ACK/响应源地址未必等于客户端看见的 dest IP ⇒ 对称路由问题 |
| **接收缓冲太小** | 高速小包风暴 ⇒ 常见于不同 OS：**截断、`EAGAIN`、`ENOBUFS`、静默丢包** |
| **组播/广播** **[ch09](../chapter09-broadcast-multicast/study.md)** | **`SO_REUSEADDR`/`SO_REUSEPORT`**、出站接口、`IP_MULTICAST_TTL`、`IP multicast loopback` |
| **`connect` UDP** | 「伪连接」仅固定五元组、方便 `recv`/ICMP 报错投递 |

---

<a id="ch10-9"></a>

## 10.10 安全面

UDP（无 handshake） ⇒ **易被伪造**。常见攻击范式：

| 攻击 | 说明 | Mitigation |
|------|------|------------|
| **反射 / 放大 DDoS** | 以小查询换大应答（历史上的 Chargen、SNMP、Memcached） | ACL、限速、`<100B` ⇒ close、BCP38 源校验 |
| **分片重装消耗**（Teardrop 变体、「最后一片永远不出现」） | 占满重装缓冲 ⇒ OOM/`nf_conntrack` | 入口滤片、补丁、限制并发重装 |
| **Checksum offloading 与 spoofed inner** | SMARTNIC/TOE 可把坏包误判 | 开启硬件校验卸载与驱动一致性测试 |

[ch05 IPv4 spoofing](../chapter05-ip-protocol/study.md)；应用上的加密与认证见自顶向下 [§8 网络安全](../../top_down/08_network_security/study.md)。

---

<a id="ch10-exam"></a>

## 10.11 考点总结

### 一页易混对照

| 问题 | Must Know |
|------|-----------|
| **UDP vs TCP 边界语义** | **Datagram ↔ 字节流** |
| **`UDP Length`=`0`** | 理论上合法但几乎不用；别把 **IPv4 Padding**算进 UDP Len |
| **IPv6 UDP checksum=`0`** | **非法**：必须改写为 **`0xffff` 存入**或以实现策略拒绝发包 |
| **IP 片中 UDP header 在哪儿** | **仅首片偏移 0；后续片是纯 payload 后缀** |
| **丢任意一片的后果** | 重组定时器耗尽 ⇒ **整块上层 datagram** 当作丢失 |
| **DF=1 oversized** ⇒ | **ICMP PTB**，PMTUD 起点 **[ch08](../chapter08-icmpv4-icmpv6/study.md)** |
| **`1400`** 传说 | VLAN/QoS/GRE/GENEVE 可把有效 MTU 砍到 **`1500 − 开销`** ⇒ 保守荷载 |

### 推荐包尺度（备忘）

| 场景 | Payload 建议 |
|------|----------------|
| 互联网 UDP DNS/音视频 | **`≤1200`**（穿越 PPPoE、隧道）到 **`≤1400`** |
| Datacenter Overlay | **`path MTU` 探测**、`DF` + QUIC PMTUD |

### 延伸阅读

| 跳转 | |
|------|---|
| 下一章TCP | （卷一随后章节）序号/三次握手、[study §3 TCP](../../top_down/03_transport_layer/study.md) |
| 组播 UDP | **[ch09](../chapter09-broadcast-multicast/study.md)** |
| ICMP + ND | **[ch08](../chapter08-icmpv4-icmpv6/study.md)** |

---

## Top-Down

- **[study.md §3.3 UDP](../../top_down/03_transport_layer/study.md#ch3-3)**（无连接语义、广播/组播）[· §3.3 考点](../../top_down/03_transport_layer/study.md#ch3-3-exam)
- **[§3.2 多路复用与解复用](../../top_down/03_transport_layer/study.md#ch3-2)**：UDP 用 **目的 IP + 目的端口** 键解复用 — 与 TCP 四元组对比

## Lab

- Wireshark：`udp && !icmp` vs `udp.length > 1472`，观察 **`ip.frag`**
- **`ping`** / **`hping`** + **DF**：验证 **PTB** 报文 **[ch08](../chapter08-icmpv4-icmpv6/study.md)**
- Linux：`ip -det link show mtu`、`sysctl net.ipv4.ipfrag_*`

## Go / Rust（工程钩子）

| 栈 | Hint |
|---|-----|
| **Go** `x/net/ipv4` | **`SetControlMessage`/`SetTTL`/`SetICMPFilter`**；大图 UDP 手写 **Path MTU 探测 loop** |
| **Rust QUIC** (`quinn`) | QUIC 在用户态切 **UDP datagram**；仍受 **下层 IP 限制** |
