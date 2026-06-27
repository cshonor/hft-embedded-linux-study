# Ch 1 §3 套接字缓冲区 · The Socket Buffer (`sk_buff`)

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 3. 套接字缓冲区 (The Socket Buffer — `sk_buff`)

`sk_buff`（**SKB**）是内核中表示 **单个收/发数据包** 的核心结构：各层 **头部 (headers)** 与 **有效载荷 (payload)** 都在同一块缓冲里，通过 **指针偏移** 区分，而非每层拷贝一整份。

源码：`include/linux/skbuff.h` · 分配/释放走 **slab cache**（`skbuff_head_cache` 等）。

---

## 为什么需要专用 SKB API

**禁止** 把 SKB 当普通字节数组乱摸 — 必须走 **SKB API**，原因：

| 原因 | 说明 |
|------|------|
| **headroom/tailroom** | 向下层 **push 头**、向用户 **pull 头** 需要预留空间 |
| **共享与克隆** | `skb_clone` / `pskb_copy` — 转发、桥接、Netfilter 可能 **零拷贝共享** |
| **DMA 映射** | 驱动 ring 与 SKB 数据区对齐、unmap 生命周期 |
| **checksum 状态** | `ip_summed`、硬件 offload 标志 |

常用访问（原书强调）：

```c
skb_network_header(skb);    // L3 头（IP）
skb_transport_header(skb);  // L4 头（TCP/UDP）
skb_mac_header(skb);        // L2 头（以太网）
skb->data                  // 当前「有效数据」起点（随 push/pull 变）
```

**栈内流转时：** 每上一层 **剥头**（pull/advance），下发时 **加头**（push/reserve）— **同一块 SKB** 在 L2→L4 间传递。

---

## SKB 生命周期（简图）

```
驱动 alloc_skb → DMA 填入 payload
    → netif_receive_skb / NAPI gro
    → L3 ip_rcv → Netfilter PRE_ROUTING
    → 本地：L4 tcp_v4_rcv / udp_rcv → socket receive_queue
    → 用户 read：skb 拷贝到 userspace → kfree_skb

发包反向：socket 写 → alloc_skb → L4 填头 → L3 路由 → L2 dev_queue_xmit → 驱动 DMA
```

---

## HFT 要点

| 主题 | 说明 |
|------|------|
| **每包 alloc/free** | 高 PPS 时 **slab 压力** + cache miss — Ch 14 **per-CPU skb cache**、**GRO/LRO** 合并 |
| **copy_to_user** | 内核栈 **最后一跳拷贝** 到用户 buffer — 零拷贝需 **mmap ring**（PACKET_RX_RING）、**AF_XDP**、DPDK |
| **skb 共享** | **转发/NAT** 路径 clone — 理解 **引用计数** 防 UAF |
| **头部 room** | 恶意/异常包若 room 不足会 **drop** — 抓包工具需知 |

→ Slab 背景：[07-Gorman Ch 8](../../07-Linux-Virtual-Memory-Manager/chapter-08-slab-allocator/) · 用户态 API：[11-UNP](../../11-UNP-Vol1/)

---

← [2. net_device](./section-2-网络设备-net_device.md) · [Ch 1](../README.md) · 下一节 [4. 收发包流转](./section-4-数据包的收发与流转.md)
