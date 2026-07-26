# 第 18 章：安全

> 按书节速记：[18.1](18.1-introduction.md) · [18.2](18.2-security-principles.md) · [18.3](18.3-network-threats.md) · [18.4](18.4-cryptography-basics.md) · [18.5](18.5-pki-certificates.md) · [18.6](18.6-security-layering.md) · [18.7](18.7-eap-8021x.md) · [18.8](18.8-ipsec.md) · [18.9](18.9-tls-dtls.md) · [18.10](18.10-dnssec.md) · [18.11](18.11-dkim.md) · [18.12](18.12-protocol-attacks.md) · [18.13](18.13-summary.md) · [QUICKREF §18](../QUICKREF.md)

> 《TCP/IP 详解》卷1 第 2 版（Fall, 2016）· 精细化学习笔记（同步自 [tcpip_vol1_ed2_notes](../../tcpip_vol1_ed2_notes/05_application_security/ch18_security.md)）  
> DNS：[ch11](../chapter11-dns-domain-resolve/study.md) · 防火墙/NAT：[ch07](../chapter07-firewall-nat/study.md) · 自顶向下：[08_network_security/study.md](../../top_down/08_network_security/study.md)

从 ARPANET **受信模型** → 公共互联网的 **零信任**：安全不是补丁，而是**战略核心**。

---

<a id="ch18-1"></a>

## 18.1 引言

| 类型 | 范围 | 特点 |
|------|------|------|
| **端到端安全** | 对等主机 A↔B | 中间网络**透明**；全程机密/完整 |
| **逐跳安全** | 相邻节点（路由器/交换机） | 链路/段级加固 |

| 层 | 典型机制 |
|----|----------|
| 应用 | DNSSEC、DKIM、TSIG |
| 传输 | **TLS**、**DTLS** |
| 网络 | **IPsec**（VPN，对应用透明） |
| 链路 | **802.1AE MACsec** |

---

<a id="ch18-2"></a>

## 18.2–18.3 基本原则与威胁

### CIA

| 原则 | 威胁示例 |
|------|----------|
| **机密性 (C)** | **窃听 (Snooping)** — 镜像口、无线嗅探 |
| **完整性 (I)** | **篡改 (Tampering)** — 改包再转发 |
| **可用性 (A)** | **DoS/DDoS** — 如 [SYN Flood](../chapter13-tcp-connection-manage/study.md#ch13-8) |

### 欺骗 (Spoofing)

**IP 源地址伪造** → 绕过基于 IP 的信任 → [ch05](../chapter05-ip-protocol/study.md#ch05-7)、BCP38

---

<a id="ch18-4"></a>

## 18.4 基础加密机制

### 对称 vs 非对称

| | 对称（AES） | 非对称（RSA/ECC） |
|--|-------------|-------------------|
| 性能 | **高**，适合 bulk | 开销大 |
| 用途 | 数据加密 | 密钥交换、签名 |
| 密钥 | 分发难 | 公钥公开 |

**ECC**（Ed25519 等）：更短密钥、更小证书、更快握手 → 移动端首选。

### 关键特性

| 术语 | 说明 |
|------|------|
| **PFS** | **ECDHE/DHE** 临时密钥 → 长期私钥泄露**不能**解密历史流量 |
| **Nonce** | 防 **重放** |
| **摘要** | SHA-256 等单向哈希 |

### MAC vs 数字签名

| | MAC | 数字签名 |
|--|-----|----------|
| 密钥 | **共享** | **私钥**签、公钥验 |
| 提供 | 完整性（高效） | 完整性 + **不可抵赖** |

---

<a id="ch18-5"></a>

## 18.5–18.6 证书与协议分层

**X.509** + **PKI**：CA 为**信任锚**；解决「这个公钥属于谁」。

### 协议映射（复盘）

| 层 | 协议 | 架构逻辑 |
|----|------|----------|
| 应用 | DNSSEC, DKIM, TSIG | 域名/邮件**真实性** |
| 传输 | TLS, DTLS | 应用可控的**加密会话** |
| 网络 | IPsec AH/ESP | **透明 VPN** |
| 链路 | MACsec | 硬件级逐跳 |

---

<a id="ch18-7"></a>

## 18.7 网络访问控制与 EAP

**802.1X + EAP**：企业**接入边缘**准入。

| 角色 | 设备 |
|------|------|
| **Supplicant** | 终端 |
| **Authenticator** | 交换机/AP |
| **Authentication Server** | 常 **RADIUS** |

**EAP**：承载框架（MD5 → EAP-TLS 等），不固定单一算法。

**PANA**：在 **IP 层**做接入认证（无 802.1X 环境）。

---

<a id="ch18-8"></a>

## 18.8 IPsec（网络层）

RFC 4301；**IKEv2** 协商密钥；**AH** / **ESP** 保护报文。

### AH vs ESP

| | AH | ESP |
|--|-----|-----|
| 机密性 | 无 | **可加密** |
| 完整性 | 有 | 可选 |
| NAT | **不能**过 NAT（校验含 IP 头） | 可（常配合 **NAT-T**） |

### 传输模式 vs 隧道模式

| 模式 | 加密范围 | 场景 |
|------|----------|------|
| **传输** | 仅 IP **载荷** | 主机↔主机，开销小 |
| **隧道** | **整个**原 IP 报文 + 新外层 IP | **Site-to-Site VPN**，隐藏内网拓扑 |

### IKEv2 与 NAT-T

1. **IKE_SA_INIT**：算法、Nonce、DH  
2. **IKE_AUTH**：身份认证、**Child SA**  

**NAT-T**：路径有 NAT 时 ESP 封装在 **UDP 4500**。

→ 自顶向下：[08 §8.4 IPsec](../../top_down/08_network_security/8.4_ipsec_vpn.md)

---

<a id="ch18-9"></a>

## 18.9–18.11 TLS、DTLS、DNS 与邮件安全

### TLS 1.2 握手（概要）

1. ClientHello / ServerHello（算法）  
2. Server Certificate、Key Exchange  
3. Client Key Exchange、**Change Cipher Spec**  
4. **Finished**（握手校验）

**TLS 1.3**：弱化算法移除；**1-RTT** 握手；**0-RTT** 恢复（注意重放风险）。

→ [08 §8.3 TLS/HTTPS](../../top_down/08_network_security/8.3_tls_https.md)

### DTLS

UDP 上的 TLS 逻辑 + **序号/重传** → VoIP、WebRTC 等。

### DNS 安全三维度

| 机制 | 作用 |
|------|------|
| **DNSSEC** | **RRSIG / DNSKEY / DS** 签名链 → 防缓存污染 → [ch11](../chapter11-dns-domain-resolve/study.md) |
| **TSIG/TKEY** | 共享密钥；**区域传输**认证 |
| **SIG(0)** | 非对称，单条 DNS 消息验证 |

### 邮件

**DKIM**：域名密钥对邮件签名（RFC 6376）。

---

<a id="ch18-12"></a>

## 18.12–18.13 攻击、清单与 RFC

### 典型攻击

| 攻击 | 防御 |
|------|------|
| **MITM** | 握手期替换证书 → **严格证书链**、HSTS、证书钉扎 |
| **重放** | **Nonce**、序列号、TLS 记录层 |

### 部署 Checklist

- [ ] 禁用 **MD5、RC4**、弱 RSA（<2048，宜 3072+）  
- [ ] **HSTS** / 证书钉扎（pinning）  
- [ ] **OCSP/CRL** 吊销检查  
- [ ] IPsec **SA 软硬周期**（Soft/Hard limit）触发重协商  

### 核心 RFC

| RFC | 主题 |
|-----|------|
| 4301 | IPsec 架构 |
| 5246 | TLS 1.2 |
| 8446 | TLS 1.3（扩展） |
| 5996 | IKEv2 |
| 4033 | DNSSEC 导论 |
| 6376 | DKIM |

---

<a id="ch18-exam"></a>

## 架构师总结与考点

**没有万能协议** — 有效防御 = **IPsec 隧道隐蔽** + **TLS 会话强度** + **DNSSEC 基础设施真实** 的组合。

### 易混

| 问题 | 要点 |
|------|------|
| E2E vs 逐跳 | TLS=端到端；IPsec 隧道=网关间；MACsec=链路 |
| AH vs ESP | AH **不过 NAT**；ESP **可加密** |
| TLS vs IPsec | 应用感知 vs 对应用**透明** |
| PFS | 需 **(EC)DHE**，非静态 RSA 密钥交换 |
| DNSSEC | 防**篡改/伪造**，不加密查询内容（除非 DoH/TLS） |

### 全书收束

传输层精读：[ch10–ch17](../chapter09-broadcast-multicast/) · 体系原则：[ch01](../chapter01-overview/study.md)

---

## Top-Down

- [08_network_security/study.md](../../top_down/08_network_security/study.md)

## Lab

- `openssl s_client -connect` / `testssl.sh`  
- `dig +dnssec`、Wireshark TLS 握手  
- `ip xfrm` / strongSwan（IPsec 实验）

## Go / Rust

- **Go**：`crypto/tls`、`x509` 校验；`Configure` 最小 TLS1.3  
- **Rust**：`rustls`、`webpki-roots`  
- **实践**：默认 **TLS 1.3** + 强 cipher；DNS 用 **DoT/DoH** 补机密性
