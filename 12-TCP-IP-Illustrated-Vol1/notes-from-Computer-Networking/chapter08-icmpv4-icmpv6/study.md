# 第 8 章：ICMPv4 与 ICMPv6

> 按书节速记：[8.1](8.1-introduction.md) · [8.2](8.2-icmp-packet-format.md) · [8.3](8.3-icmp-error-messages.md) · [8.4](8.4-icmp-query-ping.md) · [8.5](8.5-ipv6-ndp.md) · [8.6](8.6-icmpv4-v6-translation.md) · [8.7](8.7-icmp-attacks.md) · [8.8](8.8-summary.md) · [QUICKREF §8](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/03_network_layer/ch08_icmpv4_icmpv6.md)）  
> 前置：[ch05 IP](../chapter05-ip-protocol/study.md) · [ch04 ARP](../chapter04-arp-protocol/study.md) · [ch06 SLAAC](../chapter06-dhcp-config/study.md#ch06-3) · [ch07 边界](../chapter07-firewall-nat/study.md)

**ICMP** 是 IP 的**反馈环路与诊断中枢**：IP **尽力而为、无内置纠错** → ICMP 报告异常、探测路径、（v6）邻居发现与多播管理。无 ICMP，丢包与配置错误多为**黑盒**。

---

<a id="ch08-1"></a>

## 8.1 引言与封装

### 战略地位

从简单差错反馈 → **IPv6 即插即用、移动性、多播**的核心支撑。

### 协议号与封装

| 版本 | IP 协议字段 | 说明 |
|------|-------------|------|
| **ICMPv4** | **1** | IPv4 附属协议 |
| **ICMPv6** | **58** | v6 中地位更高，整合 ARP/IGMP 职能 |

ICMP 作为 IP **载荷** 发送，依赖 IP 路由；生存受 **TTL/Hop Limit** 约束。栈解析 IP 后按协议号 **1/58** 分发给 ICMP 模块。

**分两步识别**（非以太网直接标 ICMP）：`EtherType 0x0800` → IPv4 → `Protocol=1` → ICMP → [8.1 §demux](8.1-introduction.md#ch08-1-demux) · [ch01](../chapter01-overview/study.md#ch01-layering-icmp)

### 公共首部（前 4 字节）

| 偏移 | 字段 | 职能 |
|------|------|------|
| 0 | **Type** | 差错 vs 查询等大类 |
| 1 | **Code** | 类型内细分原因 |
| 2–3 | **Checksum** | 覆盖整个 ICMP 报文 |

```text
[ IPv4/v6 首部 ] → [ Type | Code | Checksum | 类型相关字段 | 数据 ]
```

---

<a id="ch08-2"></a>

## 8.2 报文分类与处理规则

| 阵营 | 触发 | 典型用途 |
|------|------|----------|
| **差错报文** | 被动（传输出错） | 不可达、超时、参数错误 |
| **信息/查询报文** | 主动 | Ping、路由器发现、（v6）ND |

### 强制性处理准则

1. **防止差错风暴**：**ICMP 差错不再触发 ICMP 差错**；差错报文损坏则**静默丢弃**  
2. **源地址合法性**：原始 IP **源为单播**才发差错；对**多播/广播**源通常**不**回应，防放大 DoS  
3. **负载镜像**：差错须含**原 IP 首部 + 至少传输层前 8 字节**，便于定位 **Socket**

### ICMPv6 扩展

除 v4 核心功能外，承担原 **ARP**、**IGMP** 等职责 → 简化 v6 协议栈。

→ IPv4 ARP：[ch04](../chapter04-arp-protocol/study.md) · v6 用 ND 见 §8.5

---

<a id="ch08-3"></a>

## 8.3 差错报文深度解析

### 目的不可达

| | ICMPv4 | ICMPv6 |
|--|--------|--------|
| Type | **3** | **1** |

| Code | 含义（v4 常见） |
|------|-----------------|
| 0 | 网络不可达 |
| 1 | 主机不可达 |
| 2 | 协议不可达 |
| 3 | **端口不可达** |
| 4 | **需要分片但未设 DF** → **PMTUD 核心** |

**IPv6**：**Packet Too Big** 独立为 **Type 2**（不再塞进 Type 1 Code 4）。路由器**不分片** → 必须发 Type 2，否则 **PMTUD 黑洞**。

### 其他差错

| 报文 | v4 Type | 要点 |
|------|---------|------|
| **重定向** | 5 | 仅**路由器→直连主机**；路由器间禁用；安全上常全网关闭 |
| **超时** | 11 | TTL/Hop Limit=0；**Traceroute** 基础 |
| **参数问题** | 12 | IP 首部错误字节偏移 |

### 与 ch05 联动

- TTL 减至 0 → **Time Exceeded**：[ch05 §5.2](../chapter05-ip-protocol/study.md#ch05-2)  
- MTU 超限（v6）→ **Packet Too Big**：[ch03 MTU](../chapter03-link-layer/study.md#ch03-8)

---

<a id="ch08-4"></a>

## 8.4 查询/信息类报文

### Ping（Echo Request/Reply）

| 步骤 | 行为 |
|------|------|
| 1 | 发 **Echo Request**，填 **Identifier + 递增 Sequence** |
| 2 | 对端改 Type 为 **Echo Reply**，回显载荷 |
| 3 | 源端算 **RTT**、用序号缺口估 **丢包** |

v4：**Type 8/0**；v6：**Type 128/129**。

### 已式微的 v4 查询

**地址掩码请求**、**时间戳请求** 等在 v6 由 **ND + SLAAC** 取代 → [ch06](../chapter06-dhcp-config/study.md#ch06-3)

### 移动 IPv6（概念）

**移动前缀请求/通告** 等 ICMPv6 扩展，支撑归属前缀与转交地址刷新（与 [ch05 Mobile IP](../chapter05-ip-protocol/study.md#ch05-5) 呼应）。

---

<a id="ch08-5"></a>

## 8.5 IPv6 邻居发现（ND）

基于 **ICMPv6** 的 ND 取代广播式 **ARP**，并整合 **RS/RA**（SLAAC）。

### 核心报文

| 报文 | 作用 |
|------|------|
| **NS** (Neighbor Solicitation) | 解析 MAC（替代 ARP 请求） |
| **NA** (Neighbor Advertisement) | 应答 MAC |
| **RS** (Router Solicitation) | 主机请求前缀 |
| **RA** (Router Advertisement) | 路由器下发前缀/标志 |

**被请求节点多播** 替代全网广播 → 降低链路干扰，支撑大规模接入。

### 链路层地址选项（示意）

| 字段 | 说明 |
|------|------|
| Type | 1=源链路层地址，2=目标链路层地址 |
| Length | 以 8 字节为单位 |
| L-L Address | MAC 等 |

### DAD（重复地址检测）

1. 新地址先为 **Tentative（暂定）**，不收单播  
2. 发 **NS**，Target=暂定地址，源常为 **::**  
3. 收到 **NA** → **冲突**，禁用  
4. 超时无响应 → **Preferred（优选）**

### NUD（邻居不可达检测）

维护 **REACHABLE / STALE / …** 等状态 → `ip neigh` 所见。

→ 与 ch04：**IPv6 无 ARP，ND 在 ICMPv6 内**

---

<a id="ch08-6"></a>

## 8.6 ICMPv4 与 ICMPv6 转换（SIIT）

双栈/翻译网关需**类型/代码映射** + **校验和重算**。

| ICMPv4 | → ICMPv6 | 语义 |
|--------|----------|------|
| 3/0 网络不可达 | 1/0 | 路由无目的 |
| 3/1 主机不可达 | 1/3 | 地址不可达 |
| 11/0 TTL 超时 | 3/0 | 跳数限制 |
| 3/4 需要分片 | **2/0 报文太长** | **源端 PMTUD** |

**难点**：v6 ICMP 校验和含 **128 位地址伪首部**；v4 仅对 ICMP 报文求和 → 转换开销大，常需硬件加速。

→ 与 [ch07 NAT64](../chapter07-firewall-nat/study.md#ch07-6) 控制面协同

---

<a id="ch08-7"></a>

## 8.7 与 ICMP 相关的攻击

设计缺**身份认证** → 反射、伪造、隧道滥用。

| 攻击 | 机制 |
|------|------|
| **Smurf** | 伪造受害者源 IP，向子网广播 **Echo Request** → 海量 **Reply** 淹没受害者 |
| **伪造差错** | 假 **不可达/Packet Too Big** → 断连或窗口骤降 |
| **重定向攻击** | 伪造 **Redirect** 劫持主机路由 |
| **ICMP 隧道** | 用 Echo **Data** 藏数据，绕过端口过滤 |
| **ICMP 泛洪** | 耗尽带宽或 CPU |

### 防御两难与最佳实践

| 极端 | 后果 |
|------|------|
| **全拦 ICMP** | **PMTUD 失效**、路径诊断瘫痪 |
| **全开放** | 泛洪、反射风险 |

**建议（差异化过滤）**：

- **放行**：ICMPv6 **Type 2**、关键 **Type 1** 等（PMTUD 生命线）  
- **限速**：入口 **Echo Request**  
- **禁用**：**Redirect**（主机侧常 ignore）  
- 结合 [ch07](../chapter07-firewall-nat/study.md#ch07-7) 状态表与 uRPF

---

<a id="ch08-exam"></a>

## 8.8 总结与考点

ICMP 是协议栈的**神经系统**：在不可靠 IP 之上建立**自反馈**。

### IPv4 → IPv6 五维演进

| 维度 | 变化 |
|------|------|
| 职能 | 分散辅助 → **集成 ARP/IGMP/ND** |
| 链路发现 | 广播 ARP → **多播 NS/NA** |
| 扩展 | 固定 → **Options**（移动 IP 等） |
| PMTUD | 重要 → **v6 唯一驱动**（中间不分片） |
| 配置 | 手工查询 → **RS/RA + SLAAC** |

### 易混速记

| 问题 | 要点 |
|------|------|
| ARP vs ND | v4 **ARP**；v6 **ND ⊂ ICMPv6** |
| v4 不可达 vs v6 PTB | v4 Type3 Code4；v6 **Type 2** |
| 差错能否再触发差错 | **不能** |
| 屏蔽 ICMP 与 NAT | 防火墙拦 Type2 → **TCP 大数据挂死** |
| Ping Type | v4 **8/0**；v6 **128/129** |

### 下一章

- [ch09 广播/多播](../chapter09-broadcast-multicast/study.md) — IGMP/MLD  
- [ch10 UDP](../chapter10-udp-ip-fragment/study.md) — 端口不可达、PMTUD  
- [ch07 防火墙](../chapter07-firewall-nat/study.md) — ICMP 过滤策略

---

## Top-Down

- [04_network_layer_data_plane/study.md](../../top_down/04_network_layer_data_plane/study.md)（PMTUD、NAT 与 MTU）  
- [06_link_layer §6.7](../../top_down/06_link_layer_and_lan/study.md#ch6-7)（Traceroute 路径）

## Lab

- `ping` / `ping6`；`traceroute` / `tracepath`（ICMP 超时）  
- `ip neigh` 观察 NUD 状态  
- 故意阻断 ICMPv6 Type 2，观察 TCP 大文件传输

## Go / Rust

- **Go**：`net.ICMPConn`；`ipv4.PacketConn` 发 Echo；解析 **Destination Unreachable**  
- **Rust**：`pnet` / `surge-ping`；容器网络 CNI 常要求允许 **Fragmentation Needed / PTB**  
- **排障**：高延迟先 `mtr`；黑洞用 `tracepath` 看 MTU 标记
