# Ch 7 §4 IPv6 NDISC · Neighbour Discovery in IPv6

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 4. IPv6 中的 NDISC（邻居发现）

IPv6 **无 ARP 帧** — **邻居发现 (Neighbor Discovery)** 用 **ICMPv6** 消息，由 **`ndisc_rcv()`** 统一入口分发。

源码：`net/ipv6/ndisc.c` · 表 **`nd_tbl`**

---

## 五种主要 ICMPv6 ND 消息

| 类型 | 名 | 作用 |
|------|-----|------|
| **133** | **Router Solicitation (RS)** | 主机请求 **路由器通告** |
| **134** | **Router Advertisement (RA)** | 路由器 **前缀/默认路由/标志** |
| **135** | **Neighbor Solicitation (NS)** | **地址解析**（类似 ARP 问） |
| **136** | **Neighbor Advertisement (NA)** | **地址解析应答** / 宣告 |
| **137** | **Redirect** | 同链路 **更优下一跳**（类比 ICMPv4 redirect） |

**L2 封装：** 仍为以太网 **IPv6 + ICMPv6** — 不是独立 eth type。

---

## `ndisc_rcv()` 分发

```
icmpv6_rcv → type 133–137
  → ndisc_rcv(skb)
       ├─ RS / RA → 路由器发现、SLAAC 参数
       ├─ NS / NA → neighbour 创建/更新
       └─ Redirect → 路由/exception 更新
```

**NS 寻址：** 目的 IPv6 为 **被查询地址对应的 solicited-node 多播** — **降低广播域噪声**（相对 ARP 全广播）。

**NA 标志：** **Router / Solicited / Override** — 决定 **是否覆盖已有缓存**。

---

## 与 ARP 对照

| | ARP | NDISC |
|---|-----|-------|
| 载体 | 独立 ARP 帧 | **ICMPv6** |
| 请求扩散 | 广播 | **solicited-node 多播** |
| 额外功能 | 较少 | **RA/RS、Redirect、DAD** |

→ ICMPv6 总览：[Ch 3 §2](../../chapter-03-icmp/notes/section-2-ICMPv6的扩展与变化.md) · IPv6 全章：[Ch 8](../../chapter-08-IPv6.md)

---

← [3. ARP](./section-3-IPv4-ARP协议实现.md) · [Ch 7](../README.md) · 下一节 [5. DAD](./section-5-重复地址检测-DAD.md)
