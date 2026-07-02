# Ch 8 §3 扩展头部 · Extension Headers

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 3. 扩展头部 (Extension Headers)

IPv6 用 **链式扩展头** 替代 IPv4 **IP Options** — **中间路由器默认只处理主头 + 逐跳头**，**其余扩展头在目的主机处理**，提升 **转发效率**。

---

## 处理规则

| 扩展头 | 中间路由器 | 目的主机 |
|--------|------------|----------|
| **Hop-by-Hop Options** | **必须处理**（紧跟 IPv6 头后） | 处理 |
| **Routing / Fragment / AH / ESP / Dest Options** | **通常忽略**（仅转发） | **处理** |

```
[ IPv6 40B ][ Hop-by-Hop? ][ Routing? ][ Fragment? ][ AH/ESP? ][ TCP/UDP ]
                  ↑
           唯一每跳必看（若存在）
```

---

## 常见扩展头

| 类型 | 作用 |
|------|------|
| **Hop-by-Hop** | Router Alert、Jumbo payload 等 **稀有用** |
| **Routing (RH)** | 源路由（**生产常禁**） |
| **Fragment** | **分片信息** — only **源主机分片** |
| **AH / ESP** | **IPsec**（[Ch 10](../../chapter-10-ipsec/)） |
| **Destination Options** | 目的端选项 |

---

## 分片策略（与 IPv4 对比）

| | IPv4 | IPv6 |
|---|------|------|
| 谁分片 | 主机 **或** 路由器 | **仅发送主机** |
| 中间 MTU 更小 | 路由器可 ICMP **需要分片** | 路由器发 **Packet Too Big** ICMPv6 → **源端 PMTUD 再发** |
| 路由器分片 | 允许（无 DF 时） | **不允许** |

**HFT：** IPv6 **路径 MTU 发现更强制** — 中间 **drop ICMPv6 PTB** = **黑洞**，与 IPv4 PMTU 问题 **同构**（[Ch 3](../../chapter-03-icmp/)）。

---

## 内核解析

`ipv6_rcv()` → **`ipv6_parse_exthdrs()`** / `skb_pull` 链 — **PRE_ROUTING 前** 至少解析 **Hop-by-Hop**；**本地交付** 前解析到 **L4 nexthdr**。

---

← [2. IPv6 头部](./section-2-IPv6头部.md) · [Ch 8](../README.md) · 下一节 [4. SLAAC](./section-4-地址自动配置-SLAAC.md)
