# Ch 13 §3 数据包结构与管理实体 · Packets & Management

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 3. 数据包结构与管理实体 (Packets & Management Entities)

InfiniBand **分层头部** + **管理平面**（SM/SMA/CM）与 **数据平面**（QP 上 WR）分离。

---

## InfiniBand 数据包头部（概念）

| 头部 | 全称 | 作用 |
|------|------|------|
| **LRH** | Local Route Header | **子网内** — 源/目的 **LID**、服务级别 **SL**、包长度 |
| **GRH** | Global Route Header | **跨子网** — **GID** 路由（类 IPv6） |
| **BTH** | Base Transport Header | **QP 号、PSN、opcode** — **传输层核心** |
| **ETH** | Extended Transport Header | **RDMA 扩展** — 如 **RDMA READ/ATOMIC** 地址/长度 |
| **Payload** | | 数据 |
| **ICRC / VCRC** | | **不变/可变 CRC** 校验 |

**RoCEv2 封装：**

```
[ Ethernet ][ IP ][ UDP 4791 ][ BTH (+ ETH) ][ Payload ][ ICRC ]
```

---

## 管理实体

| 实体 | 角色 |
|------|------|
| **SM (Subnet Manager)** | **发现拓扑**、分配 **LID/GID**、路径 **SL→VL** — 通常跑在 **交换机或专用节点** |
| **SMA (Subnet Management Agent)** | 每个 **端口上的代理** — **接收 SM MAD**、配置本地端口 |
| **CM (Communication Manager)** | **连接生命周期** — **REQ/REP、RTU、DREQ** — 用户态 **rdmacm** / 内核 **ib_cm** |

```
建链（简化）:
  CM exchange → 协商 QP 参数、路径
  → QP RST → INIT → RTR → RTS
  → post Send/Recv WR
```

---

## MAD 与管理包

**MAD (Management Datagram)** — SM ↔ SMA **带内管理**；与 **数据 WR** 不同队列/优先级。

**HFT：** 生产 **SM 稳定** 即可；**CM 建链** 在 **进程启动/重连** 路径 — 非 tick 热路径，但 **重连延迟** 要纳入 **DR 设计**。

---

← [2. 寻址](./section-2-硬件组件与寻址.md) · [Ch 13](../README.md) · 下一节 [4. RDMA 资源](./section-4-RDMA核心资源与数据结构.md)
