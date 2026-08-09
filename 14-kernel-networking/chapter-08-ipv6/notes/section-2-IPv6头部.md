# Ch 8 §2 IPv6 头部 · IPv6 Header

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 2. IPv6 头部 (IPv6 Header)

IPv6 主头部 **固定 40 字节** — **无 IHL、无 options 塞入主头**；选项移至 **扩展头**（§3）。

```c
struct ipv6hdr {
    __u8      priority:4, version:4;
    __u8      flow_lbl[3];   /* 流标签 */
    __be16    payload_len;   /* 扩展头 + 上层 payload */
    __u8      nexthdr;       /* 下一头部类型 */
    __u8      hop_limit;     /* 等同 IPv4 TTL */
    struct in6_addr saddr;
    struct in6_addr daddr;
};
```

---

## 相对 IPv4 的关键变化

| 字段 | IPv4 | IPv6 |
|------|------|------|
| 头长 | 20–60 B 可变 | **固定 40 B** |
| **Checksum** | 头部校验和 | **移除** — 路由器 **改 hop_limit 不需重算 IP sum** |
| TTL | `ttl` | **`hop_limit`** |
| 协议 | `protocol` | **`nexthdr`**（可链扩展头） |
| 分片 | 主头 `frag_off` | **分片扩展头**（§3） |

---

## 无 IP 校验和的影响

| 受益者 | 说明 |
|--------|------|
| **软件路由器** | 每跳 **少一次 IP header sum** — 转发更快 |
| **完整性** | 靠 **L2 FCS/CRC** + **TCP/UDP checksum**（及 UDP-Lite 等） |

**HFT：** 内核 IPv6 转发路径 **仍非 tick 主路径**；**UDP 行情** 需确认 **UDP checksum 未 offload 误配** 导致 **静默坏包**。

---

## `nexthdr` 常见值

| 值 | 含义 |
|----|------|
| 6 | TCP |
| 17 | UDP |
| 58 | ICMPv6（含 ND/MLD） |
| 0 | Hop-by-Hop Options |
| 43 | Routing Header |
| 44 | Fragment Header |
| 51 | AH · 50 ESP（IPsec） |

解析：`skb_network_header` → `ipv6hdr` → 按 nexthdr **walk 扩展头链**。

→ sk_buff：[Ch 1 §3](../../chapter-01-introduction/notes/section-3-套接字缓冲区-sk_buff.md)

---

← [1. 地址架构](./section-1-IPv6地址架构.md) · [Ch 8](../README.md) · 下一节 [3. 扩展头](./section-3-扩展头部.md)
