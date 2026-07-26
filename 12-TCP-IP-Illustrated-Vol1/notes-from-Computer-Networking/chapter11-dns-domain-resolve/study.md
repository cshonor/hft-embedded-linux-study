# 第 11 章：名称解析与域名系统（DNS）

> 按书节速记：[11.1](11.1-introduction.md) · [11.2](11.2-domain-space-structure.md) · [11.3](11.3-dns-server-hierarchy.md) · [11.4](11.4-dns-cache.md) · [11.5](11.5-dns-packet-structure.md) · [11.6](11.6-dns-traffic-practices.md) · [11.7](11.7-opendns-dyndns.md) · [11.8](11.8-dns-extensibility.md) · [11.9](11.9-dns-ipv6-transition.md) · [11.10](11.10-local-mdns.md) · [11.11](11.11-ldap-overview.md) · [11.12](11.12-dns-security-threat.md) · [11.13](11.13-summary.md) · [QUICKREF §11](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/05_application_security/ch11_dns.md)）  
> 传输载体：[ch10 UDP/53](../chapter10-udp-ip-fragment/study.md) · 地址：[ch02](../chapter02-ip-address-architecture/study.md) · 自顶向下：[§2.4 DNS](../../top_down/02_application_layer/study.md#ch2-4)

从静态 **hosts** 到分布式 **DNS**，是互联网**可扩展命名**的必然：无单点持有全球库；**解析器**与**名称服务器**协作，将人类可读名映射为 **IPv4/IPv6** 地址。

---

<a id="ch11-1"></a>

## 11.1 引言

### 战略背景

节点规模指数增长 → 中心化 hosts **同步失效** → DNS 提供**去中心化存储**、**分级管理**、对应用**透明**的解析。

### 术语

| 术语 | 说明 |
|------|------|
| **名称解析** | 主机名 → 可路由 IP |
| **映射** | 名称空间 ↔ 地址空间（可多对一负载均衡、一对多别名） |
| **主机名** | 通常对应具体接口，多为树中**叶子** |

### 要点

- **分布式**：无服务器持有全网数据；数据切为**区域（Zone）**  
- **C/S**：**Resolver（解析器）** 为客户端，**Name Server** 提供数据

---

<a id="ch11-2"></a>

## 11.2 DNS 名称空间

倒置**树状**结构；核心机制是**委派（Delegation）** — 子树管理权下放，同级标签可重复（不同父域下）。

### 术语

| 术语 | 说明 |
|------|------|
| **域 (Domain)** | 名称空间中的一棵**子树**，逻辑管理边界 |
| **标签 (Label)** | 点分每一段；单标签 ≤ **63** 字节 |
| **FQDN** | 叶到根的完整名，标准以 **`.`** 结尾（`www.example.com.`） |
| **TLD** | 根下第一层：gTLD（`.com`）、ccTLD（`.cn`） |

### 11.2.1 命名语法

- 单标签 ≤ **63 B**；FQDN（含点）≤ **255 B**  
- 字符：经典 **L-D-H**；现代 **IDN** 国际化

### 易混

| | 域名 | 主机名 |
|--|------|--------|
| 含义 | **行政/管理**边界 | **具体节点** |
| 关系 | 可无主机（仅父节点） | 必属某域 |

### 拓扑（自根向下）

```text
. (根)
├── TLD (.com, .cn, …)
│   └── SLD (example.com)
│       └── 子域 / 主机名 (www, mail, …)
```

---

<a id="ch11-3"></a>

## 11.3 名称服务器与区域

**Zone（区域）** = 某机构**实际托管**的连续名称空间（可小于一个“域”）。

| 术语 | 说明 |
|------|------|
| **委派点** | 父区通过 **NS** 把子区管理权交出 |
| **Primary** | 区域**唯一写**入点 |
| **Secondary** | **区域传输**（AXFR/IXFR）同步副本 |

### 根服务器

- 13 个根 **IP** 字母主机（a–m.root-servers.net）  
- 广泛 **Anycast** → 每 IP 对应全球多物理节点，降延迟、抗 DDoS

### 权威 vs 递归

| | 递归服务器 | 权威服务器 |
|--|------------|------------|
| 角色 | 替客户端**跑腿**迭代查询 | 对本区数据**负责** |
| 典型 | ISP/8.8.8.8/企业递归 | 域名注册商/自托管权威 |

### 递归解析五步

1. 客户端 → 递归器  
2. 递归器 → **根**（得 TLD NS）  
3. → **TLD**（得权威 NS）  
4. → **权威**（得 A/AAAA 等）  
5. 结果返回客户端（可缓存）

---

<a id="ch11-4"></a>

## 11.4 缓存

以**弱一致性**换性能；减轻根/TLD 压力。

| 术语 | 说明 |
|------|------|
| **TTL** | 权威设定的记录生存时间 |
| **负缓存** | 缓存 **NXDOMAIN**，避免反复无效查询 |

### 迁移实践

切 IP 前 **24–48h** 将 TTL **降至 60s** 等 → 全球缓存快速失效。

### 易混

| | 权威应答 | 缓存应答 |
|--|----------|----------|
| 标志 | **AA=1** | 常无 AA，依赖 TTL 过期 |

---

<a id="ch11-5"></a>

## 11.5 DNS 协议

二进制编码；常规 **UDP**，大包/区传用 **TCP**。

### 11.5.1 报文结构

**12 字节首部** + 四段（Question / Answer / Authority / Additional）。

**首部标志（选）**：

| 位 | 含义 |
|----|------|
| **QR** | 0=查询，1=响应 |
| **Opcode** | 0=标准 QUERY |
| **AA** | 权威回答 |
| **TC** | 截断（UDP>512 且无 EDNS0） |
| **RD** | 期望递归 |
| **RA** | 支持递归 |
| **RCODE** | 0 NoError，2 ServFail，3 **NXDOMAIN** … |

### 11.5.2 EDNS0

**OPT 伪 RR** → UDP 可 **>512 B**（如 4096），减少被迫切 TCP。

### 11.5.3 UDP → TCP

1. UDP 查询  
2. 应答过大 → **TC=1** 截断  
3. 客户端改 **TCP 53** 重查完整结果

→ [ch10 UDP](../chapter10-udp-ip-fragment/study.md)

### 11.5.6 核心 RR 类型

| 类型 | 用途 |
|------|------|
| **A** | IPv4 |
| **AAAA** | IPv6 |
| **CNAME** | 别名 → 规范名 |
| **MX** | 邮件；含 **Preference** |
| **NS** | 子区域委派 |
| **SOA** | 区权威起点；Serial/Refresh/Retry/Expire |
| **PTR** | 反向解析 |
| **SRV** | 服务：优先级、权重、端口、主机 |

### 11.5.7–8 动态更新与区传

1. Secondary 查 Primary **SOA Serial**  
2. Serial 更大 → **AXFR**（全量）或 **IXFR**（增量）  
3. **TCP 53** 同步

---

<a id="ch11-6"></a>

## 11.6–11.8 高级特性

### Round Robin

同一名多条 **A/AAAA** → 服务器**轮转**返回 → 简单 DNS **负载均衡**。

### 透明与可扩展

应用通过 **Resolver API**（`getaddrinfo` 等）无需知细节；**Class**（几乎仅 **IN**）与 **Type** 可扩展新记录类型。

---

<a id="ch11-9"></a>

## 11.9 IPv4/IPv6 与 Happy Eyeballs

双栈时并发 **A + AAAA** 查询（RFC 6555 **Happy Eyeballs**）：

1. 并行解析  
2. IPv6 快且通 → 优先 v6  
3. v6 慢/失败 → 快速回退 **IPv4**

→ [ch07 NAT64/DNS64](../chapter07-firewall-nat/study.md#ch07-6)（仅 v6 客户端访问 v4 服务）

---

<a id="ch11-10"></a>

## 11.10–11.11 零配置与 LDAP

| 协议 | 端口 | 特点 |
|------|------|------|
| **mDNS** (RFC 6762) | **5353** | `.local`，Apple/Bonjour |
| **LLMNR** (RFC 4795) | **5355** | Microsoft，更广 TLD |

**LDAP**：企业**目录**（用户/组织/权限）— DNS 偏**快速名→IP**；LDAP 偏**复杂属性**，互补非替代。

→ 组播语境：[ch09](../chapter09-broadcast-multicast/study.md)

---

<a id="ch11-12"></a>

## 11.12 与 DNS 相关的攻击

### 11.12.1 缓存投毒

伪造响应匹配 **Transaction ID** + **源端口**；**Bailiwick** 规则：应答记录须属于**查询域**子树。

**生日攻击**：海量伪造包提高在合法应答前**碰撞**概率。

**缓解**：随机化 ID/端口、**DNSSEC**、0x20 编码、QNAME 最小化。

### 11.12.2 放大 DDoS

伪造受害者源 IP → 小查询（如 ANY）→ **EDNS0 大响应** 打向受害者。

→ [ch10 反射](../chapter10-udp-ip-fragment/study.md#ch10-14) · [ch07](../chapter07-firewall-nat/study.md)

### DDNS（与 ch06）

DHCP 变更 IP 时更新 **A/AAAA** → [ch06 §6.4](../chapter06-dhcp-config/study.md#ch06-4)

---

<a id="ch11-exam"></a>

## 11.13 总结与考点

DNS = 最成功的**分布式数据库**：分层空间 + 缓存 + 紧凑二进制报文。

| 主题 | 一句话 |
|------|--------|
| 委派 | **NS** 切断父区责任 |
| 性能 | **TTL** 与递归缓存 |
| 传输 | 默认 **UDP 53**；TC → **TCP** |
| 双栈 | **Happy Eyeballs** |
| 信任 | **DNSSEC** 签名链（见 ch18） |

### 易混速记

| 问题 | 要点 |
|------|------|
| 递归 vs 迭代 | 客户端常只看见**递归器**；递归器对上游**迭代** |
| 域 vs 区域 | **域**=树子树概念；**区域**=实际托管文件 |
| CNAME 与 A | CNAME **不能**与 NS 等某些记录同存于同名（规范限制） |
| MX 优先级 | 数值**小**优先 |
| 端口 | DNS **53**；mDNS **5353** |

### 下一章

- [ch12 TCP](../chapter12-tcp-basic/study.md)  
- [ch18 安全](../chapter18-network-security/study.md) — DNSSEC、TLS

---

## Top-Down

- [02_application_layer/study.md §2.4](../../top_down/02_application_layer/study.md#ch2-4)

## Lab

- `dig +trace example.com` · `dig +dnssec`  
- Wireshark：`dns` 过滤器，观察 QR/AA/TC、EDNS0  
- 迁移前调低 TTL 并观察 `dig` TTL 倒计时

## Go / Rust

- **Go**：`net.Resolver`；`context` 超时；自定义 `Dial` 到可信递归  
- **Rust**：`trust-dns-resolver` / `hickory-resolver`  
- **K8s**：`CoreDNS`；Service 名集群内 DNS；外网注意 **ndots** 与 search 域
