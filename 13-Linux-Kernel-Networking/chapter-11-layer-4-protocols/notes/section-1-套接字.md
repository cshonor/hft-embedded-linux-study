# Ch 11 §1 套接字 · Sockets

> **Linux Kernel Networking** · Rami Rosen · **精读 🔴**

### 1. 套接字 (Sockets)

**Socket** — L4 与 **用户态** 的 **标准接口**（BSD API）；内核用 **`struct socket`**（面向 VFS/file）与 **`struct sock`**（协议无关、挂 sk_buff 队列）分层表示 **同一连接**。

---

## 套接字类型

| 类型 | 常量 | 典型协议 | HFT 用途 |
|------|------|----------|----------|
| **流** | `SOCK_STREAM` | TCP | **发单、会话、FIX** |
| **数据报** | `SOCK_DGRAM` | UDP | **行情组播、快照** |
| **原始** | `SOCK_RAW` | IP 直访 | **pcap/诊断**（生产少用） |
| **数据报连接** | `SOCK_DCCP` | DCCP | 极少 |
| **顺序包** | `SOCK_SEQPACKET` | SCTP | 极少 |

**地址族：** `AF_INET` / `AF_INET6` — 与 `PF_*` 在 Linux 上等价。

---

## 核心数据结构

```
用户态 fd
  ↔ struct socket  (ops: sendmsg/recvmsg/bind/listen…)
       → struct sock  (sk_receive_queue / sk_write_queue)
            → struct proto (tcp_prot / udp_prot)
            → struct inet_sock (四元组、端口、选项)
```

| 结构 | 层次 | 要点 |
|------|------|------|
| **`struct socket`** | **VFS 层** | `struct file *file`；**`socket->ops`** 系统调用入口 |
| **`struct sock`** | **网络核心** | **接收/发送队列**、backlog、回调 **`sk_data_ready`** |
| **`struct inet_sock`** | IPv4 扩展 | `dport/sport`、`rcvbuf/sndbuf`、**`sk_reuseport`** |

**收包唤醒路径：**

```
网卡 → NAPI → IP → L4 (tcp_v4_rcv / udp_rcv)
  → sk->sk_data_ready()  → wake_up 用户线程/epoll
```

→ 软中断与 NAPI：[Ch 14](../../chapter-14-高级主题.md)

---

## `msghdr` 与 IO 向量

**`sendmsg` / `recvmsg`** 用 **`struct msghdr`** 描述 **scatter/gather**：

| 字段 | 作用 |
|------|------|
| **`msg_iov`** | **`struct iovec[]`** — 多缓冲区 |
| **`msg_name`** | 对端地址（UDP `sendto` 语义） |
| **`msg_control`** | **辅助数据** — `SCM_TIMESTAMPING`、`IP_PKTINFO` 等 |
| **`msg_flags`** | `MSG_DONTWAIT`、`MSG_PEEK`、`MSG_TRUNC`… |

**HFT：**

| 实践 | 原因 |
|------|------|
| **`recvmmsg` / `sendmmsg`** | **批量 syscall** 降开销 |
| **`SO_REUSEPORT` + 多 fd** | **RSS 多队列** 到多核收包 |
| **`SO_BUSY_POLL`** | 内核 **busy poll**（Ch 14）— 降 wakeup 延迟 |
| **固定 buffer + `MSG_DONTWAIT`** | 热路径 **无阻塞** |

---

## 与 UNP / CSAPP 对照

| 用户态 | 内核本章 |
|--------|----------|
| `socket/bind/listen/accept` | `sock_create`、**`inet_bind`**、**`tcp_v4_connect`** |
| `read/write` | **`sock_recvmsg` → tcp_recvmsg / udp_recvmsg** |
| `setsockopt` | **`tcp_setsockopt`** — `TCP_NODELAY`、`SO_RCVBUF`… |

→ API 书：[11-UNP-Vol1](../../../11-UNP-Vol1/) · [01-CSAPP Ch11](../../../01-CSAPP-3rd/chapter-11-network-programming/)

---

← [Ch 11](../README.md) · 下一节 [2. UDP](./section-2-UDP.md)
