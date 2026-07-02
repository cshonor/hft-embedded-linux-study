# Ch 4 §2 协议初始化与接收路径 · Initialization & Receiving IPv4 Packets

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 2. 协议初始化与接收路径 (Initialization & Receiving IPv4 Packets)

IPv4 作为 **以太网 demux 的上层协议**，启动时向 **`dev_add_pack()`** 注册 **`ip_rcv()`** — 所有 **ETH_P_IP** 帧最终进入此函数。

---

## 注册

```text
inet_init() / dev_init 相关
  → dev_add_pack(&ip_packet_type)   /* .func = ip_rcv */
```

---

## 接收主路径

```
网卡/NAPI → netif_receive_skb
  → __netif_receive_skb → 以太网 type=0x0800
  → ip_rcv(skb)
       ├─ 长度/版本/ihl 健全性
       ├─ ip_hdr checksum（软件或已 offload 验证）
       ├─ NF_INET_PRE_ROUTING（Netfilter）
       └─ ip_rcv_finish()
              └─ 路由查找 ip_route_input_*()
                     ├─ 本地 → ip_local_deliver() → L4
                     └─ 转发 → ip_forward()（§7）
```

| 阶段 | 函数 | 要点 |
|------|------|------|
| 入口 | **`ip_rcv()`** | 丢弃畸形包、统计 `InHdrErrors` |
| 钩子 | **PRE_ROUTING** | DNAT、mangle、raw 表 — [Ch 1 §4](../../chapter-01-introduction/notes/section-4-数据包的收发与流转.md) |
| 路由决策 | **`ip_rcv_finish()`** | 结果写入 `skb_dst` — **决定 local vs forward** |
| 本地交付 | **`ip_local_deliver()`** | 可能 **defrag（§6）** → 按 `protocol` 调 L4 |

**重组时机：** 非末片或 MF=1 的包在 **local_deliver 前** 走 **`ip_defrag()`**（§6）。

---

## 路由查找（本章视角）

`ip_route_input_noref()` / `ip_route_input_mc()` 等 — **FIB 查表** 细节在 [Ch 5](../../chapter-05-ipv4-routing-subsystem/)。

本章只需记住：**路由 lookup 输出 `dst_entry`**，含 **input 函数指针**（local/forward/unreachable）。

---

## HFT 要点

| 主题 | 说明 |
|------|------|
| **PRE_ROUTING 规则** | 行情 ingress **iptables/nft** 在此 — 错规则 = 静默 drop |
| **GRO** | 现代在 `ip_rcv` 前可能 **合并 TCP/IP 段** — 降 PPS（Ch 14） |
| **旁路** | DPDK **不经过 `ip_rcv`** — 用户态自解析 IP 头 |

---

← [1. IPv4 头部](./section-1-IPv4头部.md) · [Ch 4](../README.md) · 下一节 [3. 多播接收](./section-3-接收IPv4多播数据包.md)
