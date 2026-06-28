# Ch 4 §7 数据包转发 · Forwarding

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 7. 数据包转发 (Forwarding)

非本机目的地址、且路由判定为 **forward** 的包由 **`ip_forward()`** 处理 — **路由器/网关** 核心路径。共置 **交易主机通常 `ip_forward=0`**，但 **理解转发** 有助于 **网络拓扑与 ICMP 行为** 排查。

---

## `ip_forward()` 主逻辑

```
ip_forward(skb)
  ├─ ip_decrease_ttl()     /* ttl--；为 0 → ICMP Time Exceeded，drop */
  ├─ 策略 / xfrm / secmark 检查
  ├─ MTU 检查：
  │     超长 + DF → ICMP Fragmentation Needed
  │     超长 + !DF → ip_fragment() 后转发
  ├─ 头部 checksum 更新（ttl 变了必须重算 check）
  └─ ip_forward_finish() → ip_output → 出接口
```

| 步骤 | 说明 |
|------|------|
| **TTL 递减** | **`ip_decrease_ttl()`** — 防路由环路 |
| **TTL=0** | 丢弃 + **ICMP 11**（traceroute 依赖） |
| **MTU + DF** | **ICMP 3.4** — PMTUD 关键 |
| **校验和** | IPv4 **头 check** 重算；payload 不变 |
| **转发钩子** | **`NF_INET_FORWARD`** — filter FORWARD 链 |

---

## 与本地交付对比

```
                    ip_rcv_finish 路由结果
                           │
           ┌───────────────┴───────────────┐
           ▼                               ▼
   ip_local_deliver                  ip_forward
   → TCP/UDP/ICMP                    → 另一 net_device
   → socket                          → 不经本机 L4
```

**HFT 主机：** 默认 **不做 IP 转发** — 误开 **`net.ipv4.ip_forward=1`** 可能改变 **安全边界**。

---

## sysctl 与运维

| sysctl | 含义 |
|--------|------|
| **`net.ipv4.ip_forward`** | 是否允许 IPv4 转发 |
| **`net.ipv4.conf.all.forwarding`** |  per-interface 转发 |
| **`rp_filter`** | 反向路径过滤 — 与 ** asymmetric routing** 相关 |

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | **`iphdr`、DF/MF、id、ttl、protocol** |
| §2 | **`ip_rcv` → PRE_ROUTING → route → local/forward** |
| §3 | **多播 `ip_route_input_mc`、MFC、`ip_mr_input`** |
| §4 | **IP options 编译/构建** — 生产罕见 |
| §5 | **`ip_queue_xmit` vs append/push** — TCP/UDP |
| §6 | **`ip_fragment` / `ip_defrag`** — 避免分片 |
| §7 | **`ip_forward`、TTL、MTU、checksum** |

---

## 相关章节

- 下一章：[Ch 5 IPv4 路由子系统](../../chapter-05-ipv4-routing-subsystem/)
- 转发钩子：[Ch 9 Netfilter](../../chapter-09-Netfilter.md)
- 组播：[note-组播IGMP](../../note-组播IGMP.md)

---

← [6. 分片与重组](./section-6-分片与重组.md) · [Ch 4](../README.md)
