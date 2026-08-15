# 9.1 动态主机配置协议（DHCP）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · UDP：[§8.2](../chapter-08-transport-layer-tcp-udp/04-udp-protocol.md)

**核心主旨**：应用层自动下发 IP、网关、DNS 等；抓包识别 DORA 与选项 53。

## 核心知识点

### 9.1.1 DHCP 头结构（BOOTP 继承）

| 字段 | 说明 |
|------|------|
| **OpCode** | Boot Request / Boot Reply |
| HW type / len | 如以太网 type 1，长度 6 |
| **Transaction ID** | 随机 ID，匹配请求与应答 |
| Seconds | 自首次请求起秒数 |
| **Client IP** | 请求前常为 `0.0.0.0` |
| **Your (yiaddr)** | 服务器提供的 IP |
| **Server IP / Gateway** | 服务器与网关 |
| **Options** | 扩展；含消息类型等 |

**链路**：UDP **67**（服务器）/ **68**（客户端）；L2 常为广播。

**Wireshark**：`bootp` 或 `dhcp`；`dhcp.option.dhcp == 1` 等。

---

### 9.1.2 DORA 续租（完整获取）

| 阶段 | 包 | 选项 53 | 要点 |
|------|-----|---------|------|
| **D**iscover | 客户端 → 广播 | **1** | `0.0.0.0:68` → `255.255.255.255:67` |
| **O**ffer | 服务器 → 客户端 | **2** | yiaddr、掩码、租期、续租时间 |
| **R**equest | 客户端 → 广播/单播 | **3** | 仍可能 `0.0.0.0`；选项含请求 IP、Server ID |
| **A**CK | 服务器 → 客户端 | **5** | 租约生效 |

```text
Discover → Offer → Request → ACK  (DORA)
```

**过滤器**：`dhcp.option.dhcp == 1`（Discover）… 或按 `bootp.option.dhcp_message_type`

---

### 9.1.3 租约内续租（重启）

- 租约未过期时重启：**跳过 D、O**
- 仅 **Request + ACK** 认领原 IP

---

### 9.1.4 选项与消息类型

| 选项 53 值 | 类型 |
|------------|------|
| 1 | Discover |
| 2 | Offer |
| 3 | Request |
| 5 | ACK |
| 6 | NAK（拒绝） |

选项 53 为**强制**；其余选项承载 DNS、域名、租期等。

---

### 9.1.5 DHCPv6

| 项 | 说明 |
|----|------|
| RFC | 3315；结构适配 **128 位**地址 |
| 流程 | **SARR**：Solicit、Advertise、Request、Reply（对应 DORA） |
| 组播 | 客户端 → `ff02::1:2`；UDP **546/547** |

**Wireshark**：`dhcpv6` · `udp.port==546 or udp.port==547`

> **拓展**：**DHCP Relay（Option 82）** 跨网段时中继以单播转发，giaddr 字段可见；抓包点在中继或服务器侧。

## 抓包/实操记录

| 实验 | 操作 |
|------|------|
| 完整 DORA | 接口 `ipconfig /release` `/renew` 或断网重连 → 过滤 `dhcp` |
| 看 Transaction ID | 同一 DORA 四包 ID 相同 |
| 续租 | 重启网卡，看是否仅 Request/ACK |

```bash
tshark -r cap.pcapng -Y "bootp" -T fields -e frame.number -e bootp.option.dhcp -e bootp.ip.your -e bootp.id
```

## 疑问与总结

- DHCP 依赖 **同广播域** 或中继；VLAN 无中继则拿不到地址。
- 与 [ARP](../chapter-07-network-layer-proto/01-arp-protocol.md) 配合：获 IP 后常跟 ARP 网关。
