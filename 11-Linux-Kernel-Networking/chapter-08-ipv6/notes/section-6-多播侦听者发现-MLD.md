# Ch 8 §6 MLD · Multicast Listener Discovery

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 6. 多播侦听者发现 (MLD)

**MLD** = IPv6 版 **IGMP** — 主机告诉 **本地路由器**：「我监听 **哪些组播组**」。**MLDv2 基于 ICMPv6** 实现（非独立 L4 协议）。

→ IPv4 对照：[note-组播IGMP](../../note-组播IGMP.md) · [Ch 6 §1 多播路由](../../chapter-06-advanced-routing/notes/section-1-多播路由.md)

---

## 与 IGMP 对比

| | IGMP (v2/v3) | MLD (v1/v2) |
|---|--------------|-------------|
| 封装 | IP protocol **2** | **ICMPv6 (58)** |
| 报告 | Membership Report | **Multicast Listener Report** |
| 离开 | Leave (v2) | **Done** 消息 |
| 源过滤 | IGMPv3 | **MLDv2** (S,G) |

---

## 内核操作（概念）

```
setsockopt(IPV6_JOIN_GROUP) / MCAST_JOIN_GROUP
  → inet6_mc_join()
  → 接口加入 ff02::/16 组过滤
  → 发 MLD Report（ICMPv6 131/143 等，视版本）

leave / 接口 down
  → MLD Done / 超时
```

| 消息（ICMPv6 type） | 作用 |
|---------------------|------|
| **130** Query | 路由器查询成员 |
| **131** v1 Report | 旧版报告 |
| **132** Done | 离开组 |
| **143** v2 Report | **MLDv2** 带源列表 |

**构建发送：** `igmp6_send()` / `mld_send_report()` 等 — 经 **`ndisc`/`icmpv6`** 路径。

---

## 数据面

组播包 **ffxx::/8** 到达：

```
ipv6_rcv → 路由（是否本地订阅）
  → ip6_mc_input / udp6 等
```

未 **JOIN** 的组 — **默认丢弃**（同 IPv4）。

---

## HFT

| 主题 | 说明 |
|------|------|
| **IPv6 组播行情** | 需 **MLD JOIN** + 交换机 **MLD Snooping** |
| **纯 IPv4 feed** | 本章 **可跳过** — 看 note-IGMP 即可 |
| **双栈** | 同内容可能 **v4/v6 各一路** — 别 **重复收** |

---

← [5. Rx/转发](./section-5-数据包的接收与转发.md) · [Ch 8](../README.md) · 下一节 [7. Tx/路由](./section-7-IPv6的发送与路由机制.md)
