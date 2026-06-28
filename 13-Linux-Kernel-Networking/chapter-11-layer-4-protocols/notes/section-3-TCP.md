# Ch 11 §3 TCP · Transmission Control Protocol

> **Linux Kernel Networking** · Rami Rosen · **精读 🔴**

### 3. TCP (Transmission Control Protocol)

**TCP** — **面向连接、可靠、字节流**；**序列号/ACK、重传、拥塞控制、流量控制**。互联网与 **交易所发单/FIX** 的 **默认 L4**。

---

## TCP 头部（概念）

```
├─ src/dst port ─┤
├─ seq number ───┤
├─ ack number ───┤
│ off │ flags │ win │   ← SYN ACK FIN RST PSH URG
├─ checksum ─────┤
├─ urgent ptr ───┤
│   options (MSS, window scale, SACK, timestamps) │
```

| 标志 | 握手/ teardown | HFT 注意 |
|------|----------------|----------|
| **SYN** | 三次握手 | **连接建立延迟** — 长连接 + **TCP Fast Open**（若对端支持） |
| **ACK** | 确认 | **delayed ACK** 增 RTT 感知 — 对 **小消息发单** 不友好 |
| **FIN/RST** | 关闭/复位 | **会话恢复** 要有 **状态机** |
| **PSH** | 推送 | **`TCP_NODELAY`** 关 Nagle — **发单几乎必开** |

**选项：** **MSS**、**Window Scale**、**SACK**、**Timestamps**（RTT 测量 / PAWS）。

---

## 初始化与 `tcp_prot`

**`tcp_v4_init()`** — 注册 **`tcp_protocol`**（IPv4 handler **`tcp_v4_rcv`**）与 **`struct proto tcp_prot`**。

---

## 连接建立 (Connection Setup)

```
客户端                         服务端
  SYN ───────────────────────→
  ←────────────────────── SYN+ACK
  ACK ───────────────────────→
  → ESTABLISHED
```

内核：**`tcp_v4_connect()`** / **`tcp_v4_do_rcv()`** 状态机 **`TCP_SYN_SENT` → `ESTABLISHED`**。

**listen backlog：** **`listen()` + `accept()`** — **`somaxconn`**、**`tcp_max_syn_backlog`**；HFT **服务端** 少见，**客户端** 要 **连接池/预热**。

---

## TCP 定时器 (Timers)

| 定时器 | 作用 | HFT |
|--------|------|-----|
| **RTO 重传** | 丢包恢复 | **抖动来源** — 网络要 **低丢包** |
| **Delayed ACK** | 合并 ACK | 与 **Nagle** 叠加 → **小写延迟** |
| **Keepalive** | 检测死连接 | **会话保活** — 与 **应用 heartbeat** 二选一 |
| **TIME_WAIT** | 2MSL | **频繁重连** 时注意 **端口耗尽** — **`SO_REUSEADDR`** |

---

## 接收 `tcp_v4_rcv()`

```
tcp_v4_rcv
  → __inet_lookup_established / listener
  → tcp_v4_do_rcv
       → 校验和
       → tcp_check_req (SYN 半连接)
       → tcp_rcv_established (ESTABLISHED)
            → 序号检查、乱序队列 (ooo)
            → 拷贝到 sk_receive_queue / 直接 copy_to_user (GRO/LRO)
  → sk_data_ready
```

**GRO：** 网卡/栈 **合并段** — 降 CPU；**latency-sensitive** 有时 **disable GRO**（Ch 14）。

---

## 发送 `tcp_sendmsg()`

```
tcp_sendmsg
  → 拷贝用户数据到 send buffer (sk_write_queue)
  → tcp_write_xmit / tcp_transmit_skb
       → 分段（MSS、TSO）
       → 加 TCP 头、算 seq
       → ip_queue_xmit
```

| 调优 | 说明 |
|------|------|
| **`TCP_NODELAY`** | **禁用 Nagle** — 小 FIX 包 **立即发** |
| **`TCP_QUICKACK`** | 促 **立即 ACK**（接收侧） |
| **`SO_SNDBUF` / `SO_RCVBUF`** | 窗口与 **缓冲延迟** 权衡 |
| **TSO/GSO** | 大块 **硬件分段** — 吞吐 ↑，极端低延迟需测 |

---

## HFT 发单路径要点

```
应用 write → tcp_sendmsg → IP → 网卡
         ↑
    避免：Nagle、过大 sndbuf、与用户态 **多余拷贝**
    可选：kernel bypass (DPDK)、**固定连接**、**同 NUMA 网卡**
```

与 **Ch 4 发送**、**Ch 7 邻居 ARP**、**Ch 14 NAPI/XPS** 串成 **端到端延迟预算**。

---

← [2. UDP](./section-2-UDP.md) · [Ch 11](../README.md) · 下一节 [4. SCTP](./section-4-SCTP.md)
