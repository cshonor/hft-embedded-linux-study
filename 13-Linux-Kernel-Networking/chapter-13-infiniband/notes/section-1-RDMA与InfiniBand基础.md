# Ch 13 §1 RDMA 与 InfiniBand 基础 · General

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 1. RDMA 与 InfiniBand 基础 (RDMA and InfiniBand - General)

**RDMA (Remote Direct Memory Access)** — 本机 **HCA/NIC** 在 **几乎不占用 CPU** 的情况下 **读/写远端内存** — **零拷贝、低延迟、高带宽**。

---

## 三种 RDMA 承载

| 协议 | 链路 | 说明 |
|------|------|------|
| **InfiniBand** | 原生 IB  fabric | 专用 **HCA + 交换机** |
| **RoCE** | **融合以太网 RDMA** | **RoCEv1** L2 · **RoCEv2** **UDP/IP** — 共置 **最常见 RDMA over 以太** |
| **iWARP** | **TCP/IP** | RDMA 语义跑在 **TCP** 上 — 部署少 |

**Linux：** 三者共享 **`drivers/infiniband/`** 与 **`libibverbs`** 用户态 API。

---

## 内核 RDMA 栈组织

```
用户态: libibverbs (ibv_*) / rdmacm / librdmacm
           ↕ uverbs (/dev/infiniband/uverbs*)
内核:   IB core (ib_core)
           ├── HCA 驱动 (mlx5, qib, …)
           ├── CM (通信管理器) — 连接建立
           ├── IPoIB — IP over IB
           ├── iSER — iSCSI RDMA
           └── RDS — Reliable Datagram Sockets
```

| 模块 | 作用 |
|------|------|
| **CM** | **连接/断开**、路径解析 — 类似 **socket connect** |
| **IPoIB** | 在 IB 上跑 **IP** — 管理/兼容 |
| **iSER/RDS** | 存储/集群 **专用协议** |

---

## 与 TCP/UDP 对比（HFT）

| | **TCP/UDP (Ch 11)** | **RDMA** |
|---|---------------------|----------|
| CPU | **拷贝 + 协议栈** | **卸载到 HCA** |
| 语义 | 流/数据报 | **Read/Write/Send/Recv** 直达 **已注册 MR** |
| 部署 | **交易所接入主流** | **内部集群、部分 feed、存储** |
| 旁路 | DPDK **poll mode** | **verbs** 直接 **post WR** |

→ DPDK 对照：[14-DPDK](../../14-DPDK-Low-Latency-Network/)

---

← [Ch 13](../README.md) · 下一节 [2. 硬件与寻址](./section-2-硬件组件与寻址.md)
