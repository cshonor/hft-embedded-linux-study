# Ch 13 §4 RDMA 核心资源 · RDMA Resources

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 4. RDMA 核心资源与数据结构 (RDMA Resources)

RDMA 编程 **先建资源、再 post WR** — 内核 **`struct ib_*`** / 用户态 **`ibv_*`** 一一对应。

---

## 资源层次

```
Device (HCA)
  └── PD (Protection Domain)
        ├── MR / FMR (内存注册)
        ├── CQ (完成队列)
        ├── QP (队列对) ──→ 关联 CQ
        ├── SRQ (可选共享接收队列)
        └── AH (地址句柄)
```

---

## 保护域 PD

**PD** — **隔离边界**：同一 PD 内 **QP/MR/CQ** 可互操作；**跨 PD** 非法。

**作用：** 防 **错误 QP 访问他人 MR** — 多租户/多进程 **安全**。

---

## 内存区域 MR / FMR

| | **MR (Memory Region)** | **FMR (Fast MR)** |
|---|------------------------|-------------------|
| 注册 | **pin 物理页** + **DMA 映射** | **动态映射** — 批量/频繁映射场景 |
| 成本 | **注册慢**（页表 walk） | 摊销注册 |
| 访问 | RDMA **rkey + 远端 VA** | 同 |

**HFT：** **预注册大缓冲池** — 避免 tick 路径 **`ibv_reg_mr`**；与 **DPDK mempool** 思路同。

---

## 队列对 QP 与完成队列 CQ

**QP** — **Send Queue + Receive Queue**：

| QP 状态 | 含义 |
|---------|------|
| **RESET** | 初始 |
| **INIT** | 本地 PD/端口就绪 |
| **RTR** | **Ready to Receive** |
| **RTS** | **Ready to Send** — 可 post WR |

**CQ** — HCA 完成 WR 后写 **Work Completion (WC)**：

| WC 字段 | 说明 |
|---------|------|
| **status** | success / local/remote error |
| **opcode** | send/recv/rdma write… |
| **byte_len** | 实际长度 |

**poll CQ** ≈ DPDK **dequeue burst** — HFT **busy poll CQ** 降延迟。

---

## 共享接收队列 SRQ

**SRQ** — **多个 QP 共享一个 RQ**：

| 收益 | 说明 |
|------|------|
| **内存** | 少 duplicate RQ |
| **扩展** | 大量 **短连接/多 QP** 接收端 |

---

## 地址句柄 AH

**AH** — 缓存 **源端口 → 目的 GID/LID/SL/路径** — post Send 时 **指定 AH** 避免 **每次解析路径**。

---

← [3. 数据包与管理](./section-3-数据包结构与管理实体.md) · [Ch 13](../README.md) · 下一节 [5. WR 异常流](./section-5-工作请求处理与异常流.md)
