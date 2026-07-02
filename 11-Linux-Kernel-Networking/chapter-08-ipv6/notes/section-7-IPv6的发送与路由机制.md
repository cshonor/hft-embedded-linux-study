# Ch 8 §7 发送与路由 · Tx Path & Routing

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 7. IPv6 的发送与路由机制 (Tx Path & Routing)

IPv6 **发送 API** 与 IPv4 **镜像**：TCP 类走 **`ip6_xmit()`**，UDP/RAW 走 **`ip6_append_data()` + push** — 见 [Ch 4 §5](../../chapter-04-ipv4/notes/section-5-发送IPv4数据包.md)。

---

## 发送路径

```
TCP:  tcp_v6_xmit → ip6_xmit → ip6_local_out → POST_ROUTING → neigh → dev_xmit
UDP:  udp_sendmsg → ip6_append_data → ip6_push_pending_frames
```

| 函数 | 用户 |
|------|------|
| **`ip6_xmit()`** | TCP、SCTP 等 **已分片/分段** 的 L4 |
| **`ip6_append_data()`** | UDP、RAW **组包** |
| **`ip6_local_out()`** | Netfilter LOCAL_OUT |

**PMTU：** IPv6 **`ip6_sk_update_pmtu()`** — 收到 **Packet Too Big** 后 **更新 dst metrics**。

---

## 路由：FIB6

| IPv4 | IPv6 |
|------|------|
| **`flowi4`** 查找键 | **`flowi6`** |
| **`rtable` / `dst`** | **`rt6_info`** + `dst_entry` |
| **`fib_lookup`** | **`fib6_lookup`** / `ip6_route_output` |
| 策略路由 `fib_rules` | **同样支持** `ip -6 rule` |

```bash
ip -6 route show
ip -6 rule show
ip -6 route get 2001:db8::1
```

**表：** main、local、default — 与 [Ch 6 §2 策略路由](../../chapter-06-advanced-routing/notes/section-2-策略路由.md) **同构**。

---

## 邻居与 L2

出站仍经 **`nd_tbl`** — [Ch 7](../../chapter-07-neighbouring-subsystem/) **`neigh_resolve_output`**，**NS/NA** 替 ARP。

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | **无广播**；link-local / ::1 |
| §2 | **40B 头、无 IP checksum** |
| §3 | **扩展头、仅源端分片** |
| §4 | **SLAAC + RS/RA + DAD** |
| §5 | **`ipv6_rcv` → local/forward** |
| §6 | **MLD = ICMPv6 版 IGMP** |
| §7 | **`ip6_xmit` / `flowi6` / fib6** |

---

## 相关章节

- 下一章：[Ch 9 Netfilter](../../chapter-09-netfilter/)
- IPv4 平行：[Ch 4](../../chapter-04-ipv4/) · [Ch 5](../../chapter-05-ipv4-routing-subsystem/)

---

← [6. MLD](./section-6-多播侦听者发现-MLD.md) · [Ch 8](../README.md)
