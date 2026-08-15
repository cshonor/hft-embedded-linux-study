# 7.2.2 互联网协议第 6 版（IPv6）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：128 位地址、精简头部、NDP 替代 ARP、主机侧分片与隧道。

## 核心知识点

### 地址表示

| 项 | 说明 |
|----|------|
| 长度 | **128 位**，8 组 16 进制，用 `:` 分隔 |
| 压缩 | 连续 0 可用 **`::`**（每地址仅一次） |
| 类型 | **单播**、**任播**、**组播**（无广播） |

### 常用地址类型

| 类型 | 特征 | 示例前缀 |
|------|------|----------|
| **Link-local** | 仅本链路；固定前缀 | `fe80::/10`（常见 `fe80::…`） |
| **全局单播** | 前 3 位 `001` | 全球路由前缀 + 子网 ID + 接口 ID |
| **接口 ID** | 常由 MAC **EUI-64** 生成 | MAC 拆两半，中间插 `ff:fe`，U/L 位翻转 |

### IPv6 头部（相对 IPv4）

| 变更 | 说明 |
|------|------|
| 固定主头 | 约 40 字节，无 IPv4 式可变选项 |
| **Hop Limit** | 等同 **TTL** 语义 |
| **Next Header** | 指示上层或**扩展头**链（TCP/UDP/ICMPv6/分片头等） |
| 无头部校验和 | L3 完整性依赖下层或上层 |

### 地址解析：NDP（替代 ARP）

| 项 | 说明 |
|----|------|
| 协议 | **邻居发现（NDP）** over **ICMPv6** |
| 请求 | **Neighbor Solicitation**（Type **135**） |
| 通告 | **Neighbor Advertisement**（Type **136**） |
| 机制 | 多播 solicited-node 等（细节见 ICMPv6 节） |

**Wireshark**：`icmpv6.type == 135` / `136`；或 `ipv6.nd`。

### 分片（与 IPv4 重大差异）

| IPv4 | IPv6 |
|------|------|
| 路由器可分片 | **中间路由器不分片** |
| | 过大 → **ICMPv6 Packet Too Big** |
| | 源主机用 **IPv6 Fragment Header** 分片；常先做 **PMTUD** |

### 与 IPv4 互通

隧道（6in4、ISATAP 等）在 IPv4 网上承载 IPv6；抓包可见外层 IPv4 + 内层 IPv6。

> **拓展**：双栈主机同抓包可见 `ip` 与 `ipv6`；Wireshark 按 Next Header 链解析扩展头。

## 抓包/实操记录

| 过滤器 | 用途 |
|--------|------|
| `ipv6` | 所有 IPv6 |
| `ipv6.addr == fe80::1` | 某地址 |
| `icmpv6` | 含 NDP、PMTUD、Ping6 |
| `ipv6.route` 等 | 视实验环境 |

`ping -6` 或 `ping6` 同网段抓 **NS/NA** 替代 ARP 过程。

## 疑问与总结

- 看不到 ARP 不代表无地址解析，应找 **ICMPv6 135/136**。
- Link-local 不出本链路；上网需全局地址或 NAT64/DNS64 等（环境相关）。
