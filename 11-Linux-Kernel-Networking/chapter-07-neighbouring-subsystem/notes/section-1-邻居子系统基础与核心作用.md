# Ch 7 §1 邻居子系统基础与核心作用

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 1. 邻居子系统基础与核心作用

L3 路由决定 **「下一跳 IP / 出接口」**；真正 **在以太网上发帧** 还需要 **目的 MAC**。邻居子系统完成 **同一链路上的 L3→L2 映射**。

---

## 为什么需要邻居层

```
ip_output 已知：下一跳 GW = 192.168.1.1（或目的即在同网段主机）
  → 仍缺：eth_dst = ??:??:??:??:??:??
  → 邻居子系统：ARP/ND 查询 → 得到 MAC → 填 ethhdr
  → dev_queue_xmit
```

| 层 | 问题 | 谁答 |
|----|------|------|
| **L3** | 包往哪个 **IP** 走？ | FIB / 路由（Ch 5） |
| **L2** | 帧发给哪个 **MAC**？ | **邻居子系统** |

**跨路由器：** 只解析 **下一跳 GW** 的 MAC — **不是** 最终目的 IP 的 MAC（若目的在远端）。

---

## IPv4 vs IPv6

| | IPv4 | IPv6 |
|---|------|------|
| 协议 | **ARP**（独立以太网 type 0x0806） | **NDISC** — 封装在 **ICMPv6** 内 |
| 广播 | ARP **请求广播** | NS 多播到 ** solicited-node ** |
| 表 | **`arp_tbl`** | **`nd_tbl`** |

→ ARP 详 §3 · ND 详 §4 · [Ch 3 §2 ICMPv6/ND](../../chapter-03-icmp/notes/section-2-ICMPv6的扩展与变化.md)

---

## 在发送路径中的位置

[Ch 4 §5](../../chapter-04-ipv4/notes/section-5-发送IPv4数据包.md)：

```
ip_finish_output → neigh_resolve_output / dev_hard_start_xmit
                      ↑
                 neighbour 缓存 hit → 直接发
                 miss → arp_solicit / ndisc，包进 arp_queue
```

**首包延迟：** 无缓存时 **等待 ARP/NS 响应** — HFT 常用 **`ip neigh add … lladdr … nud permanent`** **预热**。

---

## HFT 要点

| 场景 | 建议 |
|------|------|
| **固定共置拓扑** | **静态邻居** — 避免 ARP 超时与 **NUD STALE** 重探 |
| **双 GW ECMP** | 每个 **nexthop IP** 独立 **neighbour** 项（Ch 6） |
| **旁路 DPDK** | 内核邻居表 **不服务** PMD 路径 — 用户态 **自维护 MAC** |

---

← [Ch 7](../README.md) · 下一节 [2. 数据结构](./section-2-核心数据结构-neighbour与neigh_table.md)
