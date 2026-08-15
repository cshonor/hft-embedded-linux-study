# 7.1 地址解析协议（ARP）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 对照：[§1.3 广播](../chapter-01-network-basics/03-traffic-classification.md) · [ARP 污染 §2.3.4](../chapter-02-traffic-monitor/03-sniff-on-switched-network.md)

**核心主旨**：IPv4 同网段通信前，将 **IP → MAC**；Wireshark 中识别 Request/Reply 与 Gratuitous ARP。

## 核心知识点

### 寻址依赖

| 场景 | 需要的地址 |
|------|------------|
| 跨网段 | 逻辑地址（IP）→ 默认网关 MAC |
| 同网段直连 | 目标 **MAC**（交换机 CAM 转发） |
| MAC 未知 | 先 **ARP** 在局域网查询 |

### ARP 报文类型与头部

仅两种：**Request（1）**、**Reply（2）**。

| 字段 | 含义 |
|------|------|
| Hardware type | 如 `1` = 以太网 |
| Protocol type | 如 `0x0800` = IPv4 |
| HW / Proto addr len | MAC 6 字节、IP 4 字节 |
| **Opcode** | **1** 请求 · **2** 应答 |
| Sender MAC/IP | 发送方 |
| Target MAC/IP | 目标方 |

---

### 7.1.2 ARP 请求（Request）

| 层 | 内容 |
|----|------|
| **以太网 DA** | `ff:ff:ff:ff:ff:ff`（**广播**） |
| ARP Target IP | 要解析的 IP |
| ARP Target MAC | `00:00:00:00:00:00`（未知占位） |
| 非目标 IP 主机 | 驱动丢弃，不应答 |

**Wireshark**：`arp.opcode == 1` 或过滤器 `arp`。

---

### 7.1.3 ARP 响应（Reply）

| 项 | 说明 |
|----|------|
| Opcode | **2** |
| 寻址 | 源/目的 **IP 与 MAC 对调** |
| 链路 | **单播** 回请求方 |
| Target MAC | 填入真实 MAC |

**Wireshark**：`arp.opcode == 2`

---

### 7.1.4 无偿 ARP（Gratuitous ARP）

| 特征 | 说明 |
|------|------|
| 定义 | **未先被询问** 即发出的 ARP 广播 |
| 关键 | **Sender IP == Target IP**（同一 IP 宣告自己的 MAC） |
| 用途 | IP/MAC 变更、重启后**刷新**全网 ARP 缓存；负载均衡接管 VIP |
| 排障 | 若见异常 Gratuitous，排查是否 ARP 欺骗或双机同 IP |

> **拓展**：防御 ARP 欺骗 → 动态 ARP 检测（DAI）、静态绑定、802.1X；抓包见 [§2.3.4](../chapter-02-traffic-monitor/03-sniff-on-switched-network.md)。

## 抓包/实操记录

| 实验 | 步骤 |
|------|------|
| 观察 ARP 过程 | 清 ARP 缓存后 `ping` 同网段主机 → 先 Request 广播，再 Reply 单播 |
| 过滤器 | `arp` · `arp.opcode==1` · `arp.opcode==2` |
| Gratuitous | 过滤 `arp.is_gratuitous`（字段视版本）或目视 Sender IP = Target IP |
| 列 | 添加 `arp.src.proto_ipv4`、`arp.dst.proto_ipv4`、`arp.src.hw_mac` |

```bash
tshark -r cap.pcapng -Y "arp" -T fields -e frame.number -e arp.opcode -e arp.src.proto_ipv4 -e arp.src.hw_mac
```

## 疑问与总结

- ARP 只解决 **IPv4 同广播域**；跨网段 ARP 的是**网关 MAC**。
- IPv6 用 **NDP（ICMPv6）**，不用 ARP → [§7.2.2](./03-ipv6-protocol.md)。
