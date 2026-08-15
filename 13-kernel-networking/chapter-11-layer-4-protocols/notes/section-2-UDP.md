# Ch 11 §2 UDP · User Datagram Protocol

> **Linux Kernel Networking** · Rami Rosen · **精读 🔴**

### 2. UDP (User Datagram Protocol)

**UDP** — **无连接、不可靠、面向消息**；**无拥塞控制**、无重传。头部 **8 字节** — 极低 overhead，适合 **低延迟、可丢包** 场景（**行情 tick**）。

---

## UDP 头部

```
 0                   16                  31
├──────── src port ──┼─── dst port ──────┤
├──────── length ────┼─── checksum ──────┤
│              payload …                  │
```

| 字段 | 说明 |
|------|------|
| **源/目的端口** | 多路复用 |
| **长度** | UDP 头 + 数据（字节） |
| **校验和** | IPv4 可 **0（禁用）**；IPv6 **强制** |

---

## 内核初始化

**`udp_protocol`**（`struct proto`）注册到 **`inet_protos`**：

| 回调 | 作用 |
|------|------|
| **`init`** | per-socket 初始化 |
| **`sendmsg`** | **`udp_sendmsg()`** |
| **`recvmsg`** | **`udp_recvmsg()`** |
| **`hash`** | 端口 lookup — **`udp_hash`** |

IPv4 入口：**`udp_rcv()`** — IP 层 demux 后按 **五元组** 找 socket。

---

## 发送路径 `udp_sendmsg()`

```
udp_sendmsg
  → 查路由 / 缓存 dst
  → 构造 skb + UDP 头
  → ip_append_data / ip_queue_xmit 路径（[Ch 4 §5](../../chapter-04-ipv4/notes/section-5-发送IPv4数据包.md)）
  → ip_output → 邻居 → dev_queue_xmit
```

| 选项 | HFT |
|------|-----|
| **`UDP_CORK` / `MSG_MORE`** | 合并小包 — **tick 路径常关** |
| **GSO/UFO** | 内核 **offload 分片** — 与 **固定 MTU** 策略冲突时注意 |
| **`SO_SNDBUF`** | 过小 → **`EAGAIN`**；过大 → **缓冲延迟** |

---

## 接收路径 `udp_rcv()`

```
udp_rcv(skb)
  → udp4_lib_lookup (sport/daddr/dport/saddr)
  → 校验和（可选 offload）
  → __udp_queue_rcv_skb → sk_receive_queue
  → sk->sk_data_ready()  → epoll/poll 唤醒
```

**组播：** `IP_ADD_MEMBERSHIP` → IGMP（[note-组播IGMP](../../note-组播IGMP.md)）→ 内核复制到 **多个 mcast socket**。

**丢包点：**

| 位置 | 原因 |
|------|------|
| **`netdev_max_backlog`** | softirq 来不及 |
| **`sk_rcvbuf` 满** | 用户态读慢 |
| **无匹配 socket** | ICMP port unreachable |

---

## HFT 对照

| 内核 UDP 栈 | DPDK / 内核旁路 |
|-------------|-----------------|
| **syscall + 拷贝** | **mmap ring / zero-copy** |
| **softirq 调度** | **poll mode / busy poll** |
| **通用 hash lookup** | **固定流、预绑 queue** |

**行情典型：** **组播 UDP** + **单播 snapshot**；调 **`SO_RCVBUF`**、**`recvmmsg`**、**CPU 亲和**、**RPS/RFS**（[Ch 14 §3](../../chapter-14-advanced-topics/notes/section-3-忙轮询套接字与收包路径.md)）。

---

← [1. 套接字](./section-1-套接字.md) · [Ch 11](../README.md) · 下一节 [3. TCP](./section-3-TCP.md)
