# 第 6 章：系统配置 — DHCP 与自动配置

> 按书节速记：[6.1](6.1-introduction.md) · [6.2](6.2-dhcp-protocol.md) · [6.3](6.3-slaac-autoconfig.md) · [6.4](6.4-dhcp-dns-ddns.md) · [6.5](6.5-pppoe.md) · [6.6](6.6-dhcp-security.md) · [6.7](6.7-summary.md) · [6.8 实战](6.8-practical-dhcp-bridge-nat-vm.md) · [QUICKREF §6](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/03_network_layer/ch06_dhcp.md)）  
> 前置：[ch05 IP](../chapter05-ip-protocol/study.md) · [ch04 ARP](../chapter04-arp-protocol/study.md) · 自顶向下：[§4.3 DHCP](../../top_down/04_network_layer_data_plane/study.md#ch4-3)

本章聚焦协议栈 **自举（Bootstrapping）**：主机如何从无到有获得 **IP、掩码、网关、DNS** 等参数 — 不仅是地址分配，更是**资源发现与策略下发**的纽带。

---

<a id="ch06-1"></a>

## 6.1 引言

→ 精读：[6.1-introduction.md](6.1-introduction.md) · [DHCP 通俗](6.1-introduction.md#ch06-1-dhcp-plain)

### DHCP 是什么（通俗）

**DHCP = 自动给内网设备配 IP** — 下发 IP/掩码/网关/DNS；**租期**续租防冲突；路由器默认开启，Wi‑Fi/有线设备全靠它。

→ [6.1 详述](6.1-introduction.md#ch06-1-dhcp-plain) · 协议：[6.2](#ch06-2) · **DHCP vs 桥接 vs NAT**：[6.8 虚拟机实战](6.8-practical-dhcp-bridge-nat-vm.md#ch06-8)

### 为何不能一直手动配

| 问题 | 说明 |
|------|------|
| 规模化 | 易错、难维护、成本高 |
| 移动化 | 换网频繁，无法写死 |
| 用户能力 | 家庭/企业需**自动化** |

**一句话**：规模化 + 移动化 → 手动配置不可扩展。

### 即插即用

接入链路后自动获得：**IP、掩码、网关、DNS、NTP** 等 → 开机即上网。

### 协议栈自举

```text
L2 接通 → L3（DHCP / SLAAC+DHCPv6）→ DNS → 应用
```

无 L3 自动配置 → TCP/UDP/应用「无源之水」。

### 两条主线

| 机制 | 特点 | 场景 |
|------|------|------|
| **DHCP** | **有状态**；服务器分配地址+全套参数 | 企业、家宽、IPv4 主流 → [6.2](#ch06-2) |
| **APIPA** | DHCP 失败 → **169.254/16**，不跨路由 | IPv4 备用 |
| **SLAAC** | **无状态**；RA 给前缀，主机自拼地址 | IPv6 零配置 → [6.3](#ch06-3) |

### DHCP vs SLAAC

- **DHCP**：集中管理、可审计  
- **SLAAC**：零配置、高扩展、IPv6 标配  

### 演进脉络

**RARP → BOOTP → DHCP**（v6 侧还有 **DHCPv6** 与 **SLAAC**）。

---

<a id="ch06-2"></a>

## 6.2 动态主机配置协议（DHCP）

→ 精读：[6.2-dhcp-protocol.md](6.2-dhcp-protocol.md) · [DORA](6.2-dhcp-protocol.md#ch06-2-dora) · [租期](6.2-dhcp-protocol.md#ch06-2-lease) · [DHCPv6](6.2-dhcp-protocol.md#ch06-2-v6)

**DHCP** 本质是 **应用层（UDP 67/68）** 协议，却在 **网络层初始化** 中起决定性作用。

### 为何放在应用层？

利用现有 **UDP 栈** 处理租约、多选项等复杂逻辑，**不必在内核重写**底层交互 — 降低内核复杂度。

---

<a id="ch06-2-lease"></a>

### 6.2.1 地址池与租约

| 概念 | 说明 |
|------|------|
| **地址池** | 服务器可分配的范围 |
| **租约 (Lease)** | 临时 IP 使用权，促进地址循环 |

| 定时器 | 时机 | 行为 |
|--------|------|------|
| **T1（续租）** | 租约 **50%** | 向**原服务器**单播 **Renew** |
| **T2（重绑定）** | 租约 **87.5%** | 原服务器无响应 → **广播**找任意可用服务器 |

→ 详：[6.2 §租期](6.2-dhcp-protocol.md#ch06-2-lease)

---

<a id="ch06-2-msg"></a>

### 6.2.2 消息格式（BOOTP 兼容）

| 字段 | 含义 |
|------|------|
| **op** | 1=请求，2=应答 |
| **htype/hlen** | 硬件类型/长度（以太网 **1** / **6**）→ **媒介无关** |
| **xid** | 事务 ID，匹配请求/响应 |
| **ciaddr** | 客户端已知 IP |
| **yiaddr** | **Your IP** — 服务器分配 |
| **giaddr** | 中继代理地址（跨网段） |
| **Magic Cookie** | **99.130.83.99** — BOOTP/DHCP 分界，后续按 **选项** 解析 |

---

<a id="ch06-2-options"></a>

### 6.2.3 常用选项

| Option | 内容 |
|--------|------|
| **1** | 子网掩码 |
| **3** | 路由器（默认网关） |
| **6** | DNS 服务器 |
| **15** | 域名 |

---

<a id="ch06-2-dora"></a>

### 6.2.4 DORA 四步交互

| 步骤 | 含义 | L2 MAC | L3 IP |
|------|------|--------|-------|
| **Discover** | 找服务器 | Client → **FF:FF:FF:FF:FF:FF** | **0.0.0.0 → 255.255.255.255** |
| **Offer** | 提供配置 | Server → Client（或广播） | Server → **255.255.255.255** 等 |
| **Request** | 选中并请求（DECLINE 其他 Offer） | Client → **广播** | **0.0.0.0 → 255.255.255.255** |
| **ACK** | 租约生效 | Server → Client | Server → 255.255.255.255 等 |

**Request 广播的原因**：让网内**所有**曾 Offer 的服务器知晓客户端选择，释放未中标地址。

→ 厚版：[6.2 §三 DORA](6.2-dhcp-protocol.md#ch06-2-dora) · 易混：[6.2 §考点](6.2-dhcp-protocol.md#ch06-2-cheat)

---

<a id="ch06-2-ext"></a>

### 6.2.5–6.2.12 扩展特性

| 主题 | 要点 |
|------|------|
| **DHCPv6** | 少用广播；组播 **ff02::1:2**；**Solicit → Advertise → Request → Reply**（四步，名称不同） |
| **中继 (Relay Agent)** | 捕获客户端广播 → **单播** 至远端服务器；**giaddr** 标识中继；解决广播不过路由器 |
| **位置/移动 (6.2.10–11)** | **LCI**、**LoST** 地理位置；**MoS**、**ANDSF** 辅助 WLAN/蜂窝选择 |
| **DHCP Probe (6.2.12)** | 分配后 **ARP** 或相邻探测，确认链路上地址可用 → **ACD 最后一道防线** |

### 常见故障

租约异常中止 · 中继路径 **MTU** 截断 · 未 Probe 导致 **IP 冲突**

→ 免费 ARP/ACD：[ch04 §4.8](../chapter04-arp-protocol/study.md#ch04-8)

---

<a id="ch06-3"></a>

## 6.3 无状态地址自动配置（SLAAC）

→ 精读：[6.3-slaac-autoconfig.md](6.3-slaac-autoconfig.md) · [APIPA](6.3-slaac-autoconfig.md#ch06-3-apipa) · [SLAAC 流程](6.3-slaac-autoconfig.md#ch06-3-slaac) · [M/O](6.3-slaac-autoconfig.md#ch06-3-mo) · [考点](6.3-slaac-autoconfig.md#ch06-3-cheat)

### 一、IPv4 APIPA

**DHCP 不可达** → 自动 **`169.254.0.0/16`**；**免费 ARP/ACD** 冲突检测；仅链路本地。

### 二、IPv6 SLAAC

**RS** 问前缀 → **RA** 回前缀 + **M/O** → 地址 = **前缀 + IID**（默认 **EUI-64**）

### 三、M/O 标志

| 位 | 置 1 |
|----|------|
| **M** | **有状态 DHCPv6** 分配地址 |
| **O** | 地址仍 SLAAC；仅 DHCPv6 要 **DNS** 等 |

**SLAAC 不强制 DHCPv6** — 看 RA 标志 → [6.2](6.2-dhcp-protocol.md)

### 补充：EUI-64 · 隐私扩展

48b MAC 插 **0xFFFE**、翻 U/L 位；RFC 4941 随机 IID 防追踪 → [ch08 ND](../chapter08-icmpv4-icmpv6/study.md)

---

<a id="ch06-4"></a>

## 6.4 DHCP 与 DNS 交互

→ 精读：[6.4-dhcp-dns-ddns.md](6.4-dhcp-dns-ddns.md) · [痛点](6.4-dhcp-dns-ddns.md#ch06-4-pain) · [DDNS](6.4-dhcp-dns-ddns.md#ch06-4-ddns) · [考点](6.4-dhcp-dns-ddns.md#ch06-4-cheat)

**痛点**：DHCP 动态 IP → 域名–IP 映射失效，主机名无法访问。

**DDNS**：IP 变更时自动更新 **A/AAAA**；**DHCP 服务器或客户端**发起；须 **TSIG** 等认证防篡改。

**应用**：企业 **AD 域**、家用路由器主机名自动注册。

→ DNS：[ch11](../chapter11-dns-domain-resolve/study.md)

---

<a id="ch06-5"></a>

## 6.5 以太网上的 PPP（PPPoE）

→ 精读：[6.5-pppoe.md](6.5-pppoe.md) · [本质](6.5-pppoe.md#ch06-5-role) · [流程](6.5-pppoe.md#ch06-5-flow) · [考点](6.5-pppoe.md#ch06-5-cheat)

**本质**：以太网承载 PPP — 宽带**认证 + 公网 IP**。

**三阶段**：**PADI/PADO/PADR/PADS 发现** → **LCP + PAP/CHAP** → **IPCP 配 IP/DNS**

**双层**：外网 **PPPoE 拨号**（公网）+ 内网 **DHCP**（私网）→ [ch03 PPP](../chapter03-link-layer/3.6-ppp-protocol.md#ch03-6-pppoe)

---

<a id="ch06-6"></a>

## 6.6 与系统配置相关的攻击

→ 精读：[6.6-dhcp-security.md](6.6-dhcp-security.md)

DHCP **无强认证**，默认信任**物理链路**。

| 攻击 | 说明 |
|------|------|
| **DHCP 饥饿** | 伪造大量 Discover，耗尽地址池 |
| ** Rogue DHCP** | 更快 Offer → 伪造网关/DNS → **MITM** |

**防御**：接入交换机 **DHCP Snooping** — 信任端口仅连合法服务器，丢弃其他端口的 DHCP **Server 报文**。

---

<a id="ch06-8"></a>

## 6.8 实战：DHCP、桥接、NAT 与虚拟机

→ 精读：[6.8-practical-dhcp-bridge-nat-vm.md](6.8-practical-dhcp-bridge-nat-vm.md) · [物理 NAT](6.8-practical-dhcp-bridge-nat-vm.md#ch06-8-nat-physical) · [两层 NAT](6.8-practical-dhcp-bridge-nat-vm.md#ch06-8-nat-vm-layers) · [地址举例](6.8-practical-dhcp-bridge-nat-vm.md#ch06-8-address-example) · [记忆](6.8-practical-dhcp-bridge-nat-vm.md#ch06-8-cheat)

**NAT**（网络地址转换）= 内网↔公网**翻译官**；**DHCP** = 自动发 IP；**桥接/NAT** = VM **网络模式**。

| 场景 | NAT 层数 | VM IP 示例 |
|------|----------|------------|
| **物理上网** | 路由器 **1 层 NAT** | 手机 `192.168.1.x` |
| **VM 桥接** | 仍 **1 层**（无虚拟 NAT） | `192.168.1.11` 同网段 |
| **VM NAT** | **虚拟 NAT + 路由 NAT** 两层 | `192.168.122.20` → 真机 → 公网 |

**DHCP** 只管发 IP/掩码/网关 — 桥接、NAT **都能用**。互访→**桥接**；隔离→**NAT**。

---

<a id="ch06-exam"></a>

## 6.7–6.8 总结与考点

### 对比表

| 特性 | DHCPv4 | DHCPv6 | IPv6 SLAAC |
|------|--------|--------|------------|
| 中心化 | 高（Stateful） | 高（Stateful） | **极低（Stateless）** |
| 传输 | UDP **67/68** | UDP **546/547** | **ICMPv6 NDP** |
| 底层 | **广播** | **组播** | **组播** |
| 优势 | 选项丰富、策略可控 | Rapid Commit 等 | 极简、去中心化 |
| 回退/本地 | **169.254.0.0/16** | — | **fe80::/10** 链路本地 |

### 易混速记

| 问题 | 要点 |
|------|------|
| APIPA vs SLAAC | v4 **169.254/16** vs v6 **RA 拼地址** → [6.3](6.3-slaac-autoconfig.md#ch06-3-cheat) |
| M / O 标志 | **M=有状态地址**；**O=仅 DNS，地址仍 SLAAC** → [6.3 M/O](6.3-slaac-autoconfig.md#ch06-3-mo) |
| SLAAC 必配 DHCPv6 | **否** — 看 RA **M/O** |
| DDNS | DHCP 换 IP → 刷 **A/AAAA**；须 **TSIG** → [6.4](6.4-dhcp-dns-ddns.md#ch06-4-cheat) |
| PPPoE 三阶段 | **发现 → LCP+认证 → IPCP** → [6.5](6.5-pppoe.md#ch06-5-cheat) |
| PPPoE vs DHCP | **外网 PPPoE + 内网 DHCP**，不同层级 |
| DHCP vs 桥接/NAT | **DHCP=服务**；**桥接/NAT=VM 模式** → [6.8](6.8-practical-dhcp-bridge-nat-vm.md#ch06-8-cheat) |
| VM 桥接 vs NAT | 桥接**同网段一层NAT**；NAT**122.x+两层NAT** → [6.8](6.8-practical-dhcp-bridge-nat-vm.md#ch06-8-nat-vm-layers) |

从 **DHCP 的行政管理** 到 **SLAAC 的协作发现** — 按业务在**可控性**与**灵活性**之间选型（云/数据中心常仍重度依赖 DHCP/DHCPv6）。

### 关键 RFC

| RFC | 主题 |
|-----|------|
| 2131 | DHCPv4 |
| 3315 | DHCPv6 |
| 4862 | SLAAC |
| 4941 | SLAAC 隐私扩展 |
| 3927 | IPv4 链路本地 |

### 下一章

- [ch07 NAT/防火墙](../chapter07-firewall-nat/study.md)  
- [ch08 ICMP](../chapter08-icmpv4-icmpv6/study.md) — RA/RS、ND  
- [ch05 IP](../chapter05-ip-protocol/study.md)

---

## Top-Down

- [04_network_layer_data_plane §4.3](../../top_down/04_network_layer_data_plane/study.md#ch4-3)  
- [06_link_layer §6.7 Web 路径](../../top_down/06_link_layer_and_lan/study.md#ch6-7)（DHCP 获取地址）

## Lab

- `dhclient -v` / Windows `ipconfig /all` 看租约、T1/T2  
- Wireshark：`bootp` 过滤器，抓 **DORA**  
- 容器/K8s：CNI 与 DHCP 关系（多数用静态/CNI 而非经典 DORA）

## Go / Rust

- **Go**：`net.Interfaces()` 看重启后地址；云元数据（非 DHCP）替代场景  
- **排障**：无地址时查 **169.254.x.x**；IPv6 `fe80::` + `ip -6 addr`  
- **安全**：数据中心启用 **DHCP Snooping** + 固定 DNS
