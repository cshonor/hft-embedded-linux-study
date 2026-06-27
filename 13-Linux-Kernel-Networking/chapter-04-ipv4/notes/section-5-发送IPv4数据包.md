# Ch 4 §5 发送 IPv4 数据包 · Sending IPv4 Packets

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 5. 发送 IPv4 数据包 (Sending IPv4 Packets)

L4 构造 payload 后，经 **IPv4 层** 加头、路由、可能分片，最终 **`dev_queue_xmit`**。两条经典 API 路径对应 **不同 L4 分片策略**。

---

## 两条发送路径

| 路径 | 典型协议 | 行为 |
|------|----------|------|
| **`ip_queue_xmit()`** | **TCP** | L4 已按 **MSS/PMTU** 切好段；IPv4 头 **直接挂 skb** 发出 |
| **`ip_append_data()` + `ip_push_pending_frames()`** | **UDP、ICMP** | **累积** 多个 L4 片段到同一 IP datagram，再 **一次性 push** |

```
TCP:  tcp_transmit_skb → ip_queue_xmit → __ip_local_out → POST_ROUTING → dev_xmit

UDP:  udp_sendmsg → ip_append_data (可能多次)
                   → ip_push_pending_frames → ip_send_skb
```

---

## UDP 与 corking

**软木塞 (cork)：** `setsockopt(TCP_CORK)` 对 TCP；UDP 有 **`UDP_CORK`**（及现代 **`MSG_MORE`** 语义）— **延迟 push** 以 **合并小包**、减 header overhead。

| | 未 cork | cork 启用 |
|---|---------|-----------|
| 行为 | 每 sendto 可能独立 IP 包 | 缓冲到 push/cork 关闭 |
| 锁 | 快速路径尝试 **无锁**（per-socket） | 合并后单次_xmit |
| HFT | **低延迟** 常 **disable cork**、固定报文长 | 批量非 tick 路径可用 |

**无锁快速路径：** 3.9 时代 UDP 在 **已知路由缓存命中** 时走 **`ip_build_and_send_pkt`** 简化路径 — 现代仍有 **ufo/gso** 优化（Ch 14）。

---

## 出站 Netfilter 与邻居

```
ip_local_out / __ip_local_out
  → NF_INET_LOCAL_OUT
  → ip_output / ip_finish_output
  → 邻居子系统解析 MAC（[Ch 7](../../chapter-07-邻居子系统.md)）
  → dev_queue_xmit
```

**POST_ROUTING：** SNAT、mark、mangle — 影响 **源地址/路由**.

---

## HFT 要点

- **TCP 发单** — `ip_queue_xmit` 热路径；配合 **TSO/GSO** 减 CPU（Ch 14）。
- **UDP 行情** — 注意 **包长 vs MTU**；无 cork 时 **每 tick 一包** 最简单可预测。
- **静态路由 + 固定 dst cache** — 减 **`ip_route_output` 慢路径**。

→ L4 深读：[Ch 11](../../chapter-11-第4层协议.md)

---

← [4. IP 选项](./section-4-IP选项.md) · [Ch 4](../README.md) · 下一节 [6. 分片与重组](./section-6-分片与重组.md)
