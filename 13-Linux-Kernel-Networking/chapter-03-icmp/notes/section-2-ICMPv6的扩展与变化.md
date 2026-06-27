# Ch 3 §2 ICMPv6 的扩展与变化

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 2. ICMPv6 的扩展与变化

**ICMPv6**（IPv6 next header **58**）职责 **远大于** ICMPv4：除 **差错/诊断** 外，**邻居发现 (ND)** 与 **MLD** 也 **直接基于 ICMPv6** — 分别取代 IPv4 的 **ARP** 与 **IGMP**。

源码：`net/ipv6/icmpv6.c` · `net/ipv6/ndisc.c` · `net/ipv6/mcast.c` 等

---

## 消息类型编码规则

| 范围 | 最高位 | 类别 |
|------|--------|------|
| **0 – 127** | 0 | **差错消息** |
| **128 – 255** | 1 | **信息消息** |

与 ICMPv4「type<128 差错」思路类似，但 **ND/MLD 占大量 type** — ICMPv6 是 IPv6 **控制平面主干**。

---

## 核心收发

| 方向 | 函数 | 说明 |
|------|------|------|
| **收** | `icmpv6_rcv()` | 解析 type → 分派 ND/MLD/echo/差错 handler |
| **发** | `icmpv6_send()` | 栈内异常生成 ICMPv6 差错（类比 `icmp_send`） |

**Echo Request/Reply** — 仍支撑 **`ping -6`**；差错类用于 **不可达、包过大、超时** 等。

---

## ND — 取代 ARP

| IPv4 | IPv6 |
|------|------|
| ARP 独立协议 | **NDP**（Neighbor Discovery）— **ICMPv6 type 133–137** |
| `arp_rcv` | **`ndisc_rcv`** — Router Solicitation/Advertisement、Neighbor Solicitation/Advertisement |

功能：**链路层地址解析**、**路由器发现**、**重复地址检测 (DAD)**。

→ 深读：[Ch 7 邻居子系统](../../chapter-07-邻居子系统.md) · [Ch 8 IPv6](../../chapter-08-IPv6.md)

---

## MLD — 取代 IGMP

**组播侦听者发现 (MLD)** — ICMPv6 **type 130+**，管理 IPv6 **组播组成员**。

→ HFT 组播行情：[note-组播IGMP](../../note-组播IGMP.md)（IPv4 IGMP + IPv6 MLD 对照）

---

## ICMPv4 vs ICMPv6 一表

| | ICMPv4 | ICMPv6 |
|---|--------|--------|
| 差错 + ping | ✓ | ✓ |
| 地址解析 | ARP（独立） | **ND（ICMPv6 内）** |
| 组播成员 | IGMP | **MLD（ICMPv6 内）** |
| 内核入口 | `icmp_rcv` / `icmp_send` | `icmpv6_rcv` / `icmpv6_send` |

**HFT：** 纯 IPv4 共置机 **以本章 §1 为主**；双栈或组播 **IPv6** 需 ND/MLD + §2。

---

← [1. ICMPv4](./section-1-ICMPv4的实现与消息流转.md) · [Ch 3](../README.md) · 下一节 [3. Ping 套接字](./section-3-ICMP套接字-Ping-Sockets.md)
