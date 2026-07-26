# 第 7 章：防火墙与网络地址转换（NAT）

> 按书节速记：[7.1](7.1-introduction.md) · [7.2](7.2-packet-filter-firewall.md) · [7.3](7.3-nat-napt.md) · [7.4](7.4-nat-traversal.md) · [7.5](7.5-acl-port-control.md) · [7.6](7.6-ipv6-nat-transition.md) · [7.7](7.7-security-attacks.md) · [7.8](7.8-summary.md) · [QUICKREF §7](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/03_network_layer/ch07_firewall_nat.md)）  
> 前置：[ch05 IP](../chapter05-ip-protocol/study.md) · [ch01 端到端](../chapter01-overview/study.md#ch01-e2e) · 自顶向下：[§4.3 NAT](../../top_down/04_network_layer_data_plane/study.md#ch4-3)

**Middlebox** 打破早期 **端到端透明**：防火墙建立**受限连通**安全边界，**NAT** 在 IPv4 枯竭下实现地址复用 — 网络从「哑核心、智能边缘」转向**状态化边界**。

---

<a id="ch07-1"></a>

## 7.1 引言

→ 精读：[7.1-introduction.md](7.1-introduction.md)

### 为何需要 Middlebox

早期 **端到端透明**（只转发、不改包）→ 现实压力：

| 压力 | 应对 |
|------|------|
| **IPv4 枯竭** | **NAT** 地址复用 |
| **安全威胁** | **防火墙** 边界过滤 |

打破透明，换 **安全 + 地址复用**。

### 防火墙 vs NAT

| | **防火墙** | **NAT** |
|--|------------|---------|
| 角色 | 流量警察 | 地址翻译官 |
| 核心 | **受限连通 + 策略过滤** | **私网→公网 + PAT 端口复用** |
| 节 | [7.2](#ch07-2) | [7.3](#ch07-3) |

### 代价（破坏端到端）

- 真实 IP/端口被改写；外网难主动连内网  
- **ALG**、**STUN/TURN/ICE**；会话表状态开销  

### 开发默认前提

假设必有 NAT/防火墙 → **保活**、端口映射/DMZ、不依赖固定公网 IP。

### 为何仍必需

IPv4 枯竭 + 经济边界防护 → **现实折中**（非理想端到端）。

### 本章主线

包过滤/状态防火墙 · NAT/PAT · 穿透 · 家用路由器二合一

---

<a id="ch07-2"></a>

## 7.2 防火墙

→ 精读：[7.2-packet-filter-firewall.md](7.2-packet-filter-firewall.md) · [三条规则+iptables](7.2-packet-filter-firewall.md#ch07-2-rules)

部署于**可信内网**与**不可信外网**边界，按规则管控往来流量 — 阻止未授权访问、放行合法通信。

### 包过滤（L3/L4）

- **转发前**匹配 IP/TCP/UDP 首部（地址、协议号、端口、TCP 标志）  
- **三条基础规则**：① 外网纯 **SYN** DROP · ② 出站仅 **80/443** · ③ **黑名单 IP** 最前 → [§背诵](7.2-packet-filter-firewall.md#ch07-2-rules-cheat)  
- **无状态**：逐包独立判决，快但易被伪造包绕过  
- **有状态**：**五元组状态表**跟踪会话，回程匹配表项放行 → 主流方案  

### 应用层网关 / 代理防火墙（L7）

- 内网只连**防火墙**；防火墙再**新建连接**访问外网 — **无端到端直连**  
- 可解析 HTTP/FTP 等内容；隐藏内网拓扑；延迟与开销大  
- 勿与 NAT 的 **ALG（改载荷地址）** 混为一谈 → [7.3](#ch07-3)

### DMZ

隔离网段放对外服务器；外网仅达 DMZ；DMZ 通常**不可直入内网**。

### 二者对比

| | 包过滤 | 代理 |
|--|--------|------|
| 层 | L3/L4 | L7 |
| 行为 | 头部检查 + 可选状态表 | 双重连接 + 应用解析 |
| 性能 | 高 | 低 |
| 深度 | 头部/会话 | 应用内容 |

<a id="ch07-2-state"></a>

### TCP 状态检测（补充机制）

1. **SYN** 到达 → 策略允许 → 状态表建**半开**项  
2. 后续包校验 **SEQ/ACK**；无状态项却带 ACK → 可能非法注入  
3. 反向 **SYN-ACK** 与初始五元组/状态一致 → Allow；违规 → Drop  

### 防火墙 vs IDS

| | 防火墙 | IDS |
|--|--------|-----|
| 角色 | **阻断**准入 | **检测**报警，常不直接阻断 |

---

<a id="ch07-3"></a>

## 7.3 网络地址转换（NAT）

→ 精读：[7.3-nat-napt.md](7.3-nat-napt.md) · [速记卡](7.3-nat-napt.md#ch07-3-cheat) · [工作过程](../../../top_down/04_network_layer_data_plane/4.3_ipv4_ipv6_nat.md#ch4-3-nat-flow)

缓解地址短缺，但使中间节点维护映射状态，**端到端透明性**受损。

**五要点**：隐藏内网 · **改 IP 头+状态表** · **NAPT** 多对一 · 边界网关 · 五元组**老化**

### 7.3.1 基本 NAT 与 NAPT

| 类型 | 行为 |
|------|------|
| **基本 NAT** | 主要改 **IP**；内网 ↔ 公网池 **一对一** |
| **NAPT (NAT/PAT)** | **IP + 端口** 多对一；最常见；默认仅**由内向外**易建立 |

### 7.3.2 映射行为 (Mapping)

内部 `(IP:Port)` → 外部 `(IP:Port)` 的分配规则：

| 类型 | 行为 |
|------|------|
| **端点无关 (EIM)** | 无论外部目标是谁，同一内部端点 → **同一**外部映射 |
| **地址依赖 (ADM)** | 发往不同外部地址 → **不同**外部端口 |

### 7.3.3 过滤行为 (Filtering)

外向内的报文是否允许：

| 类型 | 行为 | P2P |
|------|------|-----|
| **端点无关 (EIF)** | 有映射则**任意**外部主机可打入 | 较易 |
| **地址依赖 (ADF)** | 仅曾通信过的外部 IP 可回包 | **更安全**，P2P 更难 |

> **考点**：决定 P2P 能否打通的往往是 **Filtering**，不仅是 Mapping。

### 7.3.4–7.3.5 服务器与发夹 (Hairpinning)

内网 A、B 经公网信令交换到的是**公网映射**；若 A 访问「自己的公网 IP:端口」指向 B，需 NAT **发夹**：识别目的为自身公网地址的流量并**转回内网**，否则同网段无法互访映射端点。

### 7.3.6–7.3.7 ALG 与 CGN

| 主题 | 说明 |
|------|------|
| **NAT ALG / 编辑器** | 改应用层中的**私网 IP 泄露**（如 FTP **PORT** 命令） |
| **CGN (NAT444)** | ISP 侧多层 NAT；额外延迟、**端口配额**更紧 |

→ FTP/ALG 属边界盒行为，非链路层协议本身。

→ 厚版：[7.3 Hairpin/ALG/四类 NAT](7.3-nat-napt.md) · [§速记卡](7.3-nat-napt.md#ch07-3-cheat)

**通俗对照**：家用/虚拟机 **NAT 模式** — **虚拟 NAT + 路由器 NAT 两层** → [ch06 §6.8 实战](../chapter06-dhcp-config/6.8-practical-dhcp-bridge-nat-vm.md#ch06-8-nat-vm-layers)

<a id="ch07-4"></a>

## 7.4 NAT 穿越

→ 精读：[7.4-nat-traversal.md](7.4-nat-traversal.md) · [打洞](7.4-nat-traversal.md#ch07-4-hole-punch) · [STUN/TURN/ICE](7.4-nat-traversal.md#ch07-4-stun-turn-ice) · [考点](7.4-nat-traversal.md#ch07-4-cheat)

### 一、UDP 打洞

内网**主动外发包** → NAT 建**外向映射** → 对端经 **公网 IP:Port** 反向打入。成败看 **NAT 类型**（**全锥易，对称最难**）→ [7.3](7.3-nat-napt.md#ch07-3-mapping-types)

### 二、三剑客

| 协议 | 作用 |
|------|------|
| **STUN** | 查 **公网 IP:Port**（纯查询） |
| **TURN** | 打洞失败 **中继**（兜底） |
| **ICE** | 候选收集 + 探测，**择优路径** |

### 三、实战 · 四、保活

Go `pion/*` · Rust `webrtc-rs`/`libp2p`；降级 TURN → 查**对称 NAT** / UDP 拦截 / 超时。  
**STUN 刷新 / 心跳** 防映射空闲清除。

---

<a id="ch07-5"></a>

## 7.5 配置包过滤防火墙和 NAT

→ 精读：[7.5-acl-port-control.md](7.5-acl-port-control.md) · [ACL](7.5-acl-port-control.md#ch07-5-acl) · [Open NAT](7.5-acl-port-control.md#ch07-5-upnp) · [考点](7.5-acl-port-control.md#ch07-5-cheat)

### 架构：过滤先于 NAT

**过滤规则应先于 NAT 转换** — 先 NAT 会让攻击/无效流量占满**状态表/端口映射** → 合法流 DoS。

### ACL

**自上而下**，**First Match**；细规则在前，**默认 DROP** 收尾。  
标准顺序：**黑名单 → 拒外网 SYN → 80/443 → 默认丢弃** → [7.2 iptables](7.2-packet-filter-firewall.md#ch07-2-rules-order)

### 动态端口映射（Open NAT）

| 协议 | 说明 |
|------|------|
| **UPnP IGD** | 传统主流；游戏/P2P/VoIP |
| **NAT-PMP / PCP** | 新一代；更安全规范 |

家用常开；**企业一般禁用**（防端口暴露）。

### 运维

`conntrack` 满 → **无法新建连接** → [7.7 状态耗尽](7.7-security-attacks.md)

---

<a id="ch07-6"></a>

## 7.6 IPv4/IPv6 共存与过渡

→ 精读：[7.6-ipv6-nat-transition.md](7.6-ipv6-nat-transition.md) · [DS-Lite](7.6-ipv6-nat-transition.md#ch07-6-dslite) · [NAT64](7.6-ipv6-nat-transition.md#ch07-6-nat64) · [考点](7.6-ipv6-nat-transition.md#ch07-6-cheat)

**过渡技术** — 最终目标：**全原生 IPv6 端到端**。

### DS-Lite

内网仍 **IPv4 私网** → CPE **B4** 封装进 **IPv6 隧道** → 运营商 **AFTR/CGN** 解封装 + **NAPT** → 省公网 v4。

### NAT64 + DNS64

**纯 IPv6 终端** 访问 **IPv4 服务**：

| 组件 | 作用 |
|------|------|
| **DNS64** | 无 AAAA 时合成 AAAA，前缀 **`64:ff9b::/96`** |
| **NAT64** | **IPv6 ↔ IPv4** 双向协议翻译 |

→ DNS：[ch11 §11.9](../chapter11-dns-domain-resolve/11.9-dns-ipv6-transition.md)

### 区分

| | DS-Lite | NAT64 |
|---|---------|-------|
| 用户 | **仍用 IPv4** | **仅 IPv6** |
| 机制 | v4 **over v6 隧道** + CGN | **协议翻译** + DNS64 |

---

<a id="ch07-7"></a>

## 7.7 相关攻击

→ 精读：[7.7-security-attacks.md](7.7-security-attacks.md) · [隧道绕过](7.7-security-attacks.md#ch07-7-tunnel) · [状态耗尽](7.7-security-attacks.md#ch07-7-state-exhaustion) · [考点](7.7-security-attacks.md#ch07-7-cheat)

### 一、应用层隧道绕过

恶意流量封装在 **80/443** 放行端口内（HTTPS 藏 C2）→ 规避端口过滤。  
防护：**TLS 检测**、**正向代理**、**行为分析**。

### 二、NAT 状态耗尽

大量新建/半开连接 → **`conntrack` 满** → 新连接失败、内网断网。NAT **非专业安全设备**，易成可用性瓶颈 → [7.5](7.5-acl-port-control.md#ch07-5-ops)

### 三、缓解

**限流**（单 IP 新建速率）· **调优**（缩短超时、扩表）· **架构**（DMZ / 原生 **IPv6**）

### 补充攻击面

| 攻击 | 说明 |
|------|------|
| **分片绕过** | 首包审查后内网重组非法 L4 |
| **状态注入** | 伪造 ACK/序列号误导状态表 |

→ [ch05 §5.7](../chapter05-ip-protocol/study.md#ch05-7)

---

---

<a id="ch07-nat-onpager"></a>

## NAT 一页纸（背诵版）

→ 完整版：[7.3 §速记卡](7.3-nat-napt.md#ch07-3-cheat)

| 块 | 一句 |
|----|------|
| **基础** | 改 IP/端口 + **状态表** + 老化；**边界网关** |
| **NAPT** | **IP+端口** 多对一；对外服务 **静态映射** |
| **Hairpin** | 内网访**公网 IP** → **SNAT+DNAT** 发夹 |
| **ALG** | 改**载荷**私网地址；**TLS 失效** |
| **四类** | **全锥易 → 对称靠 TURN** |

易错：NAPT 靠**端口** · ALG 仅明文 · 内网公网 IP 互访要 **Hairpin** · 对称 NAT **无固定映射**

---

<a id="ch07-exam"></a>

## 7.8 总结与考点

边界设计需在 **Security / Transparency / Complexity** 间折中。

### 三条结论

1. **防火墙 + NAT** 使互联网失去无状态简洁性 → 协议栈需 **ALG** 等感知中间件。  
2. **NAT Filtering Behavior** 常是决定 **P2P** 能否连通的关键。  
3. IPv6 普及前，**NAT64/DNS64** 等长期承担**跨族**连通。

### 易混速记

| 问题 | 要点 |
|------|------|
| NAPT vs 基本 NAT | 是否改**端口**、多对一 → [7.3](7.3-nat-napt.md#ch07-3-napt-compare) |
| NAT 四类 / P2P | **全锥易、对称靠 TURN** → [7.3](7.3-nat-napt.md#ch07-3-mapping-types) · [7.4 打洞](7.4-nat-traversal.md#ch07-4-hole-punch) |
| STUN/TURN/ICE | **STUN 查、TURN 中继、ICE 优选** → [7.4](7.4-nat-traversal.md#ch07-4-cheat) |
| EIM vs ADM / EIF vs ADF | **Mapping** 是否随目的变；**Filtering** 决定谁能打入 → [study §7.3.2–3](#ch07-3) |
| Hairpin | 内网访问**本 NAT 公网 IP** → **DNAT+SNAT 发夹** → [7.3](7.3-nat-napt.md#ch07-3-hairpin) |
| ALG vs 代理 | ALG 改**载荷 IP**；TLS 下失效 → [7.3](7.3-nat-napt.md#ch07-3-alg) |
| 过滤 vs NAT 顺序 | **先过滤后 NAT** → [7.5](7.5-acl-port-control.md#ch07-5-acl) |
| ACL 顺序 | **黑→SYN→80/443→DROP** → [7.5](7.5-acl-port-control.md#ch07-5-cheat) |
| NAT64/DNS64 | 纯 v6 访 v4；前缀 **64:ff9b::/96** → [7.6](7.6-ipv6-nat-transition.md#ch07-6-nat64) |
| DS-Lite vs NAT64 | 用户仍 v4 **隧道** vs 用户仅 v6 **翻译** → [7.6 对比](7.6-ipv6-nat-transition.md#ch07-6-compare) |
| 隧道 bypass | **80/443** 藏恶意流 → [7.7](7.7-security-attacks.md#ch07-7-tunnel) |
| 状态耗尽 | 打满 **conntrack** → 断新连 → [7.7](7.7-security-attacks.md#ch07-7-state-exhaustion) |

### 下一章

- [ch08 ICMP](../chapter08-icmpv4-icmpv6/study.md) — 不可达、PMTUD  
- [ch11 DNS](../chapter11-dns-domain-resolve/study.md) — DNS64  
- [04_network_layer §4.3 NAT](../../top_down/04_network_layer_data_plane/4.3_ipv4_ipv6_nat/README.md)

---

## Top-Down

- [study.md §4.3](../../top_down/04_network_layer_data_plane/study.md#ch4-3) · [中间盒 §4.5](../../top_down/04_network_layer_data_plane/study.md#ch4-5)

## Lab

- 家用路由器：端口映射、DMZ、UPnP 开关对比  
- `conntrack -L`（Linux）观察 NAT 状态  
- WebRTC：`ice` 候选类型（host/srflx/relay）

## Go / Rust

- **K8s**：Service **ClusterIP / NodePort / LoadBalancer** 与 kube-proxy NAT  
- **云**：SNAT 网关、安全组 = 有状态过滤 + 无 NAT 场景  
- **排障**：P2P 失败先查 NAT 类型 + 是否需 TURN；FTP 被动模式 + ALG
