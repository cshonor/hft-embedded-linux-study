# Ch 12 §1 Mac80211 与 802.11 MAC 头部

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

### 1. Mac80211 子系统与 802.11 MAC 头部

**mac80211** — Linux 内核 **802.11 MAC 层** 软 MAC 实现；驱动通过 **`ieee80211_ops`** 挂接 **硬件**。用户态 **`wpa_supplicant` / `hostapd`** 经 **nl80211**（[Ch 2 §4](../../chapter-02-netlink-sockets/notes/section-4-通用Netlink协议.md)）配置。

---

## 802.11 协议族

| 标准 | 频段/特点 |
|------|-----------|
| **802.11a** | 5 GHz |
| **802.11b/g** | 2.4 GHz |
| **802.11n** | MIMO、聚合（§6） |
| **802.11s** | Mesh（§7） |

---

## 与 802.3 有线以太网对比

| | **802.3 以太网** | **802.11 WLAN** |
|---|------------------|-----------------|
| 介质访问 | **CSMA/CD**（半双工历史） | **CSMA/CA** — **冲突避免** |
| 确认 | **无 L2 ACK**（靠上层） | **单播通常需 ACK** |
| 拓扑 | 交换机/总线 | **AP + STA** 或 **Ad Hoc** |
| 头部 | 14 字节以太头 | **可变长 802.11 MAC 头** |

**CSMA/CA：** 发送前 **侦听 + 随机退避** — 无线 **隐藏节点** 问题 → **RTS/CTS**（可选）。

---

## `ieee80211_hdr` MAC 头部

| 字段 | 作用 |
|------|------|
| **Frame Control** | 类型/子类型（数据、管理、控制）、ToDS/FromDS |
| **Duration/ID** | NAV 虚拟载波侦听 |
| **Address 1–4** | 收/发/ BSSID / WDS 第四址 — **最多 4 地址** |
| **Sequence Control** | **分片号 + 序号** — 去重/重组 |
| **QoS Control** | **802.11e** — TID、ACK 策略 |

**帧类：**

| 类型 | 示例 |
|------|------|
| **Management** | Beacon、Probe、Auth、Assoc |
| **Control** | ACK、RTS、CTS、**Block Ack** |
| **Data** | 承载 **LLC/SNAP → IP** |

---

## HFT

共置 **光纤/铜缆 10G+** — **不走 mac80211**。Wi‑Fi **抖动大、省电增延迟** — 生产 **禁用或仅 BMC 管理**。

---

← [Ch 12](../README.md) · 下一节 [2. 拓扑](./section-2-网络拓扑结构.md)
