# Ch 7 §3 IPv4 ARP 协议实现 · ARP in IPv4

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 3. IPv4 中的 ARP 协议实现

**ARP（Address Resolution Protocol，RFC 826）** — 以太网 **0x0806**：**「谁有 IP x.x.x.x？告诉 MAC」**。

源码：`net/ipv4/arp.c` · `net/core/neighbour.c`

---

## `arphdr` 首部

```c
struct arphdr {
    __be16 ar_hrd;    /* 硬件类型 ARPHRD_ETHER */
    __be16 ar_pro;    /* 协议类型 ETH_P_IP */
    unsigned char ar_hln;  /* MAC 长度 6 */
    unsigned char ar_pln;  /* IP 长度 4 */
    __be16 ar_op;     /* ARPOP_REQUEST / REPLY */
    /* 后随：sender hw, sender ip, target hw, target ip */
};
```

| op | 含义 |
|----|------|
| **REQUEST (1)** | 问 target IP 的 MAC |
| **REPLY (2)** | 应答 MAC |

---

## 发送：`arp_solicit()`

```
neighbour 需解析（NUD_INCOMPLETE）
  → arp_solicit(neigh)
       → 构造 ARP REQUEST（广播 ff:ff:ff:ff:ff:ff）
       → 经 dev_queue_xmit 发出
  → 等待 REPLY 或超时重试
```

**排队：** 同时待发 IP 包进 **`neigh->arp_queue`** — REPLY 到达后 **批量 dequeue 发送**。

---

## 接收：`arp_rcv()`

```
以太网 demux (ETH_P_ARP)
  → arp_rcv(skb)
       ├─ REQUEST：若 target 是本机 IP → arp_reply
       └─ REPLY：更新 neighbour ha，唤醒 arp_queue
```

| 处理 | 动作 |
|------|------|
| **REQUEST 问别人** | 可 **代答/转发**（proxy_arp 等） |
| **REQUEST 问本机** | **arp_reply** |
| **REPLY** | **`neigh_update()`** → NUD_REACHABLE |

**安全：** **`arp_ignore` / `arp_announce`** sysctl — 防 **ARP 欺骗/跨网段乱答**（共置常 **严格模式**）。

---

## 与 Gratuitous ARP

主机 **主动发 ARP REPLY/REQUEST 宣告「IP↔MAC」** — **换网卡/接管 VIP** 时 **加速邻居更新**。HFT **主备切换** 场景偶见。

---

← [2. 数据结构](./section-2-核心数据结构-neighbour与neigh_table.md) · [Ch 7](../README.md) · 下一节 [4. NDISC](./section-4-IPv6-NDISC邻居发现.md)
