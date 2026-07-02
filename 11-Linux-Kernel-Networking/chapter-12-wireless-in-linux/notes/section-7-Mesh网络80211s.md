# Ch 12 §7 Mesh 网络 802.11s · Mesh Networking

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 7. Mesh 网络 (802.11s)

**802.11s** — **无线网状网** — 节点 **多跳转发**，无 **单一中心 AP**（可含 **Mesh Portal** 接有线）。

---

## 拓扑

| 类型 | 说明 |
|------|------|
| **全连接 Mesh** | 节点 **两两可达**（小规模） |
| **部分连接** | **稀疏链路** — 需 **多跳路由** |

---

## HWMP 路由

**HWMP (Hybrid Wireless Mesh Protocol)** — **802.11s 默认 L2 路由**：

| 机制 | 类比 |
|------|------|
| **On-demand (PREQ/PREP)** | **AODV 式** — 按需查路 |
| **Proactive (RANN)** | **树/根公告** — 主动维护 |

**工作在 MAC 层** — **非 IP 路由**（[Ch 5](../../chapter-05-ipv4-routing-subsystem/)）— Mesh 节点 **以太头转发** 由 **802.11s 路径选下一跳**。

---

## Linux 支持

**`NL80211_IFTYPE_MESH_POINT`** — **`iw mesh join`**、**batman-adv** 等 **替代/叠加** 方案也存在。

**HFT：** **无生产用途** — 多跳 **延迟不可控**。

---

← [6. 802.11n](./section-6-高吞吐量80211n.md) · [Ch 12](../README.md) · 下一节 [8. 开发流程](./section-8-无线开发流程.md)
