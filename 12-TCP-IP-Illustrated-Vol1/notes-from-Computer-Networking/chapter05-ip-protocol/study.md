# 第 5 章：Internet 协议（IP）

> 按书节速记：[5.1](5.1-introduction.md) · [5.2](5.2-ipv4-header.md) · [5.3](5.3-ipv6-extension-headers.md) · [5.4](5.4-ip-routing-basic.md) · [5.5](5.5-mobile-ip-basic.md) · [5.6](5.6-host-ip-processing.md) · [5.7](5.7-ip-attacks.md) · [5.8](5.8-summary.md) · [QUICKREF §5](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/03_network_layer/ch05_ip.md)）  
> 地址结构：[ch02](../chapter02-ip-address-architecture/study.md) · L2 交付：[ch04 ARP](../chapter04-arp-protocol/study.md) · 自顶向下：[§4.3 IPv4/IPv6](../../top_down/04_network_layer_data_plane/study.md#ch4-3)

在**沙漏模型**中，**IPv4/IPv6** 处于腰部：核心**极简**，可靠性/流控/拥塞推向**端系统（传输层）** — 与 [ch01 端到端](../chapter01-overview/study.md#ch01-e2e)、**命运共享**一致。

---

<a id="ch05-1"></a>

## 5.1 引言

→ 精读：[5.1-introduction.md](5.1-introduction.md)

### IP 的核心地位

- **TCP/IP 网络层核心**；跨网数据**必经 IP**，上承 TCP/UDP，下接以太网/Wi‑Fi 等链路。
- **细腰**：统一 **IP 寻址** + **逐跳转发**；路由器只看 IP 头，异构网络互通。

### 服务语义（三大特性）

| 特性 | 要点 |
|------|------|
| **无连接** | 无 per-flow 状态；每报文**独立选路**（明信片类比） |
| **尽力而为** | **不保证**送达、顺序、无重复 |
| **出错/拥塞** | 常**直接丢弃**；可能 **ICMP** 反馈，不保证回 |

### IP vs TCP 分工（端到端）

| | IP（L3） | TCP（L4） |
|--|----------|-----------|
| 管什么 | **哪台机器** | **哪个进程 + 可靠** |
| 机制 | 寻址、路由、分片 | 确认重传、排序、拥塞/流量控制 |
| 模型 | 无连接、尽力而为 | 面向连接、端到端 |

**可扩展性**：复杂逻辑在**端系统**；路由器只做简单转发 → [ch01 端到端](../chapter01-overview/study.md#ch01-e2e)。

### 本章脉络

IPv4/IPv6 首部 · 扩展头链 · **LPM 转发** · Mobile IP · 主机处理模型 · **IP 相关攻击**

---

<a id="ch05-2"></a>

## 5.2 IPv4 与 IPv6 头部

→ 精读：[5.2-ipv4-header.md](5.2-ipv4-header.md)

### 整体差异

| | IPv4 | IPv6 |
|--|------|------|
| 头长 | **可变** 20–60 B | **固定 40 B** |
| 设计 | 选项内嵌主头；路由器可分片；每跳校验和 | 扩展头按需；**路由器不分片**；无首部校验和 |

### IPv4 必记字段

| 字段 | 要点 |
|------|------|
| **TTL** | 每跳 −1；v6 → **Hop Limit** |
| **DS+ECN** | DSCP + 显式拥塞通知 |
| **头部校验和** | 仅 IP 头；**每跳重算** |
| **标识/DF/MF/偏移** | **网络层分片**；DF=1 禁止分片 |

### IPv6 改造

- **砍掉**：首部校验和、路由器分片、主头分片字段  
- **新增**：**Flow Label**（QoS/快速转发）  
- **迁移**：选项 → [5.3 扩展头链](5.3-ipv6-extension-headers.md#ch05-3-chain)

### 字段对照表

| 字段 | IPv4 | IPv6 | 架构意义 |
|------|------|------|----------|
| 版本 | 4 (0100) | 6 (0110) | 版本识别 |
| 头长 | **IHL**（32-bit 字） | 固定 40B | 加速解析 |
| QoS | **ToS** | **Traffic Class** | 分级标记 |
| 流标签 | 无 | **20 bit Flow Label** | 标识实时流（可选快速路径） |
| 长度 | **Total Length**（含头） | **Payload Length**（不含头） | v6 简化计算 |
| 寿命 | **TTL** | **Hop Limit** | 防环路 |
| 上层协议 | **Protocol**（6=TCP） | **Next Header** | 分用 → [5.2 §Protocol](5.2-ipv4-header.md#ch05-2-protocol) |
| 首部校验和 | **16 bit，每跳更新** | **移除** | 硬件转发友好 |
| 地址 | 32 bit | 128 bit | 缓解枯竭 |

### 分片 vs 分段（必考）

| | IP 分片（L3） | TCP 分段（L4） |
|--|---------------|----------------|
| 谁做 | **路由器**（v4）/ 源（v6） | **端主机** |
| 依据 | **MTU** | **MSS** |

→ [5.2 §易混](5.2-ipv4-header.md#ch05-2-fragment-vs-segment) · [ch10](../chapter10-udp-ip-fragment/study.md)

### 背诵

1. v4：可变头、头校验和、路由器可分片。  
2. v6：40 B 固定、无头校验和、路由器不分片、流标签、扩展头。  
3. **分片=MTU/三层；分段=MSS/四层**。

### IPv4 首部（位图）

```text
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Version|  IHL  |Type of Service|          Total Length         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Identification        |Flags|      Fragment Offset    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Time to Live |    Protocol   |         Header Checksum       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Source IP Address                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Destination IP Address                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 易混

| 误区 | 事实 |
|------|------|
| IPv6 无 L3 校验和 → 更不可靠 | **以太网 FCS** + **TCP/UDP 校验**已覆盖 |
| 分片 = 分段 | **层级、触发方、依据**均不同 |

→ 分片与 MTU：[ch03 §MTU](../chapter03-link-layer/study.md#ch03-8)

---

<a id="ch05-3"></a>

## 5.3 IPv6 扩展头部

→ 精读：[5.3-ipv6-extension-headers.md](5.3-ipv6-extension-headers.md) · [设计思想](5.3-ipv6-extension-headers.md#ch05-3-design) · [Next Header 链](5.3-ipv6-extension-headers.md#ch05-3-chain)

弃用 IPv4 **Options** → **扩展头菊花链**：中间节点多数只读**固定 40 B 主头**；附加功能**按需**挂载。

| 扩展头 | NH | 谁解析 |
|--------|-----|--------|
| **逐跳选项** | **0** | **每台**路由器（唯一） |
| **路由头** | 43 | 路径节点 |
| **分片头** | 44 | **仅源**分片；路由器**不分片** |
| **目标选项** | 60 | **仅目的**主机 |
| **ESP/AH** | 50/51 | IPsec 安全 |

**Next Header**：每个头部末尾字段，指向下一个扩展头 / 上层协议（TCP/UDP/ICMPv6）。

**易混**：IPv4 头可变+路由器可分片；IPv6 主头固定+功能下沉扩展头+**仅源分片**。

→ 速记：[5.3 §考点](5.3-ipv6-extension-headers.md#ch05-3-cheat) · 对比 [5.2](5.2-ipv4-header.md#ch05-2-overview)

---

<a id="ch05-4"></a>

## 5.4 IP 转发

→ 精读：[5.4](5.4-ip-routing-basic.md) · [AS 边界 IGP/BGP](5.4-ip-routing-basic.md#ch05-4-as-boundary) · [AS100→AS200](5.4-ip-routing-basic.md#ch05-4-full-chain) · [背诵](5.4-ip-routing-basic.md#ch05-4-cheat)

**AS 内 IGP、AS 门口 BGP**；终端侧 **ARP** 只到网关/每跳直连段。

### 控制面 vs 数据面

| | 转发 (Forwarding) | 路由 (Routing) |
|--|-------------------|----------------|
| 平面 | **数据面** | **控制面** |
| 时标 | 纳秒级查表 | 建表/更新（协议） |
| 对象 | **FIB** | 路由协议 → 生成 FIB |

互联网稳定于**逐跳（Hop-by-Hop）**：每跳只需**下一跳**，无需全局路径。

### 术语

- **FIB（转发表）**：由路由计算得到的快速检索表  
- **LPM（最长前缀匹配）**：多条匹配时选**掩码最长**者

### 三步

1. 取**目的 IP**，查 FIB  
2. **LPM** 选最优前缀  
3. **交付**  
   - **直接交付**：目的在本地链路 → **ARP/ND** → 发帧  
   - **间接交付**：发往 FIB 中的**下一跳**路由器

### 例题（本书 5.4.3 型）

| 前缀 | 出接口 | 下一跳 |
|------|--------|--------|
| 128.32.1.0/24 | eth0 | Direct |
| 128.32.0.0/16 | eth1 | 128.32.2.1 |
| 0.0.0.0/0 | eth1 | 128.32.2.1 |

| 目的 IP | 匹配 | LPM 结果 |
|---------|------|----------|
| 128.32.1.14 | /24 与 /16 | **eth0**（/24 更长） |
| 128.32.2.10 | 仅 /16 | eth1 → 128.32.2.1 |
| 1.1.1.1 | 默认路由 | eth1 → 128.32.2.1 |

→ 自顶向下：[§4.2 路由器](../../top_down/04_network_layer_data_plane/study.md#ch4-2) · [ch04 直接/间接交付](../chapter04-arp-protocol/study.md#ch04-2)

---

<a id="ch05-5"></a>

## 5.5 移动 IP（Mobile IP）

→ 精读：[5.5-mobile-ip-basic.md](5.5-mobile-ip-basic.md) · [**住宅IP≠HoA**](5.5-mobile-ip-basic.md#ch05-5-residential) · [在家/外地](5.5-mobile-ip-basic.md#ch05-5-home-away) · [HoA 绑定](5.5-mobile-ip-basic.md#ch05-5-hoa-bind) · [蜂窝实例](5.5-mobile-ip-basic.md#ch05-5-cellular)

### 住宅 IP ≠ 家乡地址 HoA（必先分清）

| | **住宅 IP（日常）** | **HoA（移动 IP）** |
|---|---------------------|---------------------|
| 体系 | 家宽**公网 IP 类型** | **MN 永久逻辑身份** |
| 关系 | 与 Mobile IP **无关** | MN+HA+CoA 协议核心 |

**住宅 IP** = 运营商分给家宽的公网出口；**HoA** = 设备对外不变的「身份证号」，可内网可公网，≠ 家宽 IP。

设备跨网漫游时，**上层仍用家乡地址** — **间接寻址（indirection）** + **IP 隧道**。

| 角色 | 作用 |
|------|------|
| **MN** | **移动设备本身**；逻辑上用**家乡地址**（如 `10.0.0.2`） |
| **HA** | **注册 + 转接**；通常**一区域/一运营商一台**，管全网归属 MN |
| **CoA** | **同一 MN** 在外地的临时 IP（如 `20.1.1.3`），非另一台主机 |

**在家 vs 外地**：在家 **无 CoA**，HA 当普通网关，**直连**；外出注册 CoA → HA 映射 `10.0.0.2→CoA` → **隧道**转发。

**实例**：湖北卡出省 ≈ MN+HA+CoA；**HoA 首次在家绑定、漫游不变**；全国通用流量统一扣费。

**三句**：① 首次家乡入网分配 **HoA** 写网卡，卡不变则不变 · ② 漫游只加 **CoA**，APP 仍见 HoA · ③ **系统绑定**，非 APP 记地址。

**现代**：LTE / **PMIPv6**；本节为**基础概念模型**。

→ 速记：[5.5 §考点](5.5-mobile-ip-basic.md#ch05-5-cheat) · 易混：Wi-Fi **L2** 漫游 vs Mobile IP **L3**

---

<a id="ch05-6"></a>

## 5.6 主机对 IP 数据报的处理

→ 精读：[5.6-host-ip-processing.md](5.6-host-ip-processing.md) · [强/弱主机](5.6-host-ip-processing.md#ch05-6-strong-weak) · [RFC 6724](5.6-host-ip-processing.md#ch05-6-rfc6724)

多网卡（**Multihoming**）/ VPN / 环回：收发是否**绑定接口**（RFC 1122）。

| 模式 | 接收 | 发送 | 默认 |
|------|------|------|------|
| **强主机** | 目的 IP = **接收接口** IP | 源 IP = **出站接口** IP | **Linux / Win7+ / IPv6** |
| **弱主机** | 目的 IP ∈ **本机任一**接口 | 源 IP ∈ 本机即可 | Win XP/2003（IPv4） |

**IPv6 源地址选择（RFC 6724）**：同址 → 范围 → 非废弃 → … → **最长前缀**（共 8 条）；开发**必 bind** 源 IP/接口。

→ 速记：[5.6 §考点](5.6-host-ip-processing.md#ch05-6-cheat) · 一页纸：[#ch05-6-onpager](#ch05-6-onpager)

---

<a id="ch05-6-onpager"></a>

## 5.6 一页纸：强/弱主机 + RFC 6724

| 主题 | 一句 | 易错 |
|------|------|------|
| **强主机** | 收发都**绑定接口 IP**；安全 | **现代 Linux/IPv6 默认** |
| **弱主机** | 本机**任一**接口 IP 即可收发 | Win XP；易**跨接口欺骗** |
| **RFC 6724 源选** | 1 同址 2 范围 3 非废弃 … 8 **最长前缀** | 多地址须 **bind** 或策略路由 |
| **多网卡/VPN** | 强主机：**接口–IP 不匹配即丢包** | 勿假设内核自动选对源 |
| **开发** | Go `Dialer.LocalAddr`；`ip route get` 验证 | 跨接口服务**必须显式**绑源 |

---

<a id="ch05-7"></a>

## 5.7 与 IP 相关的攻击

→ 精读：[5.7-ip-attacks.md](5.7-ip-attacks.md) · [IP 欺骗](5.7-ip-attacks.md#ch05-7-spoof) · [分片攻击](5.7-ip-attacks.md#ch05-7-fragment)

IP 设计于互信环境 → **源地址无内置认证**。

| 攻击 | 原理 | 防护 |
|------|------|------|
| **IP 欺骗** | 伪造**源 IP** → 白名单绕过、**反射 DDoS** | **BCP 38** 入站过滤、**uRPF**（严格/松散） |
| **分片攻击** | **畸形/重叠分片** → 重组崩溃（**Teardrop**） | 边界滤片、栈已加固、**PMTUD** 减分片 |

**原则**：IP 无身份/加密 → **TLS / IPsec** + 运维策略；**勿依赖源 IP**。

**联动**：常与 [ch04 ARP 欺骗](../chapter04-arp-protocol/4.11-arp-spoof-defense.md) 组合（L2+L3）。

→ 速记：[5.7 §考点](5.7-ip-attacks.md#ch05-7-cheat) · [ch02 §2.8](../chapter02-ip-address-architecture/2.8-address-security-threat.md)

---

<a id="ch05-exam"></a>

## 5.8 总结与考点

**核心**：用**极简 IP** 承载**极其复杂**的全球互联；IPv4 奠基，IPv6 面向**硬件线速**（固定头、无 L3 校验和、扩展头链、**仅源端分片**）。

### 复盘速记

| 主题 | 一句话 |
|------|--------|
| 服务模型 | 不可靠、无连接、尽力而为 |
| v4 vs v6 头 | 变长+校验和 vs 40B 固定+无首部校验和 |
| v6 分片 | **路由器不分片** → Packet Too Big |
| 转发 | **LPM** + 直接/间接交付 |
| Mobile IP | **MN+HA+CoA**；隧道+indirection；**住宅IP≠HoA** → [5.5 §零](5.5-mobile-ip-basic.md#ch05-5-residential) · [§考点](5.5-mobile-ip-basic.md#ch05-5-cheat) |
| 主机模型 | **强**=接口绑定（Linux/IPv6 默认）；**弱**=本机全局 → [5.6](5.6-host-ip-processing.md#ch05-6-strong-weak) |
| IPv6 源选 | RFC **6724** 八条；开发 **bind** → [5.6](5.6-host-ip-processing.md#ch05-6-rfc6724) |
| 安全 | 源 IP 不可信；**BCP38/uRPF**；分片 **Teardrop** → [5.7](5.7-ip-attacks.md#ch05-7-cheat) |

### 建议后续章节

- [ch06 DHCP](../chapter06-dhcp-config/study.md) — 地址动态分配  
- [ch08 ICMP](../chapter08-icmpv4-icmpv6/study.md) — TTL 超时、PMTUD、Packet Too Big  
- [ch10 UDP](../chapter10-udp-ip-fragment/study.md) — 端到端分片与 MTU  
- [ch07 NAT/防火墙](../chapter07-firewall-nat/study.md) — 地址转换与边界

---

## Top-Down

- [04_network_layer_data_plane/study.md §4.1–4.3](../../top_down/04_network_layer_data_plane/study.md#ch4-3)

## Lab

- Wireshark：IPv4 **TTL** 逐跳递减；IPv6 **Hop Limit**  
- `traceroute` 与 **ICMP Time Exceeded**  
- 对比 IPv4 **Identification/Fragment** 与 IPv6 **仅源分片**

## Go / Rust

- **Go**：`net.IP` / `net.IPNet`；`ipv4`/`ipv6` 包设 **TTL**、`Don't Fragment`（PMTUD）  
- **Rust**：`pnet` 解析首部；**强主机**排障时注意多网卡入站策略  
- **实践**：发送路径 `ip route get`；避免 UDP 触发 **IP 分片**（见 ch10）
