# 第 4 章：地址解析协议（ARP）

> 按书节速记：[4.1](4.1-introduction.md) · [4.2](4.2-arp-basic-operation.md) · [4.3](4.3-arp-cache.md) · [4.4](4.4-arp-packet-format.md) · [4.5](4.5-arp-tcpdump-example.md) · [4.6](4.6-arp-cache-timeout.md) · [4.7](4.7-proxy-arp.md) · [4.8](4.8-gratuitous-arp.md) · [4.9](4.9-arp-cli-commands.md) · [4.10](4.10-embedded-arp-setup.md) · [4.11](4.11-arp-spoof-defense.md) · [4.12](4.12-summary.md) · [QUICKREF §4](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/03_network_layer/ch04_arp.md)）  
> 链路层基础：[ch03 链路层](../chapter03-link-layer/study.md) · 自顶向下：[§6.4.1 ARP](../../top_down/06_link_layer_and_lan/study.md#ch6-4)

**ARP** 是 **IPv4 ↔ 以太网 MAC** 的战略支点：IP 提供端到端逻辑标识，**L2 交付必须靠 48 位 MAC**；硬件不识别 IP 标签，同网段内不能直接“按 IP 发帧”。

> **IPv6** 不用 ARP，而用 **邻居发现（ND，ICMPv6）** — 考点对照见 [ch08 ICMP](../chapter08-icmpv4-icmpv6/study.md)、[QUICKREF §4](../QUICKREF.md)。

---

<a id="ch04-1"></a>

## 4.1 引言与地址解析的必要性

→ 精读：[4.1 大白话](4.1-introduction.md) · [为何需要 ARP](4.1-introduction.md#ch04-1-why) · [触发条件](4.1-introduction.md#ch04-1-trigger) · [三行背诵](4.1-introduction.md#ch04-1-cheat)

- **功能**：**IPv4 → 以太网 MAC**（**RFC 826**）；**IPv6 用 ND**，不用 ARP。
- **范围**：**同一广播域**；跨网段 **ARP 网关**，不 ARP 远端主机。
- **铁律**：ARP **绝不跨路由器**；路由器是**换网卡、换 LAN 再 ARP** → [4.2 §零附 C](4.2-arp-basic-operation.md#ch04-2-iron-rules)
- **L3/L2**：路由最终要坍缩成**下一跳 MAC**；ARP 是 IPv4 在以太网上的主要动态机制。

---

<a id="ch04-2"></a>

## 4.2 工作实例：直接交付与协议交互

→ 精读：[4.2-arp-basic-operation.md](4.2-arp-basic-operation.md) · [交换机/路由器分工](4.2-arp-basic-operation.md#ch04-2-switch-router) · [路由+ARP 全流程](4.2-arp-basic-operation.md#ch04-2-route-arp)

### 交换机 vs 路由器

| 场景 | 设备 |
|------|------|
| 同广播域、同网段 | **交换机**（MAC 转发，不看 IP） |
| 跨网段 / 外网 | **路由器**（网关，隔离广播域） |

### 路由表 + ARP 联动

**路由表**：目标 IP → **下一跳 IP** + **出接口**（条目自带网口）  
**ARP 表**：下一跳 IP → **下一跳 MAC**  
**封装**：IP 目的 = 最终接收方（不变）；帧 MAC 目的 = 直连下一跳（每跳重写）

**路由器发 ARP**：路由表定 **出接口** → **只从该网口**发二层广播；广播**不过**其他接口 → [§零附 A](4.2-arp-basic-operation.md#ch04-2-router-arp-egress)

→ **双路由器三网段走读**：[§零附 B](4.2-arp-basic-operation.md#ch04-2-dual-router) · **ARP 铁律**：[§零附 C](4.2-arp-basic-operation.md#ch04-2-iron-rules)

| 概念 | 说明 |
|------|------|
| **直接交付** | 同一物理/广播域；掩码确认目标 IP 属本 subnet |
| **间接交付** | 跨子网：IP 目的为远端主机，**以太网目的 MAC 常为默认网关** |

→ 自顶向下必背：[§6.4.1](../../top_down/06_link_layer_and_lan/study.md#ch6-4)

### ARP 请求/响应循环

**按需触发**：缓存未命中时，常**挂起**待发 IP 数据报，先完成 ARP → **首包额外延迟**。

```text
1. 查内核 ARP 缓存
2. Miss → 构造 ARP 请求
   · 以太网目的 MAC = ff:ff:ff:ff:ff:ff（广播）
   · 子网内所有活动主机收到
3. 仅 IP 匹配的目标处理请求
4. 目标以单播 ARP 应答回请求方（请求里已有源 MAC）
```

### 效率逻辑

| 报文 | 为何 |
|------|------|
| **请求广播** | 尚不知目标 MAC，需覆盖全网段 |
| **应答单播** | 已知请求方 MAC，减少无关流量 |

---

<a id="ch04-3"></a>

## 4.3 ARP 缓存：效率优化的核心

→ 精读：[4.3](4.3-arp-cache.md) · [是什么](4.3-arp-cache.md#ch04-3-what) · [ip neigh 状态](4.3-arp-cache.md#ch04-3-states) · [老化](4.3-arp-cache.md#ch04-3-timeout) · [三行背诵](4.3-arp-cache.md#ch04-3-cheat)

避免反复广播；**命中缓存**则不发 ARP。

### 条目字段

| 字段 | 作用 |
|------|------|
| **IP 地址** | 逻辑检索键 |
| **MAC 地址** | 映射结果 |
| **类型** | **dynamic**（协议学习）/ **static**（手工） |
| **Timeout** | 生命周期 |

### 典型 `arp -a` 映射

| Internet Address | Physical Address | Type |
|------------------|------------------|------|
| 192.168.1.1 | 00-50-56-c0-00-08 | dynamic |
| 192.168.1.254 | 00-0c-29-3e-0a-4b | static |

（Linux 现代等价：`ip neigh show`）

### 状态与超时

→ 超时精读：[4.6](4.6-arp-cache-timeout.md#ch04-6-timeout)

| 状态 | 说明 |
|------|------|
| **完整条目** | 解析成功；常见存活约 **20 分钟**（因内核而异） |
| **Incomplete** | 已发请求、未收到应答；约 **3 分钟**清理 |
| **自愈** | 超时删除陈旧映射 → 网卡更换 / IP 变更后可重新学习 |

### 风险

**Incomplete 堆积**：高频发往“尚未解析”的 IP 时，可能耗尽缓存资源 → 类似 **ARP Flooding** 的 DoS 面。

---

<a id="ch04-4"></a>

## 4.4 ARP 帧格式详解

→ 精读：[4.4-arp-packet-format.md](4.4-arp-packet-format.md) · [请求 vs 应答](4.4-arp-packet-format.md#ch04-4-request-reply)

ARP 是**链路层负载**（EtherType **0x0806**），**不**封装在 IP 数据报内。

### 28 字节（2+2+1+1+2+6+4+6+4）

| 字段 | 典型值 |
|------|--------|
| 硬件类型 / 协议类型 | **1** / **0x0800** |
| 地址长度 | **6** / **4**（MAC / IPv4） |
| **Opcode** | **1** 请求，**2** 应答 |
| 发送方 / 目标 | MAC + IP；**请求时目标 MAC 全 0** |

### 请求 vs 应答（必背）

| | **请求（1）** | **应答（2）** |
|---|-------------|-------------|
| 以太网目的 MAC | **广播** `FF:FF:…` | **单播** 请求方 MAC |
| ARP 目标 MAC | **全 0** | 请求方真实 MAC |
| 一句 | 广播 + 全 0 + Opcode=1 | 单播 + 填充 + Opcode=2 |

→ 字段级详解：[4.4 §三](4.4-arp-packet-format.md#ch04-4-request-reply) · 抓包：[4.5](4.5-arp-tcpdump-example.md#ch04-5-normal)

### 以太网帧

```text
[目的 MAC][源 MAC][0x0806][ARP 28B][FCS]
```

| 类型 | 以太网目的 MAC | ARP 目标 MAC |
|------|----------------|--------------|
| **请求** | `FF:FF:FF:FF:FF:FF` 广播 | 全 0 |
| **应答** | 请求方 MAC 单播 | 填入真实 MAC |

### 抓包

`arp` · `arp.opcode == 1`（请求）· `arp.opcode == 2`（应答）

### 解析失败

不存在的主机 → ARP 多次后**静默超时** → 上层可能见 **ICMP Host Unreachable**。

→ 抓包实例：[4.5 ARP 例子](4.5-arp-tcpdump-example.md#ch04-5-capture)

---

<a id="ch04-5-capture"></a>

## 4.5 ARP 例子（抓包与超时）

→ 精读：[4.5-arp-tcpdump-example.md](4.5-arp-tcpdump-example.md)

| 场景 | 抓包特征 |
|------|----------|
| **正常** | who-has **广播** → is-at **单播**；~1–3 ms |
| **无主机** | **3 次** who-has / ~1 s；**Incomplete**；~3 min 超时 |
| **成功缓存** | 完整条目 ~**20 min** |

过滤器：`arp` · `arp.opcode == 1/2` · `eth.type == 0x0806`

---

<a id="ch04-6-timeout"></a>

## 4.6 ARP 缓存超时

→ 精读：[4.6-arp-cache-timeout.md](4.6-arp-cache-timeout.md)

| 类型 | 超时 | 机制 |
|------|------|------|
| **完整条目** | ~**20 min**（Linux/macOS；Windows 可更短） | 闲置倒计时；通信**刷新**；可先 **STALE → 探测** |
| **INCOMPLETE** | ~**3 min** | 无应答快速清理；常伴 **3 次** ARP 重试 |

ARP 缓存 = **软状态**（非永久；静态 `arp -s` 除外）→ 适配上下线、换网卡、IP 变更。

验证：`ip neigh` · `arp -a`

---

<a id="ch04-7-proxy"></a>

## 4.7 代理 ARP（Proxy ARP）

→ 精读：[4.7-proxy-arp.md](4.7-proxy-arp.md)

路由器收到**跨网段** ARP 广播 → **冒充目标**，用**自身接口 MAC** 代答 → 主机 ARP 表出现 **远端 IP → 网关 MAC**。

| 前提 | 利 | 弊 |
|------|-----|-----|
| 开代理 ARP + 有目标路由 | 主机**无需默认网关** | 排障难、ARP 风暴、安全风险 |

**必考区分**：代理 ARP **代答目标 IP**；正常路由 **仅 ARP 网关 IP**（帧 MAC 目的同为网关，看 ARP 表键）。

---

<a id="ch04-8"></a>

## 4.8 免费 ARP（Gratuitous ARP）

→ 精读：[4.8-gratuitous-arp.md](4.8-gratuitous-arp.md)

主机**广播**“自己的 IP → 自己的 MAC”的 ARP（常表现为 **请求** 形态，目标 IP = 本机 IP）。

| 用途 | 行为 |
|------|------|
| **ACD（地址冲突检测）** | 若收到**应答** → 该 IP 已被占用 → **负面确认** |
| **缓存刷新** | 故障转移 / 换网卡后，迫使邻居更新 ARP 表 |

### 易混（盲区）

免费 ARP **不期望**收到回复；**收到回复 = 配置冲突信号**，不是“正常握手成功”。

---

<a id="ch04-9"></a>

## 4.9 arp 命令

→ 精读：[4.9-arp-cli-commands.md](4.9-arp-cli-commands.md)

| 工具 | 用途 |
|------|------|
| `arp -a` / `arp -d` / `arp -s` | 查看 / 删除 / **静态绑定**（Windows / 旧 Unix） |
| `ip neigh show` / `flush` | Linux 邻居表（含 REACHABLE/STALE 等状态） |

---

<a id="ch04-10"></a>

## 4.10 嵌入式设备 IP 配置（ARP-Ping）

→ 精读：[4.10-embedded-arp-setup.md](4.10-embedded-arp-setup.md)

无屏、无 DHCP 设备（串口服务器、IoT）：**静态 ARP**（`arp -s IP MAC`）+ **Ping/arping 诱导** → 设备栈**采纳目标 IP**（厂商实现，非标准）。

| 要点 | 说明 |
|------|------|
| 场景 | 产测 / 首次配置 |
| 命令 | `arp -s` → `ping` / `arping` |
| 局限 | **重启丢失**；不可替代 DHCP；仅可信内网 |

长期：DHCP 保留、串口/Web 写 Flash → [ch06](../chapter06-dhcp-config/study.md)

<a id="ch04-11"></a>

## 4.11 与 ARP 相关的攻击

→ 精读：[4.11-arp-spoof-defense.md](4.11-arp-spoof-defense.md) · [ch18 安全](../chapter18-network-security/study.md)

**根因**：无认证；**最新 ARP 应答覆盖**表项。**范围**：仅**同一广播域**。

| 攻击 | 后果 |
|------|------|
| 单向/双向欺骗 → **MITM** | 窃听、篡改、会话劫持；可组合 **DHCP 劫持** |

**缓解**：**DAI** + DHCP Snooping、**静态 ARP**、**802.1X**、**TLS/IPsec**。  
**开发**：勿信任局域网；敏感通道强制 TLS。  
**家用**：静态 ARP + HTTPS；防火墙（L3）**拦不住** ARP（L2）。

---

<a id="ch04-onepager"></a>

## 一页纸速记 + 易错对比

| 主题 | 一句 | 易错 |
|------|------|------|
| **请求 vs 应答** | 请求=**广播+目标MAC全0+Opcode1**；应答=**单播+填充+Opcode2** | 应答**不广播** |
| **缓存超时** [4.6](4.6-arp-cache-timeout.md) | **软状态**；完整 **~20 min**；**INCOMPLETE ~3 min** | INCOMPLETE 不会存 20 min |
| **代理 ARP** [4.7](4.7-proxy-arp.md) | 路由器**代答目标 IP**，表项「远端 IP→网关 MAC」 | ≠ 正常路由（只 ARP **网关 IP**） |
| **免费 ARP** [4.8](4.8-gratuitous-arp.md) | 广播「本机 IP→本机 MAC」；**ACD** 冲突检测 | **收到回复=冲突**，非握手成功 |
| **帧格式** [4.4](4.4-arp-packet-format.md) | **28 B**、帧头 **0x0806**、**不进 IP** | ARP 不是 IP 层协议 |
| **抓包** [4.5](4.5-arp-tcpdump-example.md) | 无主机：**3 次** who-has → Incomplete | 失败链路层常**静默** |

过滤器：`arp` · `arp.opcode == 1/2` · `eth.type == 0x0806` · 验证：`ip neigh` / `arp -a`

---

<a id="ch04-exam"></a>

## 考点复盘

### 技术本质（三条）

1. **L3 → L2**：逻辑路由必须落实为**下一跳 MAC**；ARP 是 IPv4 以太网上的动态粘合剂。  
2. **动态自适应**：设备上下线、IP 变更时无需手工维护全网 MAC 表。  
3. **信任模型脆弱**：效率来自**局域网互信** → 也是**最主要 L2 安全风险**之一。

### 易混对照

| 问题 | 要点 |
|------|------|
| ARP vs RARP | ARP：IP→MAC；RARP：MAC→IP（少见，DHCP 取代） |
| ARP vs ND | **IPv4/ARP**；**IPv6/ND（ICMPv6）** |
| 请求 vs 应答 | **广播+全0+Op1** vs **单播+填充+Op2**；应答绝不广播 → [4.4 §三](4.4-arp-packet-format.md#ch04-4-request-reply) |
| 代理 ARP vs 默认网关 | 代理**代答目标 IP** → 表项「远端 IP→网关 MAC」；正常跨网段 **ARP 网关 IP** → [4.7](4.7-proxy-arp.md#ch04-7-compare) |
| ARP 失败 vs ICMP | ARP 超时静默；路由/防火墙可能再报 **Host Unreachable** |

### 下一章

- [ch05 IP](../chapter05-ip-protocol/study.md) — 首部、分片、间接交付的完整路径  
- [ch03 §3.1 链路层职能](../chapter03-link-layer/study.md#ch03-1) — ARP 分用入口

---

## Top-Down

- [06_link_layer_and_lan/study.md §6.4.1](../../top_down/06_link_layer_and_lan/study.md#ch6-4)  
- [06 §6.7 Web 请求微观路径](../../top_down/06_link_layer_and_lan/study.md#ch6-7)（DNS 前常需 ARP 网关）

## Lab

- `arp -a` / `ip neigh` 对照本书缓存状态  
- Wireshark：`arp` 过滤器，观察广播请求与单播应答  
- 同一子网 ping 首包延迟（首 ARP 解析）

## Go / Rust

- **Go**：`net.Interface` 看本机 MAC；排障用 `exec` 调 `ip neigh` 或读 `/proc/net/arp`（Linux）  
- **Rust**：`pnet` / 抓包库解析 ARP；容器网络注意**邻居表**与 **hairpin**  
- **实践**：跨子网发包时区分 **dst IP（远端）** 与 **L2 dst MAC（网关）**
