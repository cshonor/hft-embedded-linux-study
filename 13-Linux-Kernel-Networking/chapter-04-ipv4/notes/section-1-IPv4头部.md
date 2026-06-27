# Ch 4 §1 IPv4 头部 · IPv4 Header

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 1. IPv4 头部 (IPv4 Header)

IPv4 头 **最少 20 字节**；含 **IP Options** 时 **最多 60 字节**（`ihl` 字段以 **4 字节为单位** 计数）。

```c
struct iphdr {
    __u8  ihl:4, version:4;
    __u8  tos;
    __be16 tot_len;
    __be16 id;
    __be16 frag_off;
    __u8  ttl;
    __u8  protocol;
    __sum16 check;
    __be32 saddr;
    __be32 daddr;
    /* options follow if ihl > 5 */
};
```

---

## 核心字段

| 字段 | 作用 |
|------|------|
| **`version`** | 4 |
| **`ihl`** | 头部长度（×4 字节）— 5 → 20B，15 → 60B |
| **`tos` / DSCP+ECN** | 旧 TOS；现代常用 **DSCP**（QoS）、**ECN** 位 |
| **`tot_len`** | **整包** IP 长度（头 + 载荷） |
| **`id`** | 分片 **标识** — 同流各片相同，供重组（§6） |
| **`frag_off`** | **分片偏移** + **DF/MF** 标志 |
| **`ttl`** | 每跳递减；0 → ICMP Time Exceeded（§7） |
| **`protocol`** | 上层协议：**6=TCP、17=UDP、1=ICMP** … |
| **`check`** | **仅头部** 校验和（IPv4 特有） |
| **`saddr` / `daddr`** | 源/目的 IPv4 地址 |

---

## `frag_off` 标志

| 位 | 名 | 含义 |
|----|-----|------|
| **DF** | Don't Fragment | 不允许中间设备分片 — **PMTU 发现** 依赖 |
| **MF** | More Fragments | 除最后一片外均为 1 |
| **offset** | 13 bit | 片在原始 datagram 中的 **8 字节对齐偏移** |

**HFT：** TCP 常 **Path MTU Discovery** — 中间链路 MTU 小于包长且 **DF=1** 时，需 **ICMP Fragmentation Needed**（[Ch 3 §1](../../chapter-03-icmp/notes/section-1-ICMPv4的实现与消息流转.md)）；**无 ICMP 则 PMTU 黑洞**。

---

## 与 sk_buff 的关系

- **`skb_network_header(skb)`** → `struct iphdr *`
- L4 头在 IP 头之后：`ip_hdr(skb) + (ihl * 4)`
- **硬件 offload**：`skb->ip_summed`、`CHECKSUM_PARTIAL` — 网卡填 **`check`**

→ 基础：[Ch 1 §3 sk_buff](../../chapter-01-introduction/notes/section-3-套接字缓冲区-sk_buff.md)

---

← [Ch 4](../README.md) · 下一节 [2. 接收路径](./section-2-协议初始化与接收路径.md)
