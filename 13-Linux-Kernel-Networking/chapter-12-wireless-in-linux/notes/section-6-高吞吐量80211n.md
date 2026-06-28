# Ch 12 §6 高吞吐量 802.11n · High Throughput

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 6. 高吞吐量技术 (IEEE 802.11n)

**802.11n** — **MIMO**、**信道绑定 (40 MHz)**、**帧聚合**、**Block Ack** — 显著提升 **吞吐**，但 **聚合增 batch 延迟**。

---

## 包聚合 (Packet Aggregation)

| 类型 | 说明 |
|------|------|
| **AMSDU** | 多个 **以太网帧** 聚成 **一个 MSDU** — 共享一个 MAC 头 |
| **AMPDU** | 多个 **MPDU**（各带 MAC 头）**物理层连续发** — 更常用 |

**收益：** 摊薄 **前导码/Preamble、ACK 开销**。

**代价：** **等待凑批** — **latency ↑**（与 HFT **小包低延迟** 相反）。

---

## 块确认 Block Ack

传统：**每 MPDU 一个 ACK** — overhead 大。

**Block Ack：**

| 类型 | 行为 |
|------|------|
| **Immediate** | 收完 **A-MPDU** 后 **立即 BA 帧** 应答 **bitmap** |
| **Delayed** | 稍后 **集中 BA** |

**会话建立：** **ADDBA Request/Response** — 协商 **窗口大小**。

---

## 与有线对比

| | 10G 以太 | 802.11n |
|---|----------|---------|
| **延迟** | **μs 级稳定** | **ms 级波动** |
| **确定性** | 高 | **CSMA/CA + 聚合** 低 |

---

← [5. 内核实现](./section-5-Mac80211内核实现.md) · [Ch 12](../README.md) · 下一节 [7. Mesh](./section-7-Mesh网络80211s.md)
