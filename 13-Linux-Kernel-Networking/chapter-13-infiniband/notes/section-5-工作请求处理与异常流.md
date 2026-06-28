# Ch 13 §5 工作请求与异常流 · Work Request Processing

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 5. 工作请求处理与异常流 (Work Request Processing & Flows)

**Work Request (WR)** — 软件 **post 到 SQ/RQ** 的工作单元；HCA **异步执行**，结果进 **CQ**。

---

## WR 类型（常见）

| Opcode | 语义 |
|--------|------|
| **Send** | 发消息 — 对端 **Recv WR** 匹配 |
| **Recv** | 预贴接收缓冲 |
| **RDMA Write** | **直接写远端 MR** — 对端 **无 CPU 参与** |
| **RDMA Read** | **读远端 MR** |
| **Atomic** | **CAS/FAA** — 分布式锁/计数 |

---

## 正常生命周期

```
ibv_post_send(qp, wr, …)
  → HCA DMA 读本地 MR
  → 链路透传 BTH (+ ETH)
  → 对端 HCA 写 MR / 消耗 Recv WR
  → 双方 CQ 产生 WC
ibv_poll_cq(cq, …)
```

**可靠连接 RC：** **PSN (Packet Sequence Number)** + **ACK/NAK** — 硬件 **重传**。

---

## 异常：Retry Flow

**触发：** 链路 **丢包**、**超时** — PSN **gap**。

```
NAK / 超时
  → 发送端 **自动重传**（在 retry 计数内）
  → 超限 → **Async Event: comm lost** — QP **ERR**
```

| 参数 | 含义 |
|------|------|
| **timeout/retry_cnt/rnr_retry** | CM 协商 / QP attr |

**HFT：** **无损以太 (PFC)** 降低丢包；仍要 **监控 async event** —  silent **QP ERR** 致命。

---

## 异常：RNR (Receiver Not Ready)

**触发：** 对端 **没有 Recv WR**（或 SRQ 空）— 入站 Send **无处落地**。

```
RNR NAK
  → 发送端 **rnr_retry** 退避重试
  → 仍失败 → 连接错误
```

**预防：** 接收端 **足够深的 Recv WR 池** / **SRQ** — 与 **TCP sk_rcvbuf** 同理。

---

## 与 TCP 重传对比

| | **TCP (Ch 11)** | **RDMA RC** |
|---|-----------------|-------------|
| 重传主体 | **内核/软件栈** | **HCA 硬件**（多数） |
| 可见性 | `retrans` 计数 | **WC error / async event** |
| 应用 | 透明 read/write | 须 **处理 CQ/事件** |

---

← [4. RDMA 资源](./section-4-RDMA核心资源与数据结构.md) · [Ch 13](../README.md) · 下一节 [6. API 区别](./section-6-内核与用户空间API区别.md)
