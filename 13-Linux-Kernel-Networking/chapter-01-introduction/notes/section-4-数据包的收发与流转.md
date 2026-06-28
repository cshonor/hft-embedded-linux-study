# Ch 1 §4 数据包的收发与流转 · Receiving and Transmitting Packets

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 4. 数据包的收发与流转 (Receiving and Transmitting Packets)

每个 **入站或出站** 包都要经过 **路由子系统** 决策：**本地交付 (local delivery)**、**转发 (forward)**、还是 **丢弃**；路径上还会被 **Netfilter 钩子**、**IPsec/NAT**、**分片/重组** 等修改。

---

## 路由查找

| 结果 | 行为 |
|------|------|
| **本地主机** | 交给 L4（TCP/UDP/RAW）或 **raw/local** 处理 |
| **转发** | 选 **出接口** → L2 重新封装发送（**不经过** 本机 socket） |
| **不可达** | 丢弃或 ICMP 差错（→ [Ch 3 ICMP](../../chapter-03-icmp/)） |

路由表实现随版本演进（3.9：**FIB hash/ trie**；现代：**FIB multipath、BPF fib lookup**）— 细节见 [Ch 5](../../chapter-05-ipv4-routing-subsystem/) · [Ch 6](../../chapter-06-advanced-routing/)。

---

## Netfilter 钩子（入站关键路径）

**Netfilter** 在栈内固定点插入 **钩子 (hooks)**；内核模块 / `iptables` / `nftables` 注册 callback，返回 verdict：

| Verdict | 含义 |
|---------|------|
| `NF_ACCEPT` | 继续正常流转 |
| `NF_DROP` | 静默丢弃 |
| `NF_STOLEN` | 钩子已接管 SKB |
| `NF_QUEUE` | 交用户态队列 |
| `NF_REPEAT` | 重新走钩子 |

**入站 IPv4 典型顺序（简化）：**

```
NIC → L2
  → NF_INET_PRE_ROUTING     ← 路由前：DNAT、raw 表、mangle
  → 路由决策（本地 vs 转发）
  → 若本地：NF_INET_LOCAL_IN  ← filter INPUT
  → L4 协议处理
  → socket 队列
```

出站：`LOCAL_OUT` → `POST_ROUTING`（SNAT、选出口）→ 驱动。

> **HFT：** 共置机 **filter/nat 规则过多** 会增 **每包 CPU**；低延迟环境常 **精简 iptables**、**固定路由**，或用 **XDP 早 drop**（→ [04-BPF Ch10 XDP](../../04-BPF-Performance-Tools/chapter-10-networking/)）。

---

## 路径上还可能发生什么

| 变化 | 场景 |
|------|------|
| **NAT / IPsec** | 改 IP/端口或加密封装（[Ch 9](../../chapter-09-netfilter/) · [Ch 10 IPsec](../../chapter-10-ipsec/)） |
| **丢弃** | 防火墙、rp_filter、TTL 耗尽 |
| **ICMP 差错** | 不可达、需要分片等 |
| **分片 / 重组** | MTU 小于包长 — **重组成本高**，HFT 常 **PMTUD + 避免分片** |
| **校验和** | 软件计算或 **硬件 offload**（`CHECKSUM_COMPLETE` / `UNNECESSARY`） |

---

## 收 vs 发 对照

```
【收包】
  硬件 DMA → 驱动 build skb → NAPI poll
    → netif_receive_skb → __netif_receive_skb_core
    → 协议 demux（ETH_P_IP → ip_rcv）
    → routing + Netfilter → L4 → socket backlog

【发包】
  write/sendmsg → sock_alloc_send_skb
    → L4 构段（TCP 可能 cork/GSO）
    → ip_queue_xmit → 邻居子系统解析 MAC
    → dev_queue_xmit → qdisc → 驱动 xmit
```

→ 邻居 ARP/ND：[Ch 7](../../chapter-07-neighbouring-subsystem/) · L4 队列与 backlog：[Ch 11](../../chapter-11-layer-4-protocols/)

---

## HFT checklist

- [ ] 能画出 **PRE_ROUTING → 路由 → LOCAL_IN → TCP** 顺序
- [ ] 知道 **转发路径不经 socket**
- [ ] 理解 **NF_DROP** 与 **silent drop** 对排查 latency spike 的意义
- [ ] 生产路径 **避免 IP 分片**、确认 **checksum offload** 状态

---

← [3. sk_buff](./section-3-套接字缓冲区-sk_buff.md) · [Ch 1](../README.md) · 下一节 [5. 开发模式](./section-5-Linux内核网络开发模式.md)
