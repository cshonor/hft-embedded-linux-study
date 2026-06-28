# Ch 11 §4 SCTP · Stream Control Transmission Protocol

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 4. SCTP (Stream Control Transmission Protocol)

**SCTP** — **消息导向 + 可靠 + 拥塞控制**；融合 **TCP 可靠性** 与 **UDP 消息边界**。电信 **SIGTRAN** 常用；**HFT 交易栈极少采用** — 知 **多流/多宿主** 设计思想即可。

---

## 与 TCP/UDP 对比

| | TCP | UDP | SCTP |
|---|-----|-----|------|
| 边界 | 字节流 | 消息 | **消息** |
| 可靠 | ✓ | ✗ | ✓ |
| 连接 | 单流 | 无 | **多流** |
| 地址 | 单 IP | 单 IP | **多宿主** |

---

## 重要特性

| 特性 | 说明 |
|------|------|
| **4 次握手** | **Cookie 机制** — 减轻 **SYN flood** |
| **Multihoming** | 端点 **多个 IP** — 链路故障 **切换** |
| **Multistreaming** | 单关联内 **多独立流** — 避免 **队头阻塞 (HOL)** |
| **Heartbeat** | 检测 **对端/路径不可达** |

---

## 包格式

```
[ SCTP Common Header ]
[ Chunk 1 TLV ][ Chunk 2 TLV ] …
```

**Chunk 类型：** **DATA、INIT、INIT-ACK、SACK、HEARTBEAT**… — **TLV** 自描述。

---

## 内核路径（概要）

| 阶段 | 函数/对象 |
|------|-----------|
| 注册 | **`sctp_prot`**、`sctp_rcv()` |
| 发送 | **`sctp_sendmsg()`** — 选 **stream id** |
| 接收 | **`sctp_rcv()`** → 按 **association/stream** 交付 |

**Linux：** `lksctp-tools` + 内核 **`net/sctp/`**。

---

## HFT

**几乎不用 SCTP 做 tick/发单** — **FIX over TCP**、**二进制 UDP** 为主。  
**可借鉴：** **多流** 思想 ≈ **多 TCP 连接分 channel**；**多宿主** ≈ **双网卡 bonding/独立 feed**。

---

← [3. TCP](./section-3-TCP.md) · [Ch 11](../README.md) · 下一节 [5. DCCP](./section-5-DCCP.md)
