# Ch 13 §2 硬件组件与寻址 · Hardware & Addressing

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 2. 硬件组件与寻址 (Hardware Components & Addressing)

**InfiniBand fabric** — **HCA（主机）**、**交换机**、**路由器** 组成；**子网管理器 (SM)** 分配 **LID**、维护拓扑。

---

## 硬件组件

| 组件 | 作用 |
|------|------|
| **HCA (Host Channel Adapter)** | 主机 **RDMA 网卡** — 如 Mellanox **ConnectX** |
| **Switch** | **L2 转发** — 按 **LID** 路由 |
| **Router** | **子网间** — **GID** 全局路由 |

**RoCE：** 无独立 IB 交换机 — 走 **以太网交换机** + **PFC/ECN**（**无损以太**）保 RDMA 可靠。

---

## 寻址三要素

| 标识 | 宽度 | 作用 |
|------|------|------|
| **GUID** | 64 bit | **硬件全球唯一** — 出厂烧录（端口/node） |
| **GID** | 128 bit | **逻辑端点/组播组** — **格式同 IPv6** |
| **LID** | 16 bit | **子网内本地** — **SM 分配**，交换机 **转发键** |

```
子网内单播: 常 LRH + BTH 用 LID
跨子网:     GRH 携带 GID
RoCEv2:     UDP/IP 外层 + BTH 内层 — GID 映射 IP/UDP 五元组
```

---

## 与以太网 MAC/IP 对照

| 以太 (Ch 7) | IB |
|-------------|-----|
| **MAC** | **LID**（子网内） |
| **IP** | **GID**（全局） |
| **ARP** | **SM/CM 解析路径** |

**HFT：** RoCE 环境调 **`ibstat`/`ibv_devinfo`**、**GID 表**、**PFC 队列** — 与 **普通 ip link** 并行存在。

---

← [1. RDMA 基础](./section-1-RDMA与InfiniBand基础.md) · [Ch 13](../README.md) · 下一节 [3. 数据包与管理](./section-3-数据包结构与管理实体.md)
